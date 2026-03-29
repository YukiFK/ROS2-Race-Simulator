#ifndef RACE_TRACK__DEMO_RUNTIME_CONFIG_HPP_
#define RACE_TRACK__DEMO_RUNTIME_CONFIG_HPP_

#include <string>
#include <vector>

#include "race_track/race_coordinator.hpp"

namespace race_track
{

RaceCoordinator::ParticipatingVehicleIds defaultDemoParticipatingVehicleIds();
std::vector<std::string> defaultDemoRuntimePositionSpecs();
RaceCoordinator::VehicleRuntimePositions parseRuntimePositionSpecs(
  const std::vector<std::string> & runtime_position_specs);

}  // namespace race_track

#endif  // RACE_TRACK__DEMO_RUNTIME_CONFIG_HPP_
