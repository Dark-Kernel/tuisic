#pragma once

#include <string>
#include <vector>
#include "../../common/Track.h"

class Justmusic {
public:
    std::string fetchURL(const std::string &url);
    std::string extractName(const std::string &url);
    std::vector<Track> extractMP3URL(const std::string &html);
    std::vector<Track> getMP3URL();

private:
    static size_t WriteCallback(void *contents, size_t size, size_t nmemb, std::string *userp);
    std::vector<Track> tracks;
};

