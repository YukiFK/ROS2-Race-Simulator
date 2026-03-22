#include <memory>
#include <sstream>
#include <string>

#include "builtin_interfaces/msg/duration.hpp"
#include "race_interfaces/msg/lap_event.hpp"
#include "race_interfaces/msg/race_state.hpp"
#include "race_interfaces/msg/vehicle_race_status.hpp"
#include "rclcpp/rclcpp.hpp"

namespace race_track
{
namespace
{

std::string formatDuration(const builtin_interfaces::msg::Duration & duration)
{
  std::ostringstream stream;
  stream << duration.sec << "." << duration.nanosec;
  return stream.str();
}

class RaceProgressMonitor : public rclcpp::Node
{
public:
  RaceProgressMonitor()
  : Node("race_progress_monitor")
  {
    race_state_subscriber_ = create_subscription<race_interfaces::msg::RaceState>(
      "/race_state", 10,
      std::bind(&RaceProgressMonitor::onRaceState, this, std::placeholders::_1));
    vehicle_status_subscriber_ =
      create_subscription<race_interfaces::msg::VehicleRaceStatus>(
      "/vehicle_race_status", 10,
      std::bind(&RaceProgressMonitor::onVehicleRaceStatus, this, std::placeholders::_1));
    lap_event_subscriber_ = create_subscription<race_interfaces::msg::LapEvent>(
      "/lap_event", 10,
      std::bind(&RaceProgressMonitor::onLapEvent, this, std::placeholders::_1));

    RCLCPP_INFO(
      get_logger(),
      "Monitoring /race_state (race-wide), /vehicle_race_status (vehicle-local), and /lap_event");
  }

private:
  void onRaceState(const race_interfaces::msg::RaceState::SharedPtr msg) const
  {
    RCLCPP_INFO(
      get_logger(), "race_state status=%s elapsed=%s completed_laps_snapshot=%d",
      msg->race_status.c_str(), formatDuration(msg->elapsed_time).c_str(), msg->completed_laps);
  }

  void onVehicleRaceStatus(
    const race_interfaces::msg::VehicleRaceStatus::SharedPtr msg) const
  {
    RCLCPP_INFO(
      get_logger(),
      "vehicle_race_status vehicle_id=%s lap_count=%d has_finished=%s current_lap_time=%s "
      "is_off_track=%s off_track_count=%d",
      msg->vehicle_id.c_str(), msg->lap_count, msg->has_finished ? "true" : "false",
      formatDuration(msg->current_lap_time).c_str(),
      msg->is_off_track ? "true" : "false", msg->off_track_count);
  }

  void onLapEvent(const race_interfaces::msg::LapEvent::SharedPtr msg) const
  {
    RCLCPP_INFO(
      get_logger(), "lap_event vehicle_id=%s lap_count=%d lap_time=%s best_lap_time=%s",
      msg->vehicle_id.c_str(), msg->lap_count, formatDuration(msg->lap_time).c_str(),
      formatDuration(msg->best_lap_time).c_str());
  }

  rclcpp::Subscription<race_interfaces::msg::RaceState>::SharedPtr race_state_subscriber_;
  rclcpp::Subscription<race_interfaces::msg::VehicleRaceStatus>::SharedPtr
    vehicle_status_subscriber_;
  rclcpp::Subscription<race_interfaces::msg::LapEvent>::SharedPtr lap_event_subscriber_;
};

}  // namespace

}  // namespace race_track

int main(int argc, char ** argv)
{
  rclcpp::init(argc, argv);
  auto node = std::make_shared<race_track::RaceProgressMonitor>();
  rclcpp::spin(node);
  rclcpp::shutdown();
  return 0;
}
