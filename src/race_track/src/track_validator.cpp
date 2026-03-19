#include "race_track/track_validator.hpp"

#include <sstream>
#include <stdexcept>

namespace race_track
{
namespace
{

bool isZeroVector(const Point2d & point)
{
  return point.x == 0.0 && point.y == 0.0;
}

bool isZeroLengthSegment(const LineSegment2d & segment)
{
  return segment.p1.x == segment.p2.x && segment.p1.y == segment.p2.y;
}

std::string formatPoint(const Point2d & point)
{
  std::ostringstream oss;
  oss << "(" << point.x << ", " << point.y << ")";
  return oss.str();
}

}  // namespace

void validateTrackOrThrow(const TrackModel & track)
{
  if (track.track_name.empty()) {
    throw std::runtime_error("Invalid track: 'track_name' must not be empty");
  }

  if (track.centerline.size() < 3U) {
    throw std::runtime_error(
            "Invalid track '" + track.track_name + "': 'centerline' must contain at least 3 points, got " +
            std::to_string(track.centerline.size()));
  }

  if (track.track_width <= 0.0) {
    throw std::runtime_error(
            "Invalid track '" + track.track_name + "': 'track_width' must be greater than 0, got " +
            std::to_string(track.track_width));
  }

  if (isZeroLengthSegment(track.start_line)) {
    throw std::runtime_error(
            "Invalid track '" + track.track_name +
            "': 'start_line' must not be zero-length; p1=" + formatPoint(track.start_line.p1) +
            ", p2=" + formatPoint(track.start_line.p2));
  }

  if (isZeroVector(track.forward_hint)) {
    throw std::runtime_error(
            "Invalid track '" + track.track_name +
            "': 'forward_hint' must not be the zero vector; got " + formatPoint(track.forward_hint));
  }
}

}  // namespace race_track
