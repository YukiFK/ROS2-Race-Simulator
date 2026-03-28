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
#include "race_track/race_coordinator.hpp"
#include "race_track/race_state_assembler.hpp"
#include "race_track/track_loader.hpp"
#include "race_track/track_validator.hpp"
#include "race_track/vehicle_race_status_assembler.hpp"
#include "rclcpp/rclcpp.hpp"

namespace race_track
{
namespace
{

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

RaceCoordinator::VehicleRuntimePositions makePublisherRuntimePositions()
{
  return {
    {{-2.0, 0.0}, {-0.5, 0.2}, {1.0, 0.2}, {6.0, 0.1}, {11.0, 0.4}, {18.0, 4.8},
     {9.0, 5.0}, {0.5, 0.0}, {-1.0, 0.0}, {1.5, -0.1}, {4.0, 4.0}},
    {{-2.0, 0.5}, {-0.5, 0.7}, {1.0, 0.7}, {6.0, 0.6}, {11.0, 0.9}, {18.0, 5.3},
     {9.0, 5.5}, {0.5, 0.5}, {-1.0, 0.5}, {1.5, 0.4}, {4.0, 4.5}},
  };
}

class RaceProgressPublisher : public rclcpp::Node
{
public:
  explicit RaceProgressPublisher(const std::filesystem::path & sample_track_path)
  : Node("race_progress_publisher"),
    target_lap_count_(declare_parameter<std::int64_t>("target_lap_count", 2)),
    coordinator_(
      loadTrackFromYaml(sample_track_path.string()), makePublisherRuntimePositions(),
      target_lap_count_)
  {
    validateTrackOrThrow(coordinator_.track());

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
      get_logger(), "Loaded track '%s' from %s", coordinator_.track().track_name.c_str(),
      sample_track_path.c_str());
    RCLCPP_INFO(get_logger(), "Target lap count: %ld", target_lap_count_);
    RCLCPP_INFO(get_logger(), "Race coordinator initialized for %zu participating vehicles",
      coordinator_.vehicle_count());
    RCLCPP_INFO(get_logger(), "Waiting for race commands on /race_command");
    publishRaceStateFromCoordinatorSource();
  }

private:
  void publishVehicleLevelOutputsForTick(
    const std::string & vehicle_id, SingleVehicleRuntime & runtime,
    const SingleVehicleTickResult & tick_result)
  {
    if (tick_result.crossing_detected) {
      publishLapEvent(vehicle_id, tick_result.step_sec, tick_result.progress_update);
    }

    publishVehicleRaceStatus(vehicle_id, tick_result.step_sec, tick_result.progress_update);

    RCLCPP_INFO(
      get_logger(),
      "vehicle_id=%s step=%zu position=(%.3f, %.3f) nearest_centerline_index=%zu distance=%.3f "
      "off_track=%s crossing=%s lap_count=%d off_track_count=%d",
      vehicle_id.c_str(), runtime.step_index() - 1U, tick_result.current_position.x,
      tick_result.current_position.y,
      tick_result.progress_update.nearest_centerline_index,
      tick_result.progress_update.distance_to_centerline,
      tick_result.progress_update.is_off_track ? "true" : "false",
      tick_result.progress_update.crossing_detected ? "true" : "false",
      tick_result.progress_update.snapshot.lap_count,
      tick_result.progress_update.snapshot.off_track_count);

    if (tick_result.just_completed) {
      RCLCPP_INFO(
        get_logger(), "vehicle_id=%s target lap count reached (%d/%ld), marking runtime completed",
        vehicle_id.c_str(), tick_result.progress_update.snapshot.lap_count, target_lap_count_);
      return;
    }

    if (tick_result.stopped_without_completion) {
      RCLCPP_WARN(
        get_logger(),
        "vehicle_id=%s progression stopped at end of fixed positions before reaching target laps "
        "(%d/%ld)",
        vehicle_id.c_str(), tick_result.progress_update.snapshot.lap_count, target_lap_count_);
    }
  }

  void applyRaceWideCommand(const std::uint8_t command)
  {
    // ROS command ingress lives here; race-wide command application policy stays in RaceCoordinator.
    switch (command) {
      case race_interfaces::msg::RaceCommand::START:
        if (coordinator_.start()) {
          RCLCPP_INFO(
            get_logger(), "Received START after completion or final step, resetting progression first");
        }
        publishRaceStateFromCoordinatorSource();
        RCLCPP_INFO(get_logger(), "Received START command, progression started");
        return;
      case race_interfaces::msg::RaceCommand::STOP:
        coordinator_.stop();
        publishRaceStateFromCoordinatorSource();
        RCLCPP_INFO(get_logger(), "Received STOP command, progression stopped");
        return;
      case race_interfaces::msg::RaceCommand::RESET:
        coordinator_.reset();
        publishRaceStateFromCoordinatorSource();
        RCLCPP_INFO(get_logger(), "Received RESET command, progression reset");
        return;
      default:
        RCLCPP_WARN(get_logger(), "Received unknown race command: %u", command);
        return;
    }
  }

  void onTimer()
  {
    bool any_runtime_advanced = false;

    for (const std::string & vehicle_id : coordinator_.participating_vehicle_ids()) {
      SingleVehicleRuntime & runtime = coordinator_.runtime_for_vehicle(vehicle_id);
      const SingleVehicleTickResult tick_result = runtime.tick();
      if (!tick_result.advanced) {
        continue;
      }

      any_runtime_advanced = true;
      publishVehicleLevelOutputsForTick(vehicle_id, runtime, tick_result);
    }

    if (any_runtime_advanced) {
      publishRaceStateFromCoordinatorSource();
    }
  }

  void onRaceCommand(const race_interfaces::msg::RaceCommand::SharedPtr msg)
  {
    applyRaceWideCommand(msg->command);
  }

  void publishLapEvent(
    const std::string & vehicle_id, const std::int32_t step_sec,
    const ProgressUpdate & progress_update)
  {
    lap_event_publisher_->publish(lap_event_assembler_.assemble(step_sec, vehicle_id, progress_update));
  }

  void publishRaceStateFromCoordinatorSource()
  {
    // RaceCoordinator owns race-wide state meaning; this node only owns ROS publish transport.
    const RaceCoordinator::RaceStateSource race_state_source =
      coordinator_.current_race_state_source();
    race_interfaces::msg::RaceState race_state = race_state_assembler_.assemble(
      race_state_source.race_status, race_state_source.step_sec, race_state_source.primary_snapshot);
    race_state_publisher_->publish(race_state);
  }

  void publishVehicleRaceStatus(
    const std::string & vehicle_id, const std::int32_t step_sec,
    const ProgressUpdate & progress_update)
  {
    status_publisher_->publish(
      vehicle_race_status_assembler_.assemble(step_sec, vehicle_id, progress_update));
  }

  std::int64_t target_lap_count_{2};
  RaceCoordinator coordinator_;
  LapEventAssembler lap_event_assembler_;
  RaceStateAssembler race_state_assembler_;
  VehicleRaceStatusAssembler vehicle_race_status_assembler_;
  rclcpp::Publisher<race_interfaces::msg::VehicleRaceStatus>::SharedPtr status_publisher_;
  rclcpp::Publisher<race_interfaces::msg::LapEvent>::SharedPtr lap_event_publisher_;
  rclcpp::Publisher<race_interfaces::msg::RaceState>::SharedPtr race_state_publisher_;
  rclcpp::Subscription<race_interfaces::msg::RaceCommand>::SharedPtr command_subscriber_;
  rclcpp::TimerBase::SharedPtr timer_;
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
