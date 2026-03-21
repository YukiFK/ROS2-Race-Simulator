#include "race_track/race_state_assembler.hpp"

#include "builtin_interfaces/msg/duration.hpp"

namespace race_track
{
namespace
{

constexpr char kFrameId[] = "map";

builtin_interfaces::msg::Duration makeDuration(const std::int32_t seconds)
{
  builtin_interfaces::msg::Duration duration;
  duration.sec = seconds;
  duration.nanosec = 0U;
  return duration;
}

}  // namespace

race_interfaces::msg::RaceState RaceStateAssembler::assemble(
  const std::string & race_status, const std::int32_t step_sec,
  const ProgressSnapshot & snapshot) const
{
  race_interfaces::msg::RaceState race_state;
  race_state.header.stamp.sec = step_sec;
  race_state.header.stamp.nanosec = 0U;
  race_state.header.frame_id = kFrameId;
  race_state.race_status = race_status;
  race_state.elapsed_time = makeDuration(step_sec);
  race_state.completed_laps = snapshot.lap_count;
  return race_state;
}

}  // namespace race_track
