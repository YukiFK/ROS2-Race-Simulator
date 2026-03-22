#include <gtest/gtest.h>

#include <array>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>

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

RaceCoordinator::VehicleRuntimePositions makeRuntimePositions()
{
  return {{
    {{0.0, 0.0}, {5.0, 0.0}, {10.0, 0.0}},
    {{0.0, 0.5}, {5.0, 0.5}, {10.0, 0.5}},
  }};
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

TEST(RaceCoordinatorTest, DoesNotCreateHiddenDefaultRuntimes)
{
  RaceCoordinator coordinator(makeTrack());

  EXPECT_THROW(static_cast<void>(coordinator.primary_runtime()), std::logic_error);
  EXPECT_THROW(
    static_cast<void>(coordinator.runtime_for_vehicle("demo_vehicle_1")), std::logic_error);
}

TEST(RaceCoordinatorTest, OwnsFixedVehicleRuntimeForEachParticipatingVehicleId)
{
  RaceCoordinator coordinator(
    makeTrack(), RaceCoordinator::ParticipatingVehicleIds{"alpha_vehicle", "beta_vehicle"},
    makeRuntimePositions(), 2);

  SingleVehicleRuntime & alpha_runtime = coordinator.runtime_for_vehicle("alpha_vehicle");
  SingleVehicleRuntime & beta_runtime = coordinator.runtime_for_vehicle("beta_vehicle");

  EXPECT_EQ(&alpha_runtime, &coordinator.primary_runtime());
  EXPECT_NE(&alpha_runtime, &beta_runtime);

  EXPECT_FALSE(alpha_runtime.running());
  EXPECT_FALSE(beta_runtime.running());

  alpha_runtime.start();

  EXPECT_TRUE(alpha_runtime.running());
  EXPECT_FALSE(beta_runtime.running());
}

TEST(RaceCoordinatorTest, RejectsUnknownParticipatingVehicleIdLookup)
{
  RaceCoordinator coordinator(
    makeTrack(), RaceCoordinator::ParticipatingVehicleIds{"alpha_vehicle", "beta_vehicle"},
    makeRuntimePositions(), 2);

  EXPECT_THROW(
    static_cast<void>(coordinator.runtime_for_vehicle("missing_vehicle")), std::out_of_range);
}

}  // namespace
}  // namespace race_track
