#pragma once

#include "../common/Track.h"
#include <string>
#include <vector>

std::vector<Track> discover_local_tracks(const std::string &music_root = "");
