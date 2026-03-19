#ifndef RACE_TRACK__TRACK_LOADER_HPP_
#define RACE_TRACK__TRACK_LOADER_HPP_

#include <string>

#include "race_track/track_model.hpp"

namespace race_track
{

TrackModel loadTrackFromYaml(const std::string & yaml_path);

}  // namespace race_track

#endif  // RACE_TRACK__TRACK_LOADER_HPP_
