#ifndef RACE_TRACK__LAP_EVENT_ASSEMBLER_HPP_
#define RACE_TRACK__LAP_EVENT_ASSEMBLER_HPP_

#include <cstdint>
#include <string>

#include "race_interfaces/msg/lap_event.hpp"
#include "race_track/progress_tracker.hpp"

namespace race_track
{

class LapEventAssembler
{
public:
  race_interfaces::msg::LapEvent assemble(
    std::int32_t step_sec, const std::string & vehicle_id,
    const ProgressUpdate & progress_update) const;
};

}  // namespace race_track

#endif  // RACE_TRACK__LAP_EVENT_ASSEMBLER_HPP_
