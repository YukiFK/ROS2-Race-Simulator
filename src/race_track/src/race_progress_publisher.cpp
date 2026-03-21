#include <cstdint>
#include <filesystem>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include "race_interfaces/msg/lap_event.hpp"
#include "race_interfaces/msg/race_command.hpp"
#include "race_interfaces/msg/race_state.hpp"
#include "race_interfaces/msg/vehicle_race_status.hpp"
#include "race_track/lap_event_assembler.hpp"
#include "race_track/race_state_assembler.hpp"
#include "race_track/single_vehicle_runtime.hpp"
#include "race_track/track_loader.hpp"
#include "race_track/track_validator.hpp"
#include "race_track/vehicle_race_status_assembler.hpp"
#include "rclcpp/rclcpp.hpp"

namespace race_track
{
namespace
{

constexpr char kVehicleId[] = "demo_vehicle_1";
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

class RaceProgressPublisher : public rclcpp::Node
{
public:
  explicit RaceProgressPublisher(const std::filesystem::path & sample_track_path)
  : Node("race_progress_publisher"),
    track_(loadTrackFromYaml(sample_track_path.string())),
    target_lap_count_(declare_parameter<std::int64_t>("target_lap_count", 2)),
    runtime_(
      track_,
      {{-2.0, 0.0}, {-0.5, 0.2}, {1.0, 0.2}, {6.0, 0.1}, {11.0, 0.4}, {18.0, 4.8},
       {9.0, 5.0}, {0.5, 0.0}, {-1.0, 0.0}, {1.5, -0.1}, {4.0, 4.0}},
      target_lap_count_)
  {
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
    publishRaceState(runtime_.current_step_sec(), runtime_.snapshot());
  }

private:
  void onTimer()
  {
    const SingleVehicleTickResult tick_result = runtime_.tick();
    if (!tick_result.advanced) {
      return;
    }

    if (tick_result.crossing_detected) {
      publishLapEvent(tick_result.step_sec, tick_result.progress_update);
    }

    publishVehicleRaceStatus(tick_result.step_sec, tick_result.progress_update);
    publishRaceState(tick_result.step_sec, tick_result.progress_update.snapshot);

    RCLCPP_INFO(
      get_logger(),
      "step=%zu position=(%.3f, %.3f) nearest_centerline_index=%zu distance=%.3f "
      "off_track=%s crossing=%s lap_count=%d off_track_count=%d",
      runtime_.step_index() - 1U, tick_result.current_position.x, tick_result.current_position.y,
      tick_result.progress_update.nearest_centerline_index,
      tick_result.progress_update.distance_to_centerline,
      tick_result.progress_update.is_off_track ? "true" : "false",
      tick_result.progress_update.crossing_detected ? "true" : "false",
      tick_result.progress_update.snapshot.lap_count,
      tick_result.progress_update.snapshot.off_track_count);

    if (tick_result.just_completed) {
      RCLCPP_INFO(
        get_logger(), "Target lap count reached (%d/%ld), marking race completed",
        tick_result.progress_update.snapshot.lap_count, target_lap_count_);
      return;
    }

    if (tick_result.stopped_without_completion) {
      RCLCPP_WARN(
        get_logger(),
        "Progression stopped at end of fixed positions before reaching target laps (%d/%ld)",
        tick_result.progress_update.snapshot.lap_count, target_lap_count_);
    }
  }

  void onRaceCommand(const race_interfaces::msg::RaceCommand::SharedPtr msg)
  {
    switch (msg->command) {
      case race_interfaces::msg::RaceCommand::START:
        if (runtime_.start()) {
          RCLCPP_INFO(
            get_logger(), "Received START after completion or final step, resetting progression first");
        }
        publishRaceState(runtime_.current_step_sec(), runtime_.snapshot());
        RCLCPP_INFO(get_logger(), "Received START command, progression started");
        break;
      case race_interfaces::msg::RaceCommand::STOP:
        runtime_.stop();
        publishRaceState(runtime_.current_step_sec(), runtime_.snapshot());
        RCLCPP_INFO(get_logger(), "Received STOP command, progression stopped");
        break;
      case race_interfaces::msg::RaceCommand::RESET:
        runtime_.reset();
        publishRaceState(runtime_.current_step_sec(), runtime_.snapshot());
        RCLCPP_INFO(get_logger(), "Received RESET command, progression reset");
        break;
      default:
        RCLCPP_WARN(get_logger(), "Received unknown race command: %u", msg->command);
        break;
    }
  }

  void publishLapEvent(const std::int32_t step_sec, const ProgressUpdate & progress_update)
  {
    lap_event_publisher_->publish(
      lap_event_assembler_.assemble(step_sec, kVehicleId, progress_update));
  }

  void publishRaceState(const std::int32_t step_sec, const ProgressSnapshot & snapshot)
  {
    race_interfaces::msg::RaceState race_state =
      race_state_assembler_.assemble(runtime_.current_race_status(), step_sec, snapshot);
    race_state_publisher_->publish(race_state);
  }

  void publishVehicleRaceStatus(
    const std::int32_t step_sec, const ProgressUpdate & progress_update)
  {
    status_publisher_->publish(
      vehicle_race_status_assembler_.assemble(step_sec, kVehicleId, progress_update));
  }

  TrackModel track_;
  LapEventAssembler lap_event_assembler_;
  RaceStateAssembler race_state_assembler_;
  VehicleRaceStatusAssembler vehicle_race_status_assembler_;
  SingleVehicleRuntime runtime_;
  rclcpp::Publisher<race_interfaces::msg::VehicleRaceStatus>::SharedPtr status_publisher_;
  rclcpp::Publisher<race_interfaces::msg::LapEvent>::SharedPtr lap_event_publisher_;
  rclcpp::Publisher<race_interfaces::msg::RaceState>::SharedPtr race_state_publisher_;
  rclcpp::Subscription<race_interfaces::msg::RaceCommand>::SharedPtr command_subscriber_;
  rclcpp::TimerBase::SharedPtr timer_;
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
