#include <gtest/gtest.h>

#include "race_track/progress_tracker.hpp"

namespace race_track
{
namespace
{

TrackModel makeTrack()
{
  TrackModel track;
  track.track_name = "unit_test_track";
  track.centerline = {{0.0, 0.0}, {5.0, 0.0}, {10.0, 0.0}};
  track.track_width = 4.0;
  track.start_line = {{0.0, -2.0}, {0.0, 2.0}};
  track.forward_hint = {1.0, 0.0};
  return track;
}

TEST(ProgressTrackerTest, UpdatesLapTimingAndOffTrackCount)
{
  ProgressTracker tracker(makeTrack());

  const ProgressUpdate first = tracker.update(0, Point2d{-1.0, 0.0});
  EXPECT_FALSE(first.crossing_detected);
  EXPECT_FALSE(first.is_off_track);
  EXPECT_EQ(first.snapshot.lap_count, 0);
  EXPECT_EQ(first.snapshot.off_track_count, 0);

  const ProgressUpdate second = tracker.update(1, Point2d{1.0, 0.0});
  EXPECT_TRUE(second.crossing_detected);
  EXPECT_EQ(second.crossed_lap_time_sec, 1);
  EXPECT_EQ(second.snapshot.lap_count, 1);
  EXPECT_EQ(second.snapshot.last_lap_time_sec, 1);
  EXPECT_EQ(second.snapshot.best_lap_time_sec, 1);
  EXPECT_EQ(second.snapshot.lap_start_step_sec, 1);
  EXPECT_FALSE(second.snapshot.has_finished);

  const ProgressUpdate third = tracker.update(2, Point2d{10.0, 3.5});
  EXPECT_FALSE(third.crossing_detected);
  EXPECT_TRUE(third.is_off_track);
  EXPECT_EQ(third.snapshot.off_track_count, 1);

  const ProgressUpdate fourth = tracker.update(3, Point2d{-1.0, 0.0});
  EXPECT_FALSE(fourth.crossing_detected);
  EXPECT_EQ(fourth.snapshot.lap_count, 1);

  const ProgressUpdate fifth = tracker.update(4, Point2d{1.0, 0.0});
  EXPECT_TRUE(fifth.crossing_detected);
  EXPECT_EQ(fifth.crossed_lap_time_sec, 3);
  EXPECT_EQ(fifth.snapshot.lap_count, 2);
  EXPECT_EQ(fifth.snapshot.last_lap_time_sec, 3);
  EXPECT_EQ(fifth.snapshot.best_lap_time_sec, 1);
  EXPECT_FALSE(fifth.snapshot.has_finished);
}

TEST(ProgressTrackerTest, SetHasFinishedUpdatesSnapshotAndResetClearsIt)
{
  ProgressTracker tracker(makeTrack());

  tracker.update(0, Point2d{-1.0, 0.0});
  tracker.setHasFinished(true);

  const ProgressSnapshot finished_snapshot = tracker.snapshot();
  EXPECT_TRUE(finished_snapshot.has_finished);

  const ProgressUpdate after_finish = tracker.update(1, Point2d{1.0, 0.0});
  EXPECT_TRUE(after_finish.snapshot.has_finished);
}

TEST(ProgressTrackerTest, ResetClearsProgressState)
{
  ProgressTracker tracker(makeTrack());

  tracker.update(0, Point2d{-1.0, 0.0});
  tracker.update(1, Point2d{1.0, 0.0});
  tracker.update(2, Point2d{10.0, 3.5});
  tracker.reset();

  const ProgressSnapshot snapshot = tracker.snapshot();
  EXPECT_EQ(snapshot.lap_count, 0);
  EXPECT_EQ(snapshot.off_track_count, 0);
  EXPECT_EQ(snapshot.lap_start_step_sec, 0);
  EXPECT_EQ(snapshot.last_lap_time_sec, 0);
  EXPECT_EQ(snapshot.best_lap_time_sec, 0);
  EXPECT_FALSE(snapshot.has_finished);

  const ProgressUpdate after_reset = tracker.update(5, Point2d{1.0, 0.0});
  EXPECT_FALSE(after_reset.crossing_detected);
  EXPECT_EQ(after_reset.snapshot.lap_count, 0);
  EXPECT_EQ(after_reset.snapshot.off_track_count, 0);
}

}  // namespace
}  // namespace race_track
