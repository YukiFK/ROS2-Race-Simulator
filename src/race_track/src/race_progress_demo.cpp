#include <filesystem>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

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

    std::size_t lap_count = 0U;

    std::cout << std::boolalpha << std::fixed << std::setprecision(3);
    std::cout << "Loaded track: " << sample_track_path << '\n';
    std::cout << "track_name: " << track.track_name << '\n';
    std::cout << "track_width: " << track.track_width << '\n';
    std::cout << "steps:\n";

    for (std::size_t step_index = 0U; step_index < positions.size(); ++step_index) {
      const race_track::Point2d & current = positions[step_index];
      const std::size_t nearest_index =
        race_track::findNearestCenterlineIndex(track.centerline, current);
      const double distance = race_track::distanceToCenterline(track.centerline, current);
      const bool off_track = distance > (track.track_width / 2.0);

      bool crossing_detected = false;
      if (step_index > 0U) {
        crossing_detected = race_track::isForwardCrossingStartLine(
          positions[step_index - 1U], current, track.start_line, track.forward_hint);
        if (crossing_detected) {
          ++lap_count;
        }
      }

      std::cout << "step " << step_index
                << " | pos=(" << current.x << ", " << current.y << ")"
                << " | nearest_index=" << nearest_index
                << " | distance=" << distance
                << " | crossing_detected=" << crossing_detected
                << " | lap_count=" << lap_count
                << " | off_track=" << off_track << '\n';
    }

    return 0;
  } catch (const std::exception & ex) {
    std::cerr << "race_progress_demo failed: " << ex.what() << '\n';
    return 1;
  }
}
