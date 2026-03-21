#ifndef RACE_TRACK__RACE_STATE_ASSEMBLER_HPP_
#define RACE_TRACK__RACE_STATE_ASSEMBLER_HPP_

#include <cstdint>
#include <string>

#include "race_interfaces/msg/race_state.hpp"
#include "race_track/progress_tracker.hpp"

namespace race_track
{

class RaceStateAssembler
{
public:
  race_interfaces::msg::RaceState assemble(
    const std::string & race_status, std::int32_t step_sec,
    const ProgressSnapshot & snapshot) const;
};

}  // namespace race_track

#endif  // RACE_TRACK__RACE_STATE_ASSEMBLER_HPP_
