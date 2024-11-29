#ifndef FETCH_HPP
#define FETCH_HPP

#include <string>
#include <vector>
#include "Track.h"

// Forward declaration of the Fetch class
class Fetch {
public:
    // Main function to fetch tracks from Last.fm
    std::vector<Track> fetch_tracks(const std::string& search_query);

private:
    // Callback for CURL
    static size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* userp);

    // Helper function to extract tracks from HTML
    std::vector<Track> extractTracks(const std::string& html);
};

#endif // FETCH_HPP

