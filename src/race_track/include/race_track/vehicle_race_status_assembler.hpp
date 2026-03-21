#ifndef RACE_TRACK__VEHICLE_RACE_STATUS_ASSEMBLER_HPP_
#define RACE_TRACK__VEHICLE_RACE_STATUS_ASSEMBLER_HPP_

#include <cstdint>
#include <string>

#include "race_interfaces/msg/vehicle_race_status.hpp"
#include "race_track/progress_tracker.hpp"

namespace race_track
{

class VehicleRaceStatusAssembler
{
public:
  race_interfaces::msg::VehicleRaceStatus assemble(
    std::int32_t step_sec, const std::string & vehicle_id,
    const ProgressUpdate & progress_update) const;
};

}  // namespace race_track

#endif  // RACE_TRACK__VEHICLE_RACE_STATUS_ASSEMBLER_HPP_
