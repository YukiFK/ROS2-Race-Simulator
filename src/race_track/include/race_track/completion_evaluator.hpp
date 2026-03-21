#ifndef RACE_TRACK__COMPLETION_EVALUATOR_HPP_
#define RACE_TRACK__COMPLETION_EVALUATOR_HPP_

#include <cstddef>
#include <cstdint>

#include "race_track/progress_tracker.hpp"

namespace race_track
{

struct CompletionDecision
{
  bool should_complete{false};
  bool should_stop_without_completion{false};
};

class SingleVehicleCompletionEvaluator
{
public:
  CompletionDecision evaluate(
    const ProgressSnapshot & snapshot, std::size_t step_index, std::size_t position_count,
    std::int64_t target_lap_count) const;
};

}  // namespace race_track

#endif  // RACE_TRACK__COMPLETION_EVALUATOR_HPP_
