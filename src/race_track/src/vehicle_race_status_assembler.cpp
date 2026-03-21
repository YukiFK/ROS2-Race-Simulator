#include "race_track/vehicle_race_status_assembler.hpp"

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

race_interfaces::msg::VehicleRaceStatus VehicleRaceStatusAssembler::assemble(
  const std::int32_t step_sec, const std::string & vehicle_id,
  const ProgressUpdate & progress_update) const
{
  race_interfaces::msg::VehicleRaceStatus status;
  status.header.stamp.sec = step_sec;
  status.header.stamp.nanosec = 0U;
  status.header.frame_id = kFrameId;
  status.vehicle_id = vehicle_id;
  status.lap_count = progress_update.snapshot.lap_count;
  status.current_lap_time = makeDuration(step_sec - progress_update.snapshot.lap_start_step_sec);
  status.last_lap_time = makeDuration(progress_update.snapshot.last_lap_time_sec);
  status.best_lap_time = makeDuration(progress_update.snapshot.best_lap_time_sec);
  status.total_elapsed_time = makeDuration(step_sec);
  status.has_finished = progress_update.snapshot.has_finished;
  status.is_off_track = progress_update.is_off_track;
  status.off_track_count = progress_update.snapshot.off_track_count;
  return status;
}

}  // namespace race_track
