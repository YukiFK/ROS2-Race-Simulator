#ifndef RACE_TRACK__GEOMETRY_HPP_
#define RACE_TRACK__GEOMETRY_HPP_

#include <cstddef>
#include <vector>

#include "race_track/types.hpp"

namespace race_track
{

std::size_t findNearestCenterlineIndex(const std::vector<Point2d> & centerline, const Point2d & p);

double distanceToCenterline(const std::vector<Point2d> & centerline, const Point2d & p);

bool isForwardCrossingStartLine(
  const Point2d & prev, const Point2d & curr, const LineSegment2d & start,
  const Point2d & forward_hint);

}  // namespace race_track

#endif  // RACE_TRACK__GEOMETRY_HPP_
