#include <gtest/gtest.h>

#include "race_track/completion_evaluator.hpp"

namespace race_track
{
namespace
{

TEST(CompletionEvaluatorTest, CompletesWhenTargetLapCountReached)
{
  SingleVehicleCompletionEvaluator evaluator;
  ProgressSnapshot snapshot;
  snapshot.lap_count = 2;

  const CompletionDecision decision = evaluator.evaluate(snapshot, 3U, 10U, 2);

  EXPECT_TRUE(decision.should_complete);
  EXPECT_FALSE(decision.should_stop_without_completion);
}

TEST(CompletionEvaluatorTest, StopsWithoutCompletionAtFinalStepWhenTargetNotReached)
{
  SingleVehicleCompletionEvaluator evaluator;
  ProgressSnapshot snapshot;
  snapshot.lap_count = 1;

  const CompletionDecision decision = evaluator.evaluate(snapshot, 4U, 5U, 2);

  EXPECT_FALSE(decision.should_complete);
  EXPECT_TRUE(decision.should_stop_without_completion);
}

TEST(CompletionEvaluatorTest, ContinuesBeforeFinalStepWhenTargetNotReached)
{
  SingleVehicleCompletionEvaluator evaluator;
  ProgressSnapshot snapshot;
  snapshot.lap_count = 1;

  const CompletionDecision decision = evaluator.evaluate(snapshot, 3U, 5U, 2);

  EXPECT_FALSE(decision.should_complete);
  EXPECT_FALSE(decision.should_stop_without_completion);
}

}  // namespace
}  // namespace race_track
