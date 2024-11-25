#include <curl/curl.h>
#include <string>
#include <vector>
#include <regex>
#include "Track.h"

class Fetch {
    public:
        // Fetch(const Fetch &) = default;
        // Fetch(Fetch &&) = default;
        // Fetch &operator=(const Fetch &) = default;
        // Fetch &operator=(Fetch &&) = default;
        // Structure to hold track info
        // struct Track {
        //     std::string name;
        //     std::string artist;
        //     std::string url;

        //     // Convert to display string for FTXUI menu
        //     std::string to_string() const {
        //         return name + " - " + artist;
        //     }
        // };

        // Callback for CURL
        static size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* userp) {
            userp->append((char*)contents, size * nmemb);
            return size * nmemb;
        }

        // Function to extract tracks from HTML
        std::vector<Track> extractTracks(const std::string& html) {
            std::vector<Track> tracks;
            std::regex pattern("chartlist-play-button[^>]*href=\"([^\"]+)\"[^>]*data-track-name=\"([^\"]+)\"[^>]*data-artist-name=\"([^\"]+)\"");

            auto matches_begin = std::sregex_iterator(html.begin(), html.end(), pattern);
            auto matches_end = std::sregex_iterator();

            for (std::sregex_iterator i = matches_begin; i != matches_end && tracks.size() < 9; ++i) {
                std::smatch match = *i;
                Track track;
                track.url = match[1];
                track.name = match[2];
                track.artist = match[3];
                tracks.push_back(track);
            }

            return tracks;
        }

        // Main function to fetch tracks
        std::vector<Track> fetch_tracks(const std::string& search_query) {
            CURL* curl = curl_easy_init();
            std::string readBuffer;
            std::vector<Track> tracks;

            if(curl) {
                std::string url = "https://www.last.fm/search?q=";
                url += curl_easy_escape(curl, search_query.c_str(), search_query.length());

                struct curl_slist *headers = NULL;
                headers = curl_slist_append(headers, "Accept: text/html,application/xhtml+xml,application/xml");
                headers = curl_slist_append(headers, "Accept-Language: en-US,en;q=0.9");

                curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
                curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
                curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
                curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
                curl_easy_setopt(curl, CURLOPT_USERAGENT, "Mozilla/5.0");
                curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

                CURLcode res = curl_easy_perform(curl);

                if(res == CURLE_OK) {
                    tracks = extractTracks(readBuffer);
                }

                curl_easy_cleanup(curl);
                curl_slist_free_all(headers);
            }

            return tracks;
        }

};
