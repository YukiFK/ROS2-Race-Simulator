#include <fcntl.h>
#include <gtest/gtest.h>
#include <unistd.h>

#include <cstdio>
#include <filesystem>
#include <functional>
#include <fstream>
#include <stdexcept>
#include <string>

#include "race_track/track_loader.hpp"

namespace race_track
{
namespace
{

std::string getSampleTrackPath()
{
  return (
    std::filesystem::path(__FILE__).parent_path().parent_path() / "config" / "sample_track.yaml").string();
}

class TemporaryYamlFile
{
public:
  explicit TemporaryYamlFile(const std::string & contents)
  {
    char path_template[] = "/tmp/race_track_loader_test_XXXXXX.yaml";
    const int fd = mkstemps(path_template, 5);
    if (fd == -1) {
      throw std::runtime_error("Failed to create temporary YAML file");
    }

    path_ = path_template;

    std::ofstream output(path_);
    if (!output.is_open()) {
      close(fd);
      std::remove(path_.c_str());
      throw std::runtime_error("Failed to open temporary YAML file for writing");
    }

    output << contents;
    output.close();
    close(fd);
  }

  ~TemporaryYamlFile()
  {
    if (!path_.empty()) {
      std::remove(path_.c_str());
    }
  }

  const std::string & path() const
  {
    return path_;
  }

private:
  std::string path_{};
};

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

TEST(TrackLoaderTest, LoadsSampleTrackYaml)
{
  const TrackModel track = loadTrackFromYaml(getSampleTrackPath());

  EXPECT_EQ(track.track_name, "sample_track");
  ASSERT_EQ(track.centerline.size(), 3U);
  EXPECT_DOUBLE_EQ(track.centerline[0].x, 0.0);
  EXPECT_DOUBLE_EQ(track.centerline[0].y, 0.0);
  EXPECT_DOUBLE_EQ(track.centerline[1].x, 10.0);
  EXPECT_DOUBLE_EQ(track.centerline[1].y, 0.0);
  EXPECT_DOUBLE_EQ(track.centerline[2].x, 20.0);
  EXPECT_DOUBLE_EQ(track.centerline[2].y, 5.0);
  EXPECT_DOUBLE_EQ(track.track_width, 6.5);
  EXPECT_DOUBLE_EQ(track.start_line.p1.x, 0.0);
  EXPECT_DOUBLE_EQ(track.start_line.p1.y, -3.25);
  EXPECT_DOUBLE_EQ(track.start_line.p2.x, 0.0);
  EXPECT_DOUBLE_EQ(track.start_line.p2.y, 3.25);
  EXPECT_DOUBLE_EQ(track.forward_hint.x, 1.0);
  EXPECT_DOUBLE_EQ(track.forward_hint.y, 0.0);
}

TEST(TrackLoaderTest, ThrowsWhenRequiredKeyIsMissing)
{
  const TemporaryYamlFile yaml_file(R"(
track_name: sample_track
centerline:
  - x: 0.0
    y: 0.0
  - x: 10.0
    y: 0.0
  - x: 20.0
    y: 5.0
start_line:
  p1:
    x: 0.0
    y: -3.25
  p2:
    x: 0.0
    y: 3.25
forward_hint:
  x: 1.0
  y: 0.0
)");

  expectRuntimeErrorContains(
    [&yaml_file]() { (void)loadTrackFromYaml(yaml_file.path()); }, "Missing required key 'track_width'");
}

TEST(TrackLoaderTest, ThrowsWhenYamlIsMalformed)
{
  const TemporaryYamlFile yaml_file(R"(
track_name: broken
centerline:
  - x: 0.0
    y: 0.0
  - x: 1.0
    y: [1.0
)");

  expectRuntimeErrorContains(
    [&yaml_file]() { (void)loadTrackFromYaml(yaml_file.path()); }, "Failed to parse YAML file");
}

}  // namespace race_track
