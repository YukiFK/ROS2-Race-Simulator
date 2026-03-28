#ifndef RACE_TRACK__RACE_COORDINATOR_HPP_
#define RACE_TRACK__RACE_COORDINATOR_HPP_

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "race_track/geometry.hpp"
#include "race_track/single_vehicle_runtime.hpp"
#include "race_track/track_model.hpp"

namespace race_track
{

class RaceCoordinator
{
public:
  struct RaceStateSource
  {
    std::string race_status;
    std::int32_t step_sec{0};
    ProgressSnapshot primary_snapshot{};
  };

  using ParticipatingVehicleIds = std::array<std::string, 2>;
  using VehicleRuntimePositions = std::array<std::vector<Point2d>, 2>;
  using VehicleRuntimes = std::array<SingleVehicleRuntime, 2>;

  explicit RaceCoordinator(TrackModel track);
  RaceCoordinator(TrackModel track, ParticipatingVehicleIds participating_vehicle_ids);
  RaceCoordinator(
    TrackModel track, VehicleRuntimePositions runtime_positions, std::int64_t target_lap_count);
  RaceCoordinator(
    TrackModel track, ParticipatingVehicleIds participating_vehicle_ids,
    VehicleRuntimePositions runtime_positions, std::int64_t target_lap_count);

  const ParticipatingVehicleIds & participating_vehicle_ids() const;
  std::size_t vehicle_count() const;
  const TrackModel & track() const;
  const SingleVehicleRuntime & primary_runtime() const;
  SingleVehicleRuntime & primary_runtime();
  const SingleVehicleRuntime & runtime_at(std::size_t vehicle_index) const;
  SingleVehicleRuntime & runtime_at(std::size_t vehicle_index);
  const SingleVehicleRuntime & runtime_for_vehicle(const std::string & vehicle_id) const;
  SingleVehicleRuntime & runtime_for_vehicle(const std::string & vehicle_id);
  bool all_participating_vehicles_finished() const;
  std::string current_race_status() const;
  std::int32_t current_step_sec() const;
  ProgressSnapshot primary_snapshot() const;
  RaceStateSource current_race_state_source() const;
  bool start();
  bool stop();
  void reset();

private:
  TrackModel track_;
  ParticipatingVehicleIds participating_vehicle_ids_;
  std::optional<VehicleRuntimes> runtimes_;
};

}  // namespace race_track

#endif  // RACE_TRACK__RACE_COORDINATOR_HPP_
