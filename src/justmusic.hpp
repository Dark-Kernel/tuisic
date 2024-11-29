#ifndef JUSTMUSIC_HPP
#define JUSTMUSIC_HPP

#include <string>
#include <vector>
#include "Track.h"

// Forward declaration of the Justmusic class
class Justmusic {
public:
    // Function to fetch the content of a URL
    std::string fetchURL(const std::string &url);

    // Function to extract the track name from a URL
    std::string extractName(const std::string &url);

    // Function to extract MP3 URLs from HTML content
    std::vector<Track> extractMP3URL(const std::string &html);

    // Function to get MP3 URLs from a range of pages
    std::vector<Track> getMP3URL();

private:
    // Callback for CURL
    static size_t WriteCallback(void *contents, size_t size, size_t nmemb, std::string *userp);

    // Store the list of tracks
    std::vector<Track> tracks;
};

#endif // JUSTMUSIC_HPP

