#include "race_track/single_vehicle_runtime.hpp"

#include <stdexcept>
#include <utility>

namespace race_track
{

SingleVehicleRuntime::SingleVehicleRuntime(
  const TrackModel & track, std::vector<Point2d> positions, const std::int64_t target_lap_count)
: progress_tracker_(track),
  positions_(std::move(positions)),
  target_lap_count_(target_lap_count)
{
  if (target_lap_count_ <= 0) {
    throw std::runtime_error("target_lap_count must be greater than zero");
  }
}

bool SingleVehicleRuntime::start()
{
  bool reset_performed = false;
  if (completed_ || step_index_ >= positions_.size()) {
    reset();
    reset_performed = true;
  }

  completed_ = false;
  running_ = true;
  return reset_performed;
}

bool SingleVehicleRuntime::stop()
{
  const bool was_running = running_;
  running_ = false;
  completed_ = false;
  return was_running;
}

void SingleVehicleRuntime::reset()
{
  running_ = false;
  completed_ = false;
  step_index_ = 0U;
  progress_tracker_.reset();
}

SingleVehicleTickResult SingleVehicleRuntime::tick()
{
  SingleVehicleTickResult result;

  if (!running_ || completed_) {
    return result;
  }

  if (step_index_ >= positions_.size()) {
    running_ = false;
    return result;
  }

  result.advanced = true;
  result.step_sec = static_cast<std::int32_t>(step_index_);
  result.current_position = positions_[step_index_];
  result.progress_update = progress_tracker_.update(result.step_sec, result.current_position);

  const CompletionDecision decision = completion_evaluator_.evaluate(
    result.progress_update.snapshot, step_index_, positions_.size(), target_lap_count_);

  if (decision.should_complete) {
    running_ = false;
    completed_ = true;
    progress_tracker_.setHasFinished(true);
    result.progress_update.snapshot = progress_tracker_.snapshot();
    result.just_completed = true;
  }

  if (decision.should_stop_without_completion) {
    running_ = false;
    result.stopped_without_completion = true;
  }

  result.crossing_detected = result.progress_update.crossing_detected;
  ++step_index_;
  return result;
}

bool SingleVehicleRuntime::running() const
{
  return running_;
}

bool SingleVehicleRuntime::completed() const
{
  return completed_;
}

std::size_t SingleVehicleRuntime::step_index() const
{
  return step_index_;
}

std::int32_t SingleVehicleRuntime::current_step_sec() const
{
  if (positions_.empty()) {
    return 0;
  }

  const std::size_t clamped_index =
    step_index_ < positions_.size() ? step_index_ : positions_.size() - 1U;
  return static_cast<std::int32_t>(clamped_index);
}

std::string SingleVehicleRuntime::current_race_status() const
{
  if (completed_) {
    return "completed";
  }

  return running_ ? "running" : "stopped";
}

ProgressSnapshot SingleVehicleRuntime::snapshot() const
{
  return progress_tracker_.snapshot();
}

}  // namespace race_track
