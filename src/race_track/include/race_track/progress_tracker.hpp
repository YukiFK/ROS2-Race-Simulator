#ifndef RACE_TRACK__PROGRESS_TRACKER_HPP_
#define RACE_TRACK__PROGRESS_TRACKER_HPP_

#include <cstddef>
#include <cstdint>

#include "race_track/track_model.hpp"

namespace race_track
{

struct ProgressSnapshot
{
  std::int32_t lap_count{0};
  std::int32_t off_track_count{0};
  std::int32_t lap_start_step_sec{0};
  std::int32_t last_lap_time_sec{0};
  std::int32_t best_lap_time_sec{0};
  bool has_finished{false};
};

struct ProgressUpdate
{
  ProgressSnapshot snapshot{};
  std::size_t nearest_centerline_index{0U};
  double distance_to_centerline{0.0};
  bool is_off_track{false};
  bool crossing_detected{false};
  std::int32_t crossed_lap_time_sec{0};
};

class ProgressTracker
{
public:
  explicit ProgressTracker(const TrackModel & track);

  void reset();

  void setHasFinished(bool has_finished);

  ProgressUpdate update(std::int32_t step_sec, const Point2d & current_position);

  ProgressSnapshot snapshot() const;

private:
  TrackModel track_;
  ProgressSnapshot snapshot_{};
  bool has_previous_position_{false};
  Point2d previous_position_{};
};

}  // namespace race_track

#endif  // RACE_TRACK__PROGRESS_TRACKER_HPP_
