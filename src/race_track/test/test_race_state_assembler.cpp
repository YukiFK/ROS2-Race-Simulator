#include <gtest/gtest.h>

#include "race_track/progress_tracker.hpp"
#include "race_track/race_state_assembler.hpp"

namespace race_track
{
namespace
{

TEST(RaceStateAssemblerTest, AssemblesRaceStateFromMinimalInputs)
{
  RaceStateAssembler assembler;
  ProgressSnapshot snapshot;
  snapshot.lap_count = 3;
  snapshot.off_track_count = 2;
  snapshot.has_finished = true;

  const auto race_state = assembler.assemble("completed", 17, snapshot);

  EXPECT_EQ(race_state.header.stamp.sec, 17);
  EXPECT_EQ(race_state.header.stamp.nanosec, 0U);
  EXPECT_EQ(race_state.header.frame_id, "map");
  EXPECT_EQ(race_state.race_status, "completed");
  EXPECT_EQ(race_state.elapsed_time.sec, 17);
  EXPECT_EQ(race_state.elapsed_time.nanosec, 0U);
  EXPECT_EQ(race_state.completed_laps, 3);
}

}  // namespace
}  // namespace race_track
