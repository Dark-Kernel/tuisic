#ifndef SOUNDCLOUD_HPP
#define SOUNDCLOUD_HPP

#include <string>
#include <vector>
#include "Track.h"

// Forward declaration of the SoundCloud class
class SoundCloud {
public:
    // Main function to fetch tracks from SoundCloud
    std::vector<Track> fetch_soundcloud_tracks(const std::string& search_query, bool is_user_profile = false);

private:
    // Callback function for CURL
    static size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* userp);

    // Helper functions for extracting tracks
    std::vector<Track> extractTracks(const std::string& html);
    std::vector<Track> extractUserTracks(const std::string& html);
};

#endif // SOUNDCLOUD_HPP

