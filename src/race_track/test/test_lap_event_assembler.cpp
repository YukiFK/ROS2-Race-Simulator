#include <gtest/gtest.h>

#include "race_track/lap_event_assembler.hpp"
#include "race_track/progress_tracker.hpp"

namespace race_track
{
namespace
{

TEST(LapEventAssemblerTest, AssemblesLapEventFromMinimalInputs)
{
  LapEventAssembler assembler;
  ProgressUpdate progress_update;
  progress_update.snapshot.lap_count = 2;
  progress_update.snapshot.best_lap_time_sec = 9;
  progress_update.snapshot.has_finished = true;
  progress_update.crossed_lap_time_sec = 11;

  const auto lap_event = assembler.assemble(17, "demo_vehicle_1", progress_update);

  EXPECT_EQ(lap_event.header.stamp.sec, 17);
  EXPECT_EQ(lap_event.header.stamp.nanosec, 0U);
  EXPECT_EQ(lap_event.header.frame_id, "map");
  EXPECT_EQ(lap_event.vehicle_id, "demo_vehicle_1");
  EXPECT_EQ(lap_event.lap_count, 2);
  EXPECT_EQ(lap_event.lap_time.sec, 11);
  EXPECT_EQ(lap_event.lap_time.nanosec, 0U);
  EXPECT_EQ(lap_event.best_lap_time.sec, 9);
  EXPECT_EQ(lap_event.best_lap_time.nanosec, 0U);
  EXPECT_TRUE(lap_event.has_finished);
}

}  // namespace
}  // namespace race_track
