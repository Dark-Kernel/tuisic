#pragma once

#include <string>
#include <vector>
#include "../../common/Track.h"

class Fetch {
public:
    std::vector<Track> fetch_tracks(const std::string& search_query);

private:
    static size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* userp);
    std::vector<Track> extractTracks(const std::string& html);
};

