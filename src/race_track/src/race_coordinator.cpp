#include "race_track/race_coordinator.hpp"

#include <utility>

namespace race_track
{
namespace
{

RaceCoordinator::ParticipatingVehicleIds makeDefaultParticipatingVehicleIds()
{
  return {"demo_vehicle_1", "demo_vehicle_2"};
}

}  // namespace

RaceCoordinator::RaceCoordinator(TrackModel track)
: RaceCoordinator(std::move(track), makeDefaultParticipatingVehicleIds())
{
}

RaceCoordinator::RaceCoordinator(
  TrackModel track, ParticipatingVehicleIds participating_vehicle_ids)
: track_(std::move(track)),
  participating_vehicle_ids_(std::move(participating_vehicle_ids))
{
}

const RaceCoordinator::ParticipatingVehicleIds & RaceCoordinator::participating_vehicle_ids() const
{
  return participating_vehicle_ids_;
}

std::size_t RaceCoordinator::vehicle_count() const
{
  return participating_vehicle_ids_.size();
}

const TrackModel & RaceCoordinator::track() const
{
  return track_;
}

}  // namespace race_track
