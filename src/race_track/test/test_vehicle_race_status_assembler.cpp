#include <gtest/gtest.h>

#include "race_track/progress_tracker.hpp"
#include "race_track/vehicle_race_status_assembler.hpp"

namespace race_track
{
namespace
{

TEST(VehicleRaceStatusAssemblerTest, AssemblesVehicleRaceStatusFromMinimalInputs)
{
  VehicleRaceStatusAssembler assembler;
  ProgressUpdate progress_update;
  progress_update.snapshot.lap_count = 2;
  progress_update.snapshot.off_track_count = 3;
  progress_update.snapshot.lap_start_step_sec = 8;
  progress_update.snapshot.last_lap_time_sec = 11;
  progress_update.snapshot.best_lap_time_sec = 9;
  progress_update.snapshot.has_finished = true;
  progress_update.is_off_track = true;

  const auto status = assembler.assemble(17, "demo_vehicle_1", progress_update);

  EXPECT_EQ(status.header.stamp.sec, 17);
  EXPECT_EQ(status.header.stamp.nanosec, 0U);
  EXPECT_EQ(status.header.frame_id, "map");
  EXPECT_EQ(status.vehicle_id, "demo_vehicle_1");
  EXPECT_EQ(status.lap_count, 2);
  EXPECT_EQ(status.current_lap_time.sec, 9);
  EXPECT_EQ(status.current_lap_time.nanosec, 0U);
  EXPECT_EQ(status.last_lap_time.sec, 11);
  EXPECT_EQ(status.last_lap_time.nanosec, 0U);
  EXPECT_EQ(status.best_lap_time.sec, 9);
  EXPECT_EQ(status.best_lap_time.nanosec, 0U);
  EXPECT_EQ(status.total_elapsed_time.sec, 17);
  EXPECT_EQ(status.total_elapsed_time.nanosec, 0U);
  EXPECT_TRUE(status.has_finished);
  EXPECT_TRUE(status.is_off_track);
  EXPECT_EQ(status.off_track_count, 3);
}

}  // namespace
}  // namespace race_track
