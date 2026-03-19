#ifndef RACE_TRACK__TYPES_HPP_
#define RACE_TRACK__TYPES_HPP_

namespace race_track
{

struct Point2d
{
  double x{0.0};
  double y{0.0};
};

struct LineSegment2d
{
  Point2d p1{};
  Point2d p2{};
};

}  // namespace race_track

#endif  // RACE_TRACK__TYPES_HPP_
