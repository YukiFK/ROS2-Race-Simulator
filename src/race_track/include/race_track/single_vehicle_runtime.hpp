#ifndef RACE_TRACK__SINGLE_VEHICLE_RUNTIME_HPP_
#define RACE_TRACK__SINGLE_VEHICLE_RUNTIME_HPP_

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "race_track/completion_evaluator.hpp"
#include "race_track/geometry.hpp"
#include "race_track/progress_tracker.hpp"
#include "race_track/track_model.hpp"

namespace race_track
{

struct SingleVehicleTickResult
{
  bool advanced{false};
  bool crossing_detected{false};
  bool just_completed{false};
  bool stopped_without_completion{false};
  std::int32_t step_sec{0};
  Point2d current_position{};
  ProgressUpdate progress_update{};
};

class SingleVehicleRuntime
{
public:
  SingleVehicleRuntime(
    const TrackModel & track, std::vector<Point2d> positions, std::int64_t target_lap_count);

  bool start();
  bool stop();
  void reset();

  SingleVehicleTickResult tick();

  bool running() const;
  bool completed() const;
  std::size_t step_index() const;
  std::int32_t current_step_sec() const;
  std::string current_race_status() const;
  ProgressSnapshot snapshot() const;

private:
  ProgressTracker progress_tracker_;
  SingleVehicleCompletionEvaluator completion_evaluator_;
  std::vector<Point2d> positions_;
  std::int64_t target_lap_count_{0};
  bool running_{false};
  bool completed_{false};
  std::size_t step_index_{0U};
};

}  // namespace race_track

#endif  // RACE_TRACK__SINGLE_VEHICLE_RUNTIME_HPP_
