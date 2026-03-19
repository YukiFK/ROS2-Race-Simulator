#ifndef RACE_TRACK__TRACK_MODEL_HPP_
#define RACE_TRACK__TRACK_MODEL_HPP_

#include <string>
#include <vector>

#include "race_track/types.hpp"

namespace race_track
{

struct TrackModel
{
  std::string track_name{};
  std::vector<Point2d> centerline{};
  double track_width{0.0};
  LineSegment2d start_line{};
  Point2d forward_hint{};
};

}  // namespace race_track

#endif  // RACE_TRACK__TRACK_MODEL_HPP_
