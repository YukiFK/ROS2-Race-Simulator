#include <gtest/gtest.h>

#include <array>
#include <string>

#include "race_track/race_coordinator.hpp"

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

TEST(RaceCoordinatorTest, ExposesDefaultParticipatingVehicleIdsAndCount)
{
  RaceCoordinator coordinator(makeTrack());

  ASSERT_EQ(coordinator.vehicle_count(), 2U);
  EXPECT_EQ(
    coordinator.participating_vehicle_ids(),
    (RaceCoordinator::ParticipatingVehicleIds{"demo_vehicle_1", "demo_vehicle_2"}));
}

TEST(RaceCoordinatorTest, OwnsTrackAndSupportsFixedVehicleIdsOverride)
{
  TrackModel track = makeTrack();
  track.track_name = "custom_track";

  RaceCoordinator coordinator(
    track, RaceCoordinator::ParticipatingVehicleIds{"alpha_vehicle", "beta_vehicle"});

  EXPECT_EQ(coordinator.track().track_name, "custom_track");
  EXPECT_EQ(coordinator.track().centerline.size(), 3U);
  EXPECT_EQ(coordinator.participating_vehicle_ids()[0], "alpha_vehicle");
  EXPECT_EQ(coordinator.participating_vehicle_ids()[1], "beta_vehicle");
}

}  // namespace
}  // namespace race_track
