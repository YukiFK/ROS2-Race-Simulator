#include <gtest/gtest.h>

#include <vector>

#include "race_track/single_vehicle_runtime.hpp"

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

std::vector<Point2d> makePositions()
{
  return {{-1.0, 0.0}, {1.0, 0.0}, {-1.0, 0.0}, {1.0, 0.0}, {10.0, 0.0}};
}

TEST(SingleVehicleRuntimeTest, TickAdvancesAndCompletesWhenTargetLapCountReached)
{
  SingleVehicleRuntime runtime(makeTrack(), makePositions(), 2);

  runtime.start();

  const SingleVehicleTickResult first = runtime.tick();
  EXPECT_TRUE(first.advanced);
  EXPECT_EQ(first.step_sec, 0);
  EXPECT_FALSE(first.crossing_detected);
  EXPECT_TRUE(runtime.running());
  EXPECT_FALSE(runtime.completed());

  const SingleVehicleTickResult second = runtime.tick();
  EXPECT_TRUE(second.crossing_detected);
  EXPECT_EQ(second.progress_update.snapshot.lap_count, 1);
  EXPECT_TRUE(runtime.running());
  EXPECT_FALSE(runtime.completed());

  const SingleVehicleTickResult third = runtime.tick();
  EXPECT_TRUE(third.advanced);
  EXPECT_FALSE(third.crossing_detected);

  const SingleVehicleTickResult fourth = runtime.tick();
  EXPECT_TRUE(fourth.advanced);
  EXPECT_TRUE(fourth.crossing_detected);
  EXPECT_TRUE(fourth.just_completed);
  EXPECT_FALSE(fourth.stopped_without_completion);
  EXPECT_TRUE(fourth.progress_update.snapshot.has_finished);
  EXPECT_FALSE(runtime.running());
  EXPECT_TRUE(runtime.completed());
  EXPECT_EQ(runtime.current_race_status(), "completed");
}

TEST(SingleVehicleRuntimeTest, StopClearsRunningWithoutResettingProgress)
{
  SingleVehicleRuntime runtime(makeTrack(), makePositions(), 2);

  runtime.start();
  runtime.tick();
  runtime.tick();

  runtime.stop();

  EXPECT_FALSE(runtime.running());
  EXPECT_FALSE(runtime.completed());
  EXPECT_EQ(runtime.step_index(), 2U);
  EXPECT_EQ(runtime.snapshot().lap_count, 1);
  EXPECT_EQ(runtime.current_race_status(), "stopped");
}

TEST(SingleVehicleRuntimeTest, StartAfterCompletionResetsBeforeRunningAgain)
{
  SingleVehicleRuntime runtime(makeTrack(), makePositions(), 1);

  runtime.start();
  runtime.tick();
  runtime.tick();

  ASSERT_TRUE(runtime.completed());
  ASSERT_EQ(runtime.snapshot().lap_count, 1);

  runtime.start();

  EXPECT_TRUE(runtime.running());
  EXPECT_FALSE(runtime.completed());
  EXPECT_EQ(runtime.step_index(), 0U);
  EXPECT_EQ(runtime.snapshot().lap_count, 0);
  EXPECT_EQ(runtime.current_race_status(), "running");
}

TEST(SingleVehicleRuntimeTest, FinalStepStopsWithoutCompletion)
{
  SingleVehicleRuntime runtime(makeTrack(), makePositions(), 3);

  runtime.start();

  SingleVehicleTickResult result;
  for (std::size_t i = 0; i < makePositions().size(); ++i) {
    result = runtime.tick();
  }

  EXPECT_TRUE(result.advanced);
  EXPECT_TRUE(result.stopped_without_completion);
  EXPECT_FALSE(result.just_completed);
  EXPECT_FALSE(runtime.running());
  EXPECT_FALSE(runtime.completed());
  EXPECT_EQ(runtime.step_index(), makePositions().size());
}

}  // namespace
}  // namespace race_track
