#include "race_track/lap_event_assembler.hpp"

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

race_interfaces::msg::LapEvent LapEventAssembler::assemble(
  const std::int32_t step_sec, const std::string & vehicle_id,
  const ProgressUpdate & progress_update) const
{
  race_interfaces::msg::LapEvent lap_event;
  lap_event.header.stamp.sec = step_sec;
  lap_event.header.stamp.nanosec = 0U;
  lap_event.header.frame_id = kFrameId;
  lap_event.vehicle_id = vehicle_id;
  lap_event.lap_count = progress_update.snapshot.lap_count;
  lap_event.lap_time = makeDuration(progress_update.crossed_lap_time_sec);
  lap_event.best_lap_time = makeDuration(progress_update.snapshot.best_lap_time_sec);
  lap_event.has_finished = progress_update.snapshot.has_finished;
  return lap_event;
}

}  // namespace race_track
