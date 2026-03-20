#include <gtest/gtest.h>

#include <vector>

#include "race_track/geometry.hpp"

namespace race_track
{
namespace
{

TEST(GeometryTest, FindsNearestCenterlineIndexForRepresentativePoint)
{
  const std::vector<Point2d> centerline = {
    {0.0, 0.0},
    {5.0, 0.0},
    {10.0, 0.0},
    {15.0, 5.0},
  };

  const std::size_t nearest_index = findNearestCenterlineIndex(centerline, Point2d{11.0, 1.0});

  EXPECT_EQ(nearest_index, 2U);
}

TEST(GeometryTest, ComputesMinimumDistanceToCenterlineSegments)
{
  const std::vector<Point2d> centerline = {
    {0.0, 0.0},
    {10.0, 0.0},
    {10.0, 10.0},
  };

  const double distance = distanceToCenterline(centerline, Point2d{8.0, 3.0});

  EXPECT_DOUBLE_EQ(distance, 2.0);
}

TEST(GeometryTest, DetectsForwardCrossingOfStartLine)
{
  const LineSegment2d start_line{{0.0, -2.0}, {0.0, 2.0}};

  EXPECT_TRUE(isForwardCrossingStartLine(
    Point2d{-1.0, 0.0}, Point2d{1.0, 0.0}, start_line, Point2d{1.0, 0.0}));
}

TEST(GeometryTest, ReturnsFalseForReverseCrossing)
{
  const LineSegment2d start_line{{0.0, -2.0}, {0.0, 2.0}};

  EXPECT_FALSE(isForwardCrossingStartLine(
    Point2d{1.0, 0.0}, Point2d{-1.0, 0.0}, start_line, Point2d{1.0, 0.0}));
}

TEST(GeometryTest, ReturnsFalseWhenSegmentDoesNotIntersectStartLine)
{
  const LineSegment2d start_line{{0.0, -2.0}, {0.0, 2.0}};

  EXPECT_FALSE(isForwardCrossingStartLine(
    Point2d{1.0, 3.0}, Point2d{2.0, 3.0}, start_line, Point2d{1.0, 0.0}));
}

}  // namespace
}  // namespace race_track
