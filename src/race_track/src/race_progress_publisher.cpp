#include <cstdint>
#include <filesystem>
#include <limits>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include "builtin_interfaces/msg/duration.hpp"
#include "race_interfaces/msg/lap_event.hpp"
#include "race_interfaces/msg/race_command.hpp"
#include "race_interfaces/msg/vehicle_race_status.hpp"
#include "race_track/geometry.hpp"
#include "race_track/track_loader.hpp"
#include "race_track/track_validator.hpp"
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
    positions_(
      {{-2.0, 0.0}, {-0.5, 0.2}, {1.0, 0.2}, {6.0, 0.1}, {11.0, 0.4}, {18.0, 4.8},
       {9.0, 5.0}, {0.5, 0.0}, {-1.0, 0.0}, {1.5, -0.1}, {4.0, 4.0}})
  {
    validateTrackOrThrow(track_);

    status_publisher_ =
      create_publisher<race_interfaces::msg::VehicleRaceStatus>("/vehicle_race_status", 10);
    lap_event_publisher_ = create_publisher<race_interfaces::msg::LapEvent>("/lap_event", 10);
    command_subscriber_ = create_subscription<race_interfaces::msg::RaceCommand>(
      "/race_command", 10,
      std::bind(&RaceProgressPublisher::onRaceCommand, this, std::placeholders::_1));
    timer_ = create_wall_timer(
      std::chrono::seconds(1), std::bind(&RaceProgressPublisher::onTimer, this));

    RCLCPP_INFO(
      get_logger(), "Loaded track '%s' from %s", track_.track_name.c_str(),
      sample_track_path.c_str());
    RCLCPP_INFO(get_logger(), "Waiting for race commands on /race_command");
  }

private:
  void onTimer()
  {
    if (!running_) {
      return;
    }

    if (step_index_ >= positions_.size()) {
      running_ = false;
      return;
    }

    const std::int32_t step_sec = static_cast<std::int32_t>(step_index_);
    const Point2d & current = positions_[step_index_];
    const std::size_t nearest_index = findNearestCenterlineIndex(track_.centerline, current);
    const double distance = distanceToCenterline(track_.centerline, current);
    const bool is_off_track = distance > (track_.track_width / 2.0);
    if (is_off_track) {
      ++off_track_count_;
    }

    bool crossing_detected = false;
    if (step_index_ > 0U) {
      crossing_detected = isForwardCrossingStartLine(
        positions_[step_index_ - 1U], current, track_.start_line, track_.forward_hint);
      if (crossing_detected) {
        publishLapEvent(step_sec);
      }
    }

    publishVehicleRaceStatus(step_sec, is_off_track);

    RCLCPP_INFO(
      get_logger(),
      "step=%zu position=(%.3f, %.3f) nearest_centerline_index=%zu distance=%.3f "
      "off_track=%s crossing=%s lap_count=%d off_track_count=%d",
      step_index_, current.x, current.y, nearest_index, distance, is_off_track ? "true" : "false",
      crossing_detected ? "true" : "false", lap_count_, off_track_count_);

    ++step_index_;
    if (step_index_ >= positions_.size()) {
      running_ = false;
      RCLCPP_INFO(get_logger(), "Reached final step, stopping progression");
    }
  }

  void onRaceCommand(const race_interfaces::msg::RaceCommand::SharedPtr msg)
  {
    switch (msg->command) {
      case race_interfaces::msg::RaceCommand::START:
        if (step_index_ >= positions_.size()) {
          RCLCPP_INFO(get_logger(), "Received START at final step, resetting progression first");
          resetProgress();
        }
        running_ = true;
        RCLCPP_INFO(get_logger(), "Received START command, progression started");
        break;
      case race_interfaces::msg::RaceCommand::STOP:
        running_ = false;
        RCLCPP_INFO(get_logger(), "Received STOP command, progression stopped");
        break;
      case race_interfaces::msg::RaceCommand::RESET:
        resetProgress();
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
    step_index_ = 0U;
    lap_count_ = 0;
    off_track_count_ = 0;
    lap_start_step_sec_ = 0;
    last_lap_time_sec_ = 0;
    best_lap_time_sec_ = 0;
    best_lap_time_candidate_sec_ = std::numeric_limits<std::int32_t>::max();
  }

  void publishLapEvent(const std::int32_t step_sec)
  {
    const std::int32_t lap_time_sec = step_sec - lap_start_step_sec_;
    ++lap_count_;
    last_lap_time_sec_ = lap_time_sec;
    if (lap_time_sec < best_lap_time_candidate_sec_) {
      best_lap_time_candidate_sec_ = lap_time_sec;
      best_lap_time_sec_ = lap_time_sec;
    }
    lap_start_step_sec_ = step_sec;

    race_interfaces::msg::LapEvent lap_event;
    lap_event.header.stamp = rclcpp::Time(step_sec, 0U, RCL_ROS_TIME);
    lap_event.header.frame_id = kFrameId;
    lap_event.vehicle_id = kVehicleId;
    lap_event.lap_count = lap_count_;
    lap_event.lap_time = makeDuration(lap_time_sec);
    lap_event.best_lap_time = makeDuration(best_lap_time_sec_);
    lap_event.has_finished = false;
    lap_event_publisher_->publish(lap_event);
  }

  void publishVehicleRaceStatus(const std::int32_t step_sec, const bool is_off_track)
  {
    race_interfaces::msg::VehicleRaceStatus status;
    status.header.stamp = rclcpp::Time(step_sec, 0U, RCL_ROS_TIME);
    status.header.frame_id = kFrameId;
    status.vehicle_id = kVehicleId;
    status.lap_count = lap_count_;
    status.current_lap_time = makeDuration(step_sec - lap_start_step_sec_);
    status.last_lap_time = makeDuration(last_lap_time_sec_);
    status.best_lap_time = makeDuration(best_lap_time_sec_);
    status.total_elapsed_time = makeDuration(step_sec);
    status.has_finished = false;
    status.is_off_track = is_off_track;
    status.off_track_count = off_track_count_;
    status_publisher_->publish(status);
  }

  TrackModel track_;
  std::vector<Point2d> positions_;
  rclcpp::Publisher<race_interfaces::msg::VehicleRaceStatus>::SharedPtr status_publisher_;
  rclcpp::Publisher<race_interfaces::msg::LapEvent>::SharedPtr lap_event_publisher_;
  rclcpp::Subscription<race_interfaces::msg::RaceCommand>::SharedPtr command_subscriber_;
  rclcpp::TimerBase::SharedPtr timer_;
  bool running_{false};
  std::size_t step_index_{0U};
  std::int32_t lap_count_{0};
  std::int32_t off_track_count_{0};
  std::int32_t lap_start_step_sec_{0};
  std::int32_t last_lap_time_sec_{0};
  std::int32_t best_lap_time_sec_{0};
  std::int32_t best_lap_time_candidate_sec_{std::numeric_limits<std::int32_t>::max()};
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
