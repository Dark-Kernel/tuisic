#include <curl/curl.h>
#include <iostream>
#include <string>
#include <vector>
#include <regex>
#include <memory>
#include "../../common/Track.h"
#include <mpv/client.h>

// Performance tuning constants
namespace {
  constexpr size_t INITIAL_BUFFER_SIZE = 16384;
  constexpr size_t EXPECTED_TRACK_COUNT = 9;
}

class Lastfm {
    private:
        // Reusable CURL handle for better performance
        std::unique_ptr<CURL, decltype(&curl_easy_cleanup)> curl_handle{nullptr, curl_easy_cleanup};
        struct curl_slist *headers = nullptr;
        
        // Cached regex pattern for better performance
        static const std::regex& get_track_pattern() {
            static const std::regex pattern("chartlist-play-button[^>]*href=\"([^\"]+)\"[^>]*data-track-name=\"([^\"]+)\"[^>]*data-artist-name=\"([^\"]+)\"");
            return pattern;
        }

        void init_curl() {
            if (!curl_handle) {
                curl_handle.reset(curl_easy_init());
                if (curl_handle) {
                    // Set up headers once
                    headers = curl_slist_append(headers, "Accept: text/html,application/xhtml+xml,application/xml");
                    headers = curl_slist_append(headers, "Accept-Language: en-US,en;q=0.9");
                    
                    // Enable connection reuse and keep-alive
                    curl_easy_setopt(curl_handle.get(), CURLOPT_TCP_KEEPALIVE, 1L);
                    curl_easy_setopt(curl_handle.get(), CURLOPT_TCP_KEEPIDLE, 120L);
                    curl_easy_setopt(curl_handle.get(), CURLOPT_TCP_KEEPINTVL, 60L);
                    curl_easy_setopt(curl_handle.get(), CURLOPT_USERAGENT, "Mozilla/5.0");
                    curl_easy_setopt(curl_handle.get(), CURLOPT_FOLLOWLOCATION, 1L);
                    curl_easy_setopt(curl_handle.get(), CURLOPT_HTTPHEADER, headers);
                }
            }
        }

    public:
        Lastfm() {
            init_curl();
        }

        ~Lastfm() {
            if (headers) {
                curl_slist_free_all(headers);
                headers = nullptr;
            }
        }

        // Callback for CURL
        static size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* userp) {
            userp->append((char*)contents, size * nmemb);
            return size * nmemb;
        }

        // Function to extract tracks from HTML
        std::vector<Track> extractTracks(const std::string& html) {
            std::vector<Track> tracks;
            tracks.reserve(EXPECTED_TRACK_COUNT); // Pre-allocate expected size
            
            const auto& pattern = get_track_pattern();
            auto matches_begin = std::sregex_iterator(html.begin(), html.end(), pattern);
            auto matches_end = std::sregex_iterator();

            for (std::sregex_iterator i = matches_begin; i != matches_end && tracks.size() < EXPECTED_TRACK_COUNT; ++i) {
                std::smatch match = *i;
                Track track;
                track.url = match[1];
                track.name = match[2];
                track.artist = match[3];
                track.id = track.name;
                track.source = "lastfm";
                tracks.push_back(std::move(track));
            }

            return tracks;
        }

        // Main function to fetch tracks
        std::vector<Track> fetch_tracks(const std::string& search_query) {
            init_curl();
            std::string readBuffer;
            readBuffer.reserve(INITIAL_BUFFER_SIZE); // Pre-allocate reasonable buffer size
            std::vector<Track> tracks;

            if(curl_handle) {
                char* escaped = curl_easy_escape(curl_handle.get(), search_query.c_str(), search_query.length());
                std::string url = "https://www.last.fm/search?q=" + std::string(escaped);
                curl_free(escaped);

                curl_easy_setopt(curl_handle.get(), CURLOPT_URL, url.c_str());
                curl_easy_setopt(curl_handle.get(), CURLOPT_WRITEFUNCTION, WriteCallback);
                curl_easy_setopt(curl_handle.get(), CURLOPT_WRITEDATA, &readBuffer);

                CURLcode res = curl_easy_perform(curl_handle.get());

                if(res == CURLE_OK) {
                    tracks = extractTracks(readBuffer);
                }
            }

            return tracks;
        }

};


// int main (int argc, char *argv[]) {
//     Lastfm lastfm;
//     std::vector<Track> tracks = lastfm.fetch_tracks("kali kali zulfon");
//     for (const auto& track : tracks) {
//         std::cout << "Track: " << track.name << ", Artist: " << track.artist << ", URL: " << track.url << std::endl;
//     }
    
//     return 0;
// }
