#include <gtest/gtest.h>

#include <functional>
#include <stdexcept>
#include <string>

#include "race_track/track_model.hpp"
#include "race_track/track_validator.hpp"

namespace race_track
{
namespace
{

TrackModel makeValidTrack()
{
  TrackModel track;
  track.track_name = "unit_test_track";
  track.centerline = {{0.0, 0.0}, {5.0, 0.0}, {10.0, 2.0}};
  track.track_width = 4.0;
  track.start_line = {{0.0, -2.0}, {0.0, 2.0}};
  track.forward_hint = {1.0, 0.0};
  return track;
}

void expectRuntimeErrorContains(
  const std::function<void()> & fn, const std::string & expected_substring)
{
  try {
    fn();
    FAIL() << "Expected std::runtime_error containing: " << expected_substring;
  } catch (const std::runtime_error & ex) {
    EXPECT_NE(std::string(ex.what()).find(expected_substring), std::string::npos)
      << "Actual message: " << ex.what();
  } catch (...) {
    FAIL() << "Expected std::runtime_error";
  }
}

}  // namespace

TEST(TrackValidatorTest, AcceptsValidTrackModel)
{
  EXPECT_NO_THROW(validateTrackOrThrow(makeValidTrack()));
}

TEST(TrackValidatorTest, ThrowsWhenTrackNameIsEmpty)
{
  TrackModel track = makeValidTrack();
  track.track_name.clear();

  expectRuntimeErrorContains(
    [&track]() { validateTrackOrThrow(track); }, "'track_name' must not be empty");
}

TEST(TrackValidatorTest, ThrowsWhenCenterlineHasFewerThanThreePoints)
{
  TrackModel track = makeValidTrack();
  track.centerline = {{0.0, 0.0}, {1.0, 0.0}};

  expectRuntimeErrorContains(
    [&track]() { validateTrackOrThrow(track); }, "'centerline' must contain at least 3 points");
}

TEST(TrackValidatorTest, ThrowsWhenTrackWidthIsNonPositive)
{
  TrackModel track = makeValidTrack();
  track.track_width = 0.0;

  expectRuntimeErrorContains(
    [&track]() { validateTrackOrThrow(track); }, "'track_width' must be greater than 0");
}

TEST(TrackValidatorTest, ThrowsWhenStartLineHasZeroLength)
{
  TrackModel track = makeValidTrack();
  track.start_line = {{1.0, 1.0}, {1.0, 1.0}};

  expectRuntimeErrorContains(
    [&track]() { validateTrackOrThrow(track); }, "'start_line' must not be zero-length");
}

TEST(TrackValidatorTest, ThrowsWhenForwardHintIsZeroVector)
{
  TrackModel track = makeValidTrack();
  track.forward_hint = {0.0, 0.0};

  expectRuntimeErrorContains(
    [&track]() { validateTrackOrThrow(track); }, "'forward_hint' must not be the zero vector");
}

}  // namespace race_track
