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

    const race_track::Point2d query_point{8.0, 3.0};
    const std::size_t nearest_index =
      race_track::findNearestCenterlineIndex(track.centerline, query_point);
    const double distance = race_track::distanceToCenterline(track.centerline, query_point);

    const bool forward_crossing = race_track::isForwardCrossingStartLine(
      race_track::Point2d{-1.0, 0.0}, race_track::Point2d{1.0, 0.0}, track.start_line,
      track.forward_hint);
    const bool reverse_crossing = race_track::isForwardCrossingStartLine(
      race_track::Point2d{1.0, 0.0}, race_track::Point2d{-1.0, 0.0}, track.start_line,
      track.forward_hint);

    std::cout << std::boolalpha << std::fixed << std::setprecision(3);
    std::cout << "Loaded track: " << sample_track_path << '\n';
    std::cout << "track_name: " << track.track_name << '\n';
    std::cout << "centerline_points: " << track.centerline.size() << '\n';
    std::cout << "track_width: " << track.track_width << '\n';
    std::cout << "query_point: (" << query_point.x << ", " << query_point.y << ")\n";
    std::cout << "nearest_centerline_index: " << nearest_index << '\n';
    std::cout << "distance_to_centerline: " << distance << '\n';
    std::cout << "forward_crossing_case: " << forward_crossing << '\n';
    std::cout << "reverse_crossing_case: " << reverse_crossing << '\n';
    return 0;
  } catch (const std::exception & ex) {
    std::cerr << "track_demo failed: " << ex.what() << '\n';
    return 1;
  }
}
