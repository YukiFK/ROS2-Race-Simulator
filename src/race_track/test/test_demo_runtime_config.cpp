#include <gtest/gtest.h>

#include <stdexcept>
#include <string>
#include <vector>

#include "race_track/demo_runtime_config.hpp"

namespace race_track
{
namespace
{

TEST(DemoRuntimeConfigTest, ExposesCurrentTwoVehicleDefaults)
{
  EXPECT_EQ(
    defaultDemoParticipatingVehicleIds(),
    (RaceCoordinator::ParticipatingVehicleIds{"demo_vehicle_1", "demo_vehicle_2"}));

  const auto default_specs = defaultDemoRuntimePositionSpecs();
  ASSERT_EQ(default_specs.size(), 2U);

  const auto runtime_positions = parseRuntimePositionSpecs(default_specs);
  ASSERT_EQ(runtime_positions.size(), 2U);
  EXPECT_EQ(runtime_positions[0].front().x, -2.0);
  EXPECT_EQ(runtime_positions[1].front().y, 0.5);
}

TEST(DemoRuntimeConfigTest, ParsesVariableLengthVehicleRuntimePositionSpecs)
{
  const auto runtime_positions = parseRuntimePositionSpecs({
      "0.0,0.0;5.0,0.0;10.0,0.0",
      "0.0,0.5;5.0,0.5;10.0,0.5",
      "0.0,1.0;5.0,1.0;10.0,1.0",
    });

  ASSERT_EQ(runtime_positions.size(), 3U);
  ASSERT_EQ(runtime_positions[2].size(), 3U);
  EXPECT_DOUBLE_EQ(runtime_positions[2][1].x, 5.0);
  EXPECT_DOUBLE_EQ(runtime_positions[2][2].y, 1.0);
}

TEST(DemoRuntimeConfigTest, RejectsMalformedRuntimePositionSpecs)
{
  EXPECT_THROW(static_cast<void>(parseRuntimePositionSpecs({})), std::invalid_argument);
  EXPECT_THROW(
    static_cast<void>(parseRuntimePositionSpecs({"0.0,0.0;", "0.0,0.5;5.0,0.5"})),
    std::invalid_argument);
  EXPECT_THROW(
    static_cast<void>(parseRuntimePositionSpecs({"0.0,0.0;broken", "0.0,0.5;5.0,0.5"})),
    std::invalid_argument);
}

}  // namespace
}  // namespace race_track
