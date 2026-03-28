#include "race_track/race_coordinator.hpp"

#include <algorithm>
#include <stdexcept>
#include <utility>

namespace race_track
{
namespace
{

RaceCoordinator::ParticipatingVehicleIds makeDefaultParticipatingVehicleIds()
{
  return {"demo_vehicle_1", "demo_vehicle_2"};
}

RaceCoordinator::VehicleRuntimes makeVehicleRuntimes(
  const TrackModel & track,
  const RaceCoordinator::VehicleRuntimePositions & runtime_positions,
  const std::int64_t target_lap_count)
{
  return {
    SingleVehicleRuntime(track, runtime_positions[0], target_lap_count),
    SingleVehicleRuntime(track, runtime_positions[1], target_lap_count),
  };
}

std::size_t findVehicleIndex(
  const RaceCoordinator::ParticipatingVehicleIds & participating_vehicle_ids,
  const std::string & vehicle_id)
{
  for (std::size_t index = 0; index < participating_vehicle_ids.size(); ++index) {
    if (participating_vehicle_ids[index] == vehicle_id) {
      return index;
    }
  }

  throw std::out_of_range("Unknown participating vehicle id: " + vehicle_id);
}

const RaceCoordinator::VehicleRuntimes & requireRuntimes(
  const std::optional<RaceCoordinator::VehicleRuntimes> & runtimes)
{
  if (!runtimes.has_value()) {
    throw std::logic_error("RaceCoordinator runtimes were not initialized");
  }

  return *runtimes;
}

RaceCoordinator::VehicleRuntimes & requireRuntimes(
  std::optional<RaceCoordinator::VehicleRuntimes> & runtimes)
{
  if (!runtimes.has_value()) {
    throw std::logic_error("RaceCoordinator runtimes were not initialized");
  }

  return *runtimes;
}

}  // namespace

RaceCoordinator::RaceCoordinator(TrackModel track)
: track_(std::move(track)),
  participating_vehicle_ids_(makeDefaultParticipatingVehicleIds())
{
}

RaceCoordinator::RaceCoordinator(
  TrackModel track, ParticipatingVehicleIds participating_vehicle_ids)
: track_(std::move(track)),
  participating_vehicle_ids_(std::move(participating_vehicle_ids))
{
}

RaceCoordinator::RaceCoordinator(
  TrackModel track, VehicleRuntimePositions runtime_positions, const std::int64_t target_lap_count)
: RaceCoordinator(
    std::move(track), makeDefaultParticipatingVehicleIds(), std::move(runtime_positions),
    target_lap_count)
{
}

RaceCoordinator::RaceCoordinator(
  TrackModel track, ParticipatingVehicleIds participating_vehicle_ids,
  VehicleRuntimePositions runtime_positions, const std::int64_t target_lap_count)
: track_(std::move(track)),
  participating_vehicle_ids_(std::move(participating_vehicle_ids)),
  runtimes_(makeVehicleRuntimes(track_, runtime_positions, target_lap_count))
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

const SingleVehicleRuntime & RaceCoordinator::primary_runtime() const
{
  return requireRuntimes(runtimes_).front();
}

SingleVehicleRuntime & RaceCoordinator::primary_runtime()
{
  return requireRuntimes(runtimes_).front();
}

const SingleVehicleRuntime & RaceCoordinator::runtime_at(const std::size_t vehicle_index) const
{
  return requireRuntimes(runtimes_).at(vehicle_index);
}

SingleVehicleRuntime & RaceCoordinator::runtime_at(const std::size_t vehicle_index)
{
  return requireRuntimes(runtimes_).at(vehicle_index);
}

const SingleVehicleRuntime & RaceCoordinator::runtime_for_vehicle(const std::string & vehicle_id) const
{
  return requireRuntimes(runtimes_)[findVehicleIndex(participating_vehicle_ids_, vehicle_id)];
}

SingleVehicleRuntime & RaceCoordinator::runtime_for_vehicle(const std::string & vehicle_id)
{
  return requireRuntimes(runtimes_)[findVehicleIndex(participating_vehicle_ids_, vehicle_id)];
}

bool RaceCoordinator::all_participating_vehicles_finished() const
{
  const VehicleRuntimes & runtimes = requireRuntimes(runtimes_);
  for (const auto & runtime : runtimes) {
    if (!runtime.snapshot().has_finished) {
      return false;
    }
  }
  return true;
}

std::string RaceCoordinator::current_race_status() const
{
  if (all_participating_vehicles_finished()) {
    return "completed";
  }

  const VehicleRuntimes & runtimes = requireRuntimes(runtimes_);
  for (const auto & runtime : runtimes) {
    if (runtime.running()) {
      return "running";
    }
  }

  return "stopped";
}

std::int32_t RaceCoordinator::current_step_sec() const
{
  const VehicleRuntimes & runtimes = requireRuntimes(runtimes_);
  std::int32_t step_sec = 0;
  for (const auto & runtime : runtimes) {
    step_sec = std::max(step_sec, runtime.current_step_sec());
  }
  return step_sec;
}

ProgressSnapshot RaceCoordinator::primary_snapshot() const
{
  return primary_runtime().snapshot();
}

RaceCoordinator::RaceStateSource RaceCoordinator::current_race_state_source() const
{
  return RaceStateSource{
    current_race_status(),
    current_step_sec(),
    primary_snapshot(),
  };
}

bool RaceCoordinator::start()
{
  bool reset_performed = false;
  for (auto & runtime : requireRuntimes(runtimes_)) {
    reset_performed = runtime.start() || reset_performed;
  }
  return reset_performed;
}

bool RaceCoordinator::stop()
{
  bool was_running = false;
  for (auto & runtime : requireRuntimes(runtimes_)) {
    was_running = runtime.stop() || was_running;
  }
  return was_running;
}

void RaceCoordinator::reset()
{
  for (auto & runtime : requireRuntimes(runtimes_)) {
    runtime.reset();
  }
}

}  // namespace race_track
