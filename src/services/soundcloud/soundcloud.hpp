#pragma once

#include <string>
#include <vector>
#include "../../common/Track.h"

class SoundCloud {
public:
    std::vector<Track> fetch_soundcloud_tracks(const std::string& search_query, bool is_user_profile = false);

private:
    static size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* userp);
    std::vector<Track> extractTracks(const std::string& html);
    std::vector<Track> extractUserTracks(const std::string& html);
};

