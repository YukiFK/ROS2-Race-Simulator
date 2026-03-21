#include <cstdint>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

#include "builtin_interfaces/msg/duration.hpp"
#include "race_interfaces/msg/lap_event.hpp"
#include "race_interfaces/msg/vehicle_race_status.hpp"
#include "race_track/geometry.hpp"
#include "race_track/track_loader.hpp"
#include "race_track/track_validator.hpp"

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

builtin_interfaces::msg::Duration makeDuration(const std::int32_t seconds)
{
  builtin_interfaces::msg::Duration duration;
  duration.sec = seconds;
  duration.nanosec = 0U;
  return duration;
}

void printDuration(
  const std::string & label, const builtin_interfaces::msg::Duration & duration)
{
  std::cout << "    " << label << ": " << duration.sec << "." << std::setw(9) << std::setfill('0')
            << duration.nanosec << std::setfill(' ') << '\n';
}

void printVehicleRaceStatus(const race_interfaces::msg::VehicleRaceStatus & status)
{
  std::cout << "  VehicleRaceStatus\n";
  std::cout << "    header.frame_id: " << status.header.frame_id << '\n';
  std::cout << "    header.stamp.sec: " << status.header.stamp.sec << '\n';
  std::cout << "    header.stamp.nanosec: " << status.header.stamp.nanosec << '\n';
  std::cout << "    vehicle_id: " << status.vehicle_id << '\n';
  std::cout << "    lap_count: " << status.lap_count << '\n';
  printDuration("current_lap_time", status.current_lap_time);
  printDuration("last_lap_time", status.last_lap_time);
  printDuration("best_lap_time", status.best_lap_time);
  printDuration("total_elapsed_time", status.total_elapsed_time);
  std::cout << "    has_finished: " << status.has_finished << '\n';
  std::cout << "    is_off_track: " << status.is_off_track << '\n';
  std::cout << "    off_track_count: " << status.off_track_count << '\n';
}

void printLapEvent(const race_interfaces::msg::LapEvent & event)
{
  std::cout << "  LapEvent\n";
  std::cout << "    header.frame_id: " << event.header.frame_id << '\n';
  std::cout << "    header.stamp.sec: " << event.header.stamp.sec << '\n';
  std::cout << "    header.stamp.nanosec: " << event.header.stamp.nanosec << '\n';
  std::cout << "    vehicle_id: " << event.vehicle_id << '\n';
  std::cout << "    lap_count: " << event.lap_count << '\n';
  printDuration("lap_time", event.lap_time);
  printDuration("best_lap_time", event.best_lap_time);
  std::cout << "    has_finished: " << event.has_finished << '\n';
}

}  // namespace

}  // namespace race_track

int main(int argc, char ** argv)
{
  (void)argc;

  try {
    const std::filesystem::path sample_track_path = race_track::resolveSampleTrackPath(argv[0]);
    const race_track::TrackModel track = race_track::loadTrackFromYaml(sample_track_path.string());
    race_track::validateTrackOrThrow(track);

    const std::vector<race_track::Point2d> positions = {
      {-2.0, 0.0},
      {-0.5, 0.2},
      {1.0, 0.2},
      {6.0, 0.1},
      {11.0, 0.4},
      {18.0, 4.8},
      {9.0, 5.0},
      {0.5, 0.0},
      {-1.0, 0.0},
      {1.5, -0.1},
      {4.0, 4.0},
    };

    constexpr char kVehicleId[] = "demo_vehicle_1";
    std::int32_t lap_count = 0;
    std::int32_t off_track_count = 0;
    std::int32_t lap_start_step = 0;
    std::int32_t last_lap_time_sec = 0;
    std::int32_t best_lap_time_sec = 0;
    std::int32_t best_lap_time_candidate = std::numeric_limits<std::int32_t>::max();

    std::cout << std::boolalpha << std::fixed << std::setprecision(3);
    std::cout << "Loaded track: " << sample_track_path << '\n';
    std::cout << "track_name: " << track.track_name << '\n';
    std::cout << "track_width: " << track.track_width << '\n';
    std::cout << "steps:\n";

    for (std::size_t step_index = 0U; step_index < positions.size(); ++step_index) {
      const std::int32_t step_sec = static_cast<std::int32_t>(step_index);
      const race_track::Point2d & current = positions[step_index];
      const std::size_t nearest_index =
        race_track::findNearestCenterlineIndex(track.centerline, current);
      const double distance = race_track::distanceToCenterline(track.centerline, current);
      const bool is_off_track = distance > (track.track_width / 2.0);
      if (is_off_track) {
        ++off_track_count;
      }

      bool crossing_detected = false;
      race_interfaces::msg::LapEvent lap_event;
      if (step_index > 0U) {
        crossing_detected = race_track::isForwardCrossingStartLine(
          positions[step_index - 1U], current, track.start_line, track.forward_hint);
        if (crossing_detected) {
          const std::int32_t lap_time_sec = step_sec - lap_start_step;
          ++lap_count;
          last_lap_time_sec = lap_time_sec;
          if (lap_time_sec < best_lap_time_candidate) {
            best_lap_time_candidate = lap_time_sec;
            best_lap_time_sec = lap_time_sec;
          }
          lap_start_step = step_sec;

          lap_event.header.frame_id = "map";
          lap_event.header.stamp.sec = step_sec;
          lap_event.header.stamp.nanosec = 0U;
          lap_event.vehicle_id = kVehicleId;
          lap_event.lap_count = lap_count;
          lap_event.lap_time = race_track::makeDuration(lap_time_sec);
          lap_event.best_lap_time = race_track::makeDuration(best_lap_time_sec);
          lap_event.has_finished = false;
        }
      }

      race_interfaces::msg::VehicleRaceStatus status;
      status.header.frame_id = "map";
      status.header.stamp.sec = step_sec;
      status.header.stamp.nanosec = 0U;
      status.vehicle_id = kVehicleId;
      status.lap_count = lap_count;
      status.current_lap_time = race_track::makeDuration(step_sec - lap_start_step);
      status.last_lap_time = race_track::makeDuration(last_lap_time_sec);
      status.best_lap_time = race_track::makeDuration(best_lap_time_sec);
      status.total_elapsed_time = race_track::makeDuration(step_sec);
      status.has_finished = false;
      status.is_off_track = is_off_track;
      status.off_track_count = off_track_count;

      std::cout << "step " << step_index << '\n';
      std::cout << "  position: (" << current.x << ", " << current.y << ")\n";
      std::cout << "  nearest_centerline_index: " << nearest_index << '\n';
      std::cout << "  distance_to_centerline: " << distance << '\n';
      std::cout << "  crossing_detected: " << crossing_detected << '\n';
      race_track::printVehicleRaceStatus(status);
      if (crossing_detected) {
        race_track::printLapEvent(lap_event);
      }
    }

    return 0;
  } catch (const std::exception & ex) {
    std::cerr << "race_message_demo failed: " << ex.what() << '\n';
    return 1;
  }
}
