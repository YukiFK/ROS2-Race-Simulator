#include "race_track/progress_tracker.hpp"

#include "race_track/geometry.hpp"

namespace race_track
{

ProgressTracker::ProgressTracker(const TrackModel & track)
: track_(track)
{
}

void ProgressTracker::reset()
{
  snapshot_ = ProgressSnapshot{};
  has_previous_position_ = false;
  previous_position_ = Point2d{};
}

void ProgressTracker::setHasFinished(const bool has_finished)
{
  snapshot_.has_finished = has_finished;
}

ProgressUpdate ProgressTracker::update(
  const std::int32_t step_sec, const Point2d & current_position)
{
  ProgressUpdate update;
  update.nearest_centerline_index =
    findNearestCenterlineIndex(track_.centerline, current_position);
  update.distance_to_centerline = distanceToCenterline(track_.centerline, current_position);
  update.is_off_track = update.distance_to_centerline > (track_.track_width / 2.0);
  if (update.is_off_track) {
    ++snapshot_.off_track_count;
  }

  if (has_previous_position_) {
    update.crossing_detected = isForwardCrossingStartLine(
      previous_position_, current_position, track_.start_line, track_.forward_hint);
    if (update.crossing_detected) {
      update.crossed_lap_time_sec = step_sec - snapshot_.lap_start_step_sec;
      ++snapshot_.lap_count;
      snapshot_.last_lap_time_sec = update.crossed_lap_time_sec;
      if (snapshot_.best_lap_time_sec == 0 ||
        update.crossed_lap_time_sec < snapshot_.best_lap_time_sec)
      {
        snapshot_.best_lap_time_sec = update.crossed_lap_time_sec;
      }
      snapshot_.lap_start_step_sec = step_sec;
    }
  }

  previous_position_ = current_position;
  has_previous_position_ = true;
  update.snapshot = snapshot_;
  return update;
}

ProgressSnapshot ProgressTracker::snapshot() const
{
  return snapshot_;
}

}  // namespace race_track
