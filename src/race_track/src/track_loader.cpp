#include "race_track/track_loader.hpp"

#include <stdexcept>
#include <string>

#include <yaml-cpp/yaml.h>

namespace race_track
{
namespace
{

YAML::Node requireKey(const YAML::Node & node, const std::string & key, const std::string & context)
{
  const YAML::Node value = node[key];
  if (!value) {
    throw std::runtime_error("Missing required key '" + context + key + "'");
  }
  return value;
}

Point2d parsePoint2d(const YAML::Node & node, const std::string & context)
{
  try {
    return Point2d{
      requireKey(node, "x", context + ".").as<double>(),
      requireKey(node, "y", context + ".").as<double>()};
  } catch (const YAML::Exception & ex) {
    throw std::runtime_error("Failed to parse key '" + context + "': " + ex.what());
  }
}

LineSegment2d parseLineSegment2d(const YAML::Node & node, const std::string & context)
{
  return LineSegment2d{
    parsePoint2d(requireKey(node, "p1", context + "."), context + ".p1"),
    parsePoint2d(requireKey(node, "p2", context + "."), context + ".p2")};
}

}  // namespace

TrackModel loadTrackFromYaml(const std::string & yaml_path)
{
  YAML::Node root;
  try {
    root = YAML::LoadFile(yaml_path);
  } catch (const YAML::Exception & ex) {
    throw std::runtime_error("Failed to parse YAML file '" + yaml_path + "': " + ex.what());
  }

  try {
    TrackModel track;
    track.track_name = requireKey(root, "track_name", "").as<std::string>();

    const YAML::Node centerline_node = requireKey(root, "centerline", "");
    if (!centerline_node.IsSequence()) {
      throw std::runtime_error("Failed to parse key 'centerline': expected sequence");
    }
    for (std::size_t i = 0; i < centerline_node.size(); ++i) {
      track.centerline.push_back(parsePoint2d(centerline_node[i], "centerline[" + std::to_string(i) + "]"));
    }

    track.track_width = requireKey(root, "track_width", "").as<double>();
    track.start_line = parseLineSegment2d(requireKey(root, "start_line", ""), "start_line");
    track.forward_hint = parsePoint2d(requireKey(root, "forward_hint", ""), "forward_hint");
    return track;
  } catch (const YAML::Exception & ex) {
    throw std::runtime_error("Failed to parse track YAML '" + yaml_path + "': " + ex.what());
  }
}

}  // namespace race_track
