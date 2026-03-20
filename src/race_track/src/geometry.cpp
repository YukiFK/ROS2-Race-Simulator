#include "race_track/geometry.hpp"

#include <algorithm>
#include <cmath>
#include <limits>

namespace race_track
{
namespace
{

constexpr double kEpsilon = 1e-9;

Point2d subtract(const Point2d & a, const Point2d & b)
{
  return Point2d{a.x - b.x, a.y - b.y};
}

double dot(const Point2d & a, const Point2d & b)
{
  return a.x * b.x + a.y * b.y;
}

double cross(const Point2d & a, const Point2d & b)
{
  return a.x * b.y - a.y * b.x;
}

double squaredNorm(const Point2d & p)
{
  return dot(p, p);
}

double squaredDistance(const Point2d & a, const Point2d & b)
{
  return squaredNorm(subtract(a, b));
}

double distancePointToSegment(const Point2d & p, const Point2d & a, const Point2d & b)
{
  const Point2d ab = subtract(b, a);
  const double ab_squared_norm = squaredNorm(ab);

  if (ab_squared_norm <= kEpsilon) {
    return std::sqrt(squaredDistance(p, a));
  }

  const double t = std::clamp(dot(subtract(p, a), ab) / ab_squared_norm, 0.0, 1.0);
  const Point2d projection{a.x + t * ab.x, a.y + t * ab.y};
  return std::sqrt(squaredDistance(p, projection));
}

int orientation(const Point2d & a, const Point2d & b, const Point2d & c)
{
  const double value = cross(subtract(b, a), subtract(c, a));
  if (value > kEpsilon) {
    return 1;
  }
  if (value < -kEpsilon) {
    return -1;
  }
  return 0;
}

bool onSegment(const Point2d & a, const Point2d & b, const Point2d & p)
{
  return p.x >= std::min(a.x, b.x) - kEpsilon &&
         p.x <= std::max(a.x, b.x) + kEpsilon &&
         p.y >= std::min(a.y, b.y) - kEpsilon &&
         p.y <= std::max(a.y, b.y) + kEpsilon;
}

bool segmentsIntersect(const Point2d & a1, const Point2d & a2, const Point2d & b1, const Point2d & b2)
{
  const int o1 = orientation(a1, a2, b1);
  const int o2 = orientation(a1, a2, b2);
  const int o3 = orientation(b1, b2, a1);
  const int o4 = orientation(b1, b2, a2);

  if (o1 != o2 && o3 != o4) {
    return true;
  }

  if (o1 == 0 && onSegment(a1, a2, b1)) {
    return true;
  }
  if (o2 == 0 && onSegment(a1, a2, b2)) {
    return true;
  }
  if (o3 == 0 && onSegment(b1, b2, a1)) {
    return true;
  }
  if (o4 == 0 && onSegment(b1, b2, a2)) {
    return true;
  }

  return false;
}

}  // namespace

std::size_t findNearestCenterlineIndex(const std::vector<Point2d> & centerline, const Point2d & p)
{
  if (centerline.empty()) {
    return 0U;
  }

  std::size_t nearest_index = 0U;
  double min_squared_distance = squaredDistance(centerline.front(), p);

  for (std::size_t i = 1U; i < centerline.size(); ++i) {
    const double current_squared_distance = squaredDistance(centerline[i], p);
    if (current_squared_distance < min_squared_distance) {
      min_squared_distance = current_squared_distance;
      nearest_index = i;
    }
  }

  return nearest_index;
}

double distanceToCenterline(const std::vector<Point2d> & centerline, const Point2d & p)
{
  if (centerline.size() < 2U) {
    return std::numeric_limits<double>::infinity();
  }

  double min_distance = distancePointToSegment(p, centerline[0], centerline[1]);

  for (std::size_t i = 1U; i + 1U < centerline.size(); ++i) {
    const double current_distance = distancePointToSegment(p, centerline[i], centerline[i + 1U]);
    if (current_distance < min_distance) {
      min_distance = current_distance;
    }
  }

  return min_distance;
}

bool isForwardCrossingStartLine(
  const Point2d & prev, const Point2d & curr, const LineSegment2d & start,
  const Point2d & forward_hint)
{
  const Point2d move_vec = subtract(curr, prev);
  if (dot(move_vec, forward_hint) <= 0.0) {
    return false;
  }

  return segmentsIntersect(prev, curr, start.p1, start.p2);
}

}  // namespace race_track
