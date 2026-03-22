#ifndef RACE_TRACK__RACE_COORDINATOR_HPP_
#define RACE_TRACK__RACE_COORDINATOR_HPP_

#include <array>
#include <cstddef>
#include <string>

#include "race_track/track_model.hpp"

namespace race_track
{

class RaceCoordinator
{
public:
  using ParticipatingVehicleIds = std::array<std::string, 2>;

  explicit RaceCoordinator(TrackModel track);
  RaceCoordinator(TrackModel track, ParticipatingVehicleIds participating_vehicle_ids);

  const ParticipatingVehicleIds & participating_vehicle_ids() const;
  std::size_t vehicle_count() const;
  const TrackModel & track() const;

private:
  TrackModel track_;
  ParticipatingVehicleIds participating_vehicle_ids_;
};

}  // namespace race_track

#endif  // RACE_TRACK__RACE_COORDINATOR_HPP_
