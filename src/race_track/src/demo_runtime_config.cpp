#include "race_track/demo_runtime_config.hpp"

#include <cctype>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace race_track
{
namespace
{

std::string trimCopy(const std::string & value)
{
  std::size_t start = 0U;
  while (start < value.size() && std::isspace(static_cast<unsigned char>(value[start])) != 0) {
    ++start;
  }

  std::size_t end = value.size();
  while (end > start && std::isspace(static_cast<unsigned char>(value[end - 1U])) != 0) {
    --end;
  }

  return value.substr(start, end - start);
}

std::vector<std::string> splitString(const std::string & value, const char delimiter)
{
  std::vector<std::string> tokens;
  std::size_t start = 0U;
  while (start <= value.size()) {
    const std::size_t delimiter_index = value.find(delimiter, start);
    if (delimiter_index == std::string::npos) {
      tokens.push_back(value.substr(start));
      break;
    }

    tokens.push_back(value.substr(start, delimiter_index - start));
    start = delimiter_index + 1U;
  }

  return tokens;
}

double parseCoordinate(const std::string & token, const char axis_name)
{
  const std::string trimmed = trimCopy(token);
  if (trimmed.empty()) {
    throw std::invalid_argument(std::string("runtime position ") + axis_name + " coordinate is empty");
  }

  std::size_t parsed_length = 0U;
  const double coordinate = std::stod(trimmed, &parsed_length);
  if (parsed_length != trimmed.size()) {
    throw std::invalid_argument(
            std::string("runtime position ") + axis_name + " coordinate is malformed: " + trimmed);
  }

  return coordinate;
}

Point2d parsePoint(const std::string & point_spec)
{
  const std::vector<std::string> coordinate_tokens = splitString(point_spec, ',');
  if (coordinate_tokens.size() != 2U) {
    throw std::invalid_argument(
            "runtime position point must be formatted as 'x,y': " + point_spec);
  }

  return Point2d{
    parseCoordinate(coordinate_tokens[0], 'x'),
    parseCoordinate(coordinate_tokens[1], 'y'),
  };
}

std::vector<Point2d> parseVehicleRuntimePositions(const std::string & vehicle_runtime_spec)
{
  std::vector<Point2d> positions;
  for (const std::string & point_token : splitString(vehicle_runtime_spec, ';')) {
    const std::string trimmed_point = trimCopy(point_token);
    if (trimmed_point.empty()) {
      throw std::invalid_argument("runtime position point must not be empty");
    }
    positions.push_back(parsePoint(trimmed_point));
  }

  if (positions.empty()) {
    throw std::invalid_argument("runtime position spec must contain at least one point");
  }

  return positions;
}

}  // namespace

RaceCoordinator::ParticipatingVehicleIds defaultDemoParticipatingVehicleIds()
{
  return {"demo_vehicle_1", "demo_vehicle_2"};
}

std::vector<std::string> defaultDemoRuntimePositionSpecs()
{
  return {
    "-2.0,0.0;-0.5,0.2;1.0,0.2;6.0,0.1;11.0,0.4;18.0,4.8;9.0,5.0;0.5,0.0;-1.0,0.0;1.5,-0.1;4.0,4.0",
    "-2.0,0.5;-0.5,0.7;1.0,0.7;6.0,0.6;11.0,0.9;18.0,5.3;9.0,5.5;0.5,0.5;-1.0,0.5;1.5,0.4;4.0,4.5",
  };
}

RaceCoordinator::VehicleRuntimePositions parseRuntimePositionSpecs(
  const std::vector<std::string> & runtime_position_specs)
{
  if (runtime_position_specs.empty()) {
    throw std::invalid_argument("runtime_position_specs must contain at least one vehicle");
  }

  RaceCoordinator::VehicleRuntimePositions runtime_positions;
  runtime_positions.reserve(runtime_position_specs.size());
  for (const std::string & vehicle_runtime_spec : runtime_position_specs) {
    runtime_positions.push_back(parseVehicleRuntimePositions(vehicle_runtime_spec));
  }

  return runtime_positions;
}

}  // namespace race_track
