#include "race_track/completion_evaluator.hpp"

namespace race_track
{

CompletionDecision SingleVehicleCompletionEvaluator::evaluate(
  const ProgressSnapshot & snapshot, const std::size_t step_index, const std::size_t position_count,
  const std::int64_t target_lap_count) const
{
  CompletionDecision decision;

  if (target_lap_count <= 0 || position_count == 0) {
    return decision;
  }

  decision.should_complete =
    snapshot.lap_count >= static_cast<std::int32_t>(target_lap_count);

  if (!decision.should_complete && (step_index + 1U) >= position_count) {
    decision.should_stop_without_completion = true;
  }

  return decision;
}

}  // namespace race_track
