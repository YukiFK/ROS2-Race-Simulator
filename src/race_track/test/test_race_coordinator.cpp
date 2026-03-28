#include <gtest/gtest.h>

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
  return {
    {{0.0, 0.0}, {5.0, 0.0}, {10.0, 0.0}},
    {{0.0, 0.5}, {5.0, 0.5}, {10.0, 0.5}},
  };
}

RaceCoordinator::VehicleRuntimePositions makeStaggeredRuntimePositions()
{
  return {
    {{-1.0, 0.0}, {1.0, 0.0}, {5.0, 0.0}},
    {{-1.0, 0.5}, {-0.5, 0.5}, {-0.2, 0.5}, {1.0, 0.5}, {5.0, 0.5}},
  };
}

RaceCoordinator::VehicleRuntimePositions makeThreeVehicleRuntimePositions()
{
  return {
    {{0.0, 0.0}, {5.0, 0.0}, {10.0, 0.0}},
    {{0.0, 0.5}, {5.0, 0.5}, {10.0, 0.5}},
    {{0.0, 1.0}, {5.0, 1.0}, {10.0, 1.0}},
  };
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

TEST(RaceCoordinatorTest, IdsOnlyConstructorRejectsNonDemoVehicleCount)
{
  EXPECT_THROW(
    static_cast<void>(RaceCoordinator(
      makeTrack(),
      RaceCoordinator::ParticipatingVehicleIds{
        "alpha_vehicle", "beta_vehicle", "gamma_vehicle"})),
    std::invalid_argument);
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
  EXPECT_EQ(&alpha_runtime, &coordinator.runtime_at(0U));
  EXPECT_EQ(&beta_runtime, &coordinator.runtime_at(1U));
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

  EXPECT_THROW(static_cast<void>(coordinator.runtime_at(2U)), std::out_of_range);
  EXPECT_THROW(
    static_cast<void>(coordinator.runtime_for_vehicle("missing_vehicle")), std::out_of_range);
}

TEST(RaceCoordinatorTest, SupportsVariableLengthParticipatingVehicleSets)
{
  RaceCoordinator coordinator(
    makeTrack(),
    RaceCoordinator::ParticipatingVehicleIds{"alpha_vehicle", "beta_vehicle", "gamma_vehicle"},
    makeThreeVehicleRuntimePositions(), 2);

  ASSERT_EQ(coordinator.vehicle_count(), 3U);
  EXPECT_EQ(coordinator.runtime_at(2U).step_index(), 0U);
  EXPECT_EQ(&coordinator.runtime_for_vehicle("gamma_vehicle"), &coordinator.runtime_at(2U));

  EXPECT_FALSE(coordinator.start());
  EXPECT_TRUE(coordinator.runtime_for_vehicle("alpha_vehicle").running());
  EXPECT_TRUE(coordinator.runtime_for_vehicle("beta_vehicle").running());
  EXPECT_TRUE(coordinator.runtime_for_vehicle("gamma_vehicle").running());
}

TEST(RaceCoordinatorTest, RejectsEmptyParticipatingVehicleSetWhenCreatingRuntimes)
{
  EXPECT_THROW(
    static_cast<void>(RaceCoordinator(
      makeTrack(), RaceCoordinator::ParticipatingVehicleIds{}, RaceCoordinator::VehicleRuntimePositions{},
      2)),
    std::invalid_argument);
}

TEST(RaceCoordinatorTest, RejectsParticipatingVehicleIdAndRuntimePositionSizeMismatch)
{
  EXPECT_THROW(
    static_cast<void>(RaceCoordinator(
      makeTrack(),
      RaceCoordinator::ParticipatingVehicleIds{"alpha_vehicle", "beta_vehicle", "gamma_vehicle"},
      makeRuntimePositions(), 2)),
    std::invalid_argument);
}

TEST(RaceCoordinatorTest, AppliesGlobalCommandsToBothParticipatingRuntimes)
{
  RaceCoordinator coordinator(
    makeTrack(), RaceCoordinator::ParticipatingVehicleIds{"alpha_vehicle", "beta_vehicle"},
    makeRuntimePositions(), 2);

  SingleVehicleRuntime & alpha_runtime = coordinator.runtime_for_vehicle("alpha_vehicle");
  SingleVehicleRuntime & beta_runtime = coordinator.runtime_for_vehicle("beta_vehicle");

  EXPECT_FALSE(coordinator.start());
  EXPECT_TRUE(alpha_runtime.running());
  EXPECT_TRUE(beta_runtime.running());

  static_cast<void>(alpha_runtime.tick());
  static_cast<void>(beta_runtime.tick());
  EXPECT_EQ(alpha_runtime.step_index(), 1U);
  EXPECT_EQ(beta_runtime.step_index(), 1U);

  EXPECT_TRUE(coordinator.stop());
  EXPECT_FALSE(alpha_runtime.running());
  EXPECT_FALSE(beta_runtime.running());

  coordinator.reset();
  EXPECT_FALSE(alpha_runtime.running());
  EXPECT_FALSE(beta_runtime.running());
  EXPECT_EQ(alpha_runtime.step_index(), 0U);
  EXPECT_EQ(beta_runtime.step_index(), 0U);
}

TEST(RaceCoordinatorTest, RequiresAllParticipatingVehiclesToFinishForRaceCompletion)
{
  RaceCoordinator coordinator(
    makeTrack(), RaceCoordinator::ParticipatingVehicleIds{"alpha_vehicle", "beta_vehicle"},
    makeStaggeredRuntimePositions(), 1);

  EXPECT_FALSE(coordinator.all_participating_vehicles_finished());
  EXPECT_EQ(coordinator.current_race_status(), "stopped");

  static_cast<void>(coordinator.start());
  EXPECT_EQ(coordinator.current_race_status(), "running");

  static_cast<void>(coordinator.runtime_for_vehicle("alpha_vehicle").tick());
  static_cast<void>(coordinator.runtime_for_vehicle("beta_vehicle").tick());
  EXPECT_FALSE(coordinator.all_participating_vehicles_finished());
  EXPECT_EQ(coordinator.current_race_status(), "running");

  static_cast<void>(coordinator.runtime_for_vehicle("alpha_vehicle").tick());
  static_cast<void>(coordinator.runtime_for_vehicle("beta_vehicle").tick());
  EXPECT_FALSE(coordinator.runtime_for_vehicle("alpha_vehicle").running());
  EXPECT_TRUE(coordinator.runtime_for_vehicle("alpha_vehicle").snapshot().has_finished);
  EXPECT_FALSE(coordinator.all_participating_vehicles_finished());
  EXPECT_EQ(coordinator.current_race_status(), "running");

  static_cast<void>(coordinator.runtime_for_vehicle("beta_vehicle").tick());
  EXPECT_FALSE(coordinator.all_participating_vehicles_finished());
  EXPECT_EQ(coordinator.current_race_status(), "running");

  static_cast<void>(coordinator.runtime_for_vehicle("beta_vehicle").tick());
  EXPECT_TRUE(coordinator.all_participating_vehicles_finished());
  EXPECT_EQ(coordinator.current_race_status(), "completed");
}

TEST(RaceCoordinatorTest, ExposesRaceWideStateSourceWithoutOwningRosPublish)
{
  RaceCoordinator coordinator(
    makeTrack(), RaceCoordinator::ParticipatingVehicleIds{"alpha_vehicle", "beta_vehicle"},
    makeStaggeredRuntimePositions(), 1);

  RaceCoordinator::RaceStateSource initial_state = coordinator.current_race_state_source();
  EXPECT_EQ(initial_state.race_status, "stopped");
  EXPECT_EQ(initial_state.step_sec, 0);
  EXPECT_EQ(initial_state.primary_snapshot.lap_count, 0);

  static_cast<void>(coordinator.start());
  static_cast<void>(coordinator.runtime_for_vehicle("alpha_vehicle").tick());

  RaceCoordinator::RaceStateSource running_state = coordinator.current_race_state_source();
  EXPECT_EQ(running_state.race_status, "running");
  EXPECT_EQ(running_state.step_sec, coordinator.current_step_sec());
  EXPECT_EQ(running_state.primary_snapshot.lap_count, coordinator.primary_snapshot().lap_count);
  EXPECT_FALSE(running_state.primary_snapshot.has_finished);
}

}  // namespace
}  // namespace race_track
