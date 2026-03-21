#include <cstdint>
#include <filesystem>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include "builtin_interfaces/msg/duration.hpp"
#include "race_interfaces/msg/lap_event.hpp"
#include "race_interfaces/msg/race_command.hpp"
#include "race_interfaces/msg/race_state.hpp"
#include "race_interfaces/msg/vehicle_race_status.hpp"
#include "race_track/completion_evaluator.hpp"
#include "race_track/geometry.hpp"
#include "race_track/progress_tracker.hpp"
#include "race_track/race_state_assembler.hpp"
#include "race_track/track_loader.hpp"
#include "race_track/track_validator.hpp"
#include "race_track/vehicle_race_status_assembler.hpp"
#include "rclcpp/rclcpp.hpp"

namespace race_track
{
namespace
{

constexpr char kVehicleId[] = "demo_vehicle_1";
constexpr char kFrameId[] = "map";
std::filesystem::path getExecutableDir(const char * argv0)
{
  if (argv0 == nullptr) {
    return std::filesystem::current_path();
  }

  const std::filesystem::path executable_path = std::filesystem::absolute(argv0);
  if (executable_path.has_parent_path()) {
    return executable_path.parent_path();
  }

  return std::filesystem::current_path();
}

std::filesystem::path resolveSampleTrackPath(const char * argv0)
{
  const std::filesystem::path executable_dir = getExecutableDir(argv0);
  const std::vector<std::filesystem::path> candidates = {
    std::filesystem::path(RACE_TRACK_SOURCE_DIR) / "config" / "sample_track.yaml",
    executable_dir / ".." / ".." / "src" / "race_track" / "config" / "sample_track.yaml",
    executable_dir / ".." / ".." / ".." / "src" / "race_track" / "config" / "sample_track.yaml",
    executable_dir / ".." / ".." / ".." / "share" / "race_track" / "config" / "sample_track.yaml",
    executable_dir / ".." / ".." / "share" / "race_track" / "config" / "sample_track.yaml",
  };

  for (const auto & candidate : candidates) {
    const std::filesystem::path normalized = candidate.lexically_normal();
    if (std::filesystem::exists(normalized)) {
      return normalized;
    }
  }

  throw std::runtime_error("Failed to locate config/sample_track.yaml");
}

builtin_interfaces::msg::Duration makeDuration(const std::int32_t seconds)
{
  builtin_interfaces::msg::Duration duration;
  duration.sec = seconds;
  duration.nanosec = 0U;
  return duration;
}

class RaceProgressPublisher : public rclcpp::Node
{
public:
  explicit RaceProgressPublisher(const std::filesystem::path & sample_track_path)
  : Node("race_progress_publisher"),
    track_(loadTrackFromYaml(sample_track_path.string())),
    progress_tracker_(track_),
    target_lap_count_(declare_parameter<std::int64_t>("target_lap_count", 2)),
    positions_(
      {{-2.0, 0.0}, {-0.5, 0.2}, {1.0, 0.2}, {6.0, 0.1}, {11.0, 0.4}, {18.0, 4.8},
       {9.0, 5.0}, {0.5, 0.0}, {-1.0, 0.0}, {1.5, -0.1}, {4.0, 4.0}})
  {
    if (target_lap_count_ <= 0) {
      throw std::runtime_error("target_lap_count must be greater than zero");
    }

    validateTrackOrThrow(track_);

    status_publisher_ =
      create_publisher<race_interfaces::msg::VehicleRaceStatus>("/vehicle_race_status", 10);
    lap_event_publisher_ = create_publisher<race_interfaces::msg::LapEvent>("/lap_event", 10);
    race_state_publisher_ = create_publisher<race_interfaces::msg::RaceState>("/race_state", 10);
    command_subscriber_ = create_subscription<race_interfaces::msg::RaceCommand>(
      "/race_command", 10,
      std::bind(&RaceProgressPublisher::onRaceCommand, this, std::placeholders::_1));
    timer_ = create_wall_timer(
      std::chrono::seconds(1), std::bind(&RaceProgressPublisher::onTimer, this));

    RCLCPP_INFO(
      get_logger(), "Loaded track '%s' from %s", track_.track_name.c_str(),
      sample_track_path.c_str());
    RCLCPP_INFO(get_logger(), "Target lap count: %ld", target_lap_count_);
    RCLCPP_INFO(get_logger(), "Waiting for race commands on /race_command");
    publishRaceState(currentStepSec(), progress_tracker_.snapshot());
  }

private:
  void onTimer()
  {
    if (!running_ || completed_) {
      return;
    }

    if (step_index_ >= positions_.size()) {
      running_ = false;
      return;
    }

    const std::int32_t step_sec = static_cast<std::int32_t>(step_index_);
    const Point2d & current = positions_[step_index_];
    ProgressUpdate progress_update = progress_tracker_.update(step_sec, current);
    const CompletionDecision decision = completion_evaluator_.evaluate(
      progress_update.snapshot, step_index_, positions_.size(), target_lap_count_);

    if (decision.should_complete) {
      running_ = false;
      completed_ = true;
      progress_tracker_.setHasFinished(true);
      progress_update.snapshot = progress_tracker_.snapshot();
    }

    if (decision.should_stop_without_completion) {
      running_ = false;
    }

    if (progress_update.crossing_detected) {
      publishLapEvent(step_sec, progress_update);
    }

    publishVehicleRaceStatus(step_sec, progress_update);
    publishRaceState(step_sec, progress_update.snapshot);

    RCLCPP_INFO(
      get_logger(),
      "step=%zu position=(%.3f, %.3f) nearest_centerline_index=%zu distance=%.3f "
      "off_track=%s crossing=%s lap_count=%d off_track_count=%d",
      step_index_, current.x, current.y, progress_update.nearest_centerline_index,
      progress_update.distance_to_centerline, progress_update.is_off_track ? "true" : "false",
      progress_update.crossing_detected ? "true" : "false", progress_update.snapshot.lap_count,
      progress_update.snapshot.off_track_count);

    if (decision.should_complete) {
      RCLCPP_INFO(
        get_logger(), "Target lap count reached (%d/%ld), marking race completed",
        progress_update.snapshot.lap_count, target_lap_count_);
      return;
    }

    if (decision.should_stop_without_completion) {
      running_ = false;
      RCLCPP_WARN(
        get_logger(),
        "Progression stopped at end of fixed positions before reaching target laps (%d/%ld)",
        progress_update.snapshot.lap_count, target_lap_count_);
      ++step_index_;
      return;
    }

    ++step_index_;
  }

  void onRaceCommand(const race_interfaces::msg::RaceCommand::SharedPtr msg)
  {
    switch (msg->command) {
      case race_interfaces::msg::RaceCommand::START:
        if (completed_ || step_index_ >= positions_.size()) {
          RCLCPP_INFO(
            get_logger(), "Received START after completion or final step, resetting progression first");
          resetProgress();
        }
        completed_ = false;
        running_ = true;
        publishRaceState(currentStepSec(), progress_tracker_.snapshot());
        RCLCPP_INFO(get_logger(), "Received START command, progression started");
        break;
      case race_interfaces::msg::RaceCommand::STOP:
        running_ = false;
        completed_ = false;
        publishRaceState(currentStepSec(), progress_tracker_.snapshot());
        RCLCPP_INFO(get_logger(), "Received STOP command, progression stopped");
        break;
      case race_interfaces::msg::RaceCommand::RESET:
        resetProgress();
        publishRaceState(currentStepSec(), progress_tracker_.snapshot());
        RCLCPP_INFO(get_logger(), "Received RESET command, progression reset");
        break;
      default:
        RCLCPP_WARN(get_logger(), "Received unknown race command: %u", msg->command);
        break;
    }
  }

  void resetProgress()
  {
    running_ = false;
    completed_ = false;
    step_index_ = 0U;
    progress_tracker_.reset();
  }

  void publishLapEvent(const std::int32_t step_sec, const ProgressUpdate & progress_update)
  {
    race_interfaces::msg::LapEvent lap_event;
    lap_event.header.stamp = rclcpp::Time(step_sec, 0U, RCL_ROS_TIME);
    lap_event.header.frame_id = kFrameId;
    lap_event.vehicle_id = kVehicleId;
    lap_event.lap_count = progress_update.snapshot.lap_count;
    lap_event.lap_time = makeDuration(progress_update.crossed_lap_time_sec);
    lap_event.best_lap_time = makeDuration(progress_update.snapshot.best_lap_time_sec);
    lap_event.has_finished = progress_update.snapshot.has_finished;
    lap_event_publisher_->publish(lap_event);
  }

  void publishRaceState(const std::int32_t step_sec, const ProgressSnapshot & snapshot)
  {
    race_interfaces::msg::RaceState race_state =
      race_state_assembler_.assemble(currentRaceStatus(), step_sec, snapshot);
    race_state_publisher_->publish(race_state);
  }

  void publishVehicleRaceStatus(
    const std::int32_t step_sec, const ProgressUpdate & progress_update)
  {
    status_publisher_->publish(
      vehicle_race_status_assembler_.assemble(step_sec, kVehicleId, progress_update));
  }

  std::int32_t currentStepSec() const
  {
    if (positions_.empty()) {
      return 0;
    }

    const std::size_t clamped_index =
      step_index_ < positions_.size() ? step_index_ : positions_.size() - 1U;
    return static_cast<std::int32_t>(clamped_index);
  }

  std::string currentRaceStatus() const
  {
    if (completed_) {
      return "completed";
    }

    return running_ ? "running" : "stopped";
  }

  TrackModel track_;
  ProgressTracker progress_tracker_;
  SingleVehicleCompletionEvaluator completion_evaluator_;
  RaceStateAssembler race_state_assembler_;
  VehicleRaceStatusAssembler vehicle_race_status_assembler_;
  std::vector<Point2d> positions_;
  rclcpp::Publisher<race_interfaces::msg::VehicleRaceStatus>::SharedPtr status_publisher_;
  rclcpp::Publisher<race_interfaces::msg::LapEvent>::SharedPtr lap_event_publisher_;
  rclcpp::Publisher<race_interfaces::msg::RaceState>::SharedPtr race_state_publisher_;
  rclcpp::Subscription<race_interfaces::msg::RaceCommand>::SharedPtr command_subscriber_;
  rclcpp::TimerBase::SharedPtr timer_;
  bool running_{false};
  bool completed_{false};
  std::size_t step_index_{0U};
  std::int64_t target_lap_count_{2};
};

}  // namespace

}  // namespace race_track

int main(int argc, char ** argv)
{
  rclcpp::init(argc, argv);

  try {
    const std::filesystem::path sample_track_path = race_track::resolveSampleTrackPath(argv[0]);
    auto node = std::make_shared<race_track::RaceProgressPublisher>(sample_track_path);
    rclcpp::spin(node);
    rclcpp::shutdown();
    return 0;
  } catch (const std::exception & ex) {
    RCLCPP_FATAL(
      rclcpp::get_logger("race_progress_publisher"), "race_progress_publisher failed: %s",
      ex.what());
    rclcpp::shutdown();
    return 1;
  }
}
