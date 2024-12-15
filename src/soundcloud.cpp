#include <algorithm>
#include <curl/curl.h>
#include <iostream>
#include <string>
#include <vector>
#include <regex>
#include "Track.h"

class SoundCloud{
    public: 
        // struct SoundCloudTrack {
        //     std::string url;

        //     // Convert to display string for FTXUI menu
        //     std::string to_string() const {
        //         // Remove quotes and return cleaned URL
        //         std::string cleaned = url;
        //         cleaned.erase(std::remove(cleaned.begin(), cleaned.end(), '\"'), cleaned.end());
        //         return cleaned;
        //     }
        // };

        static size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* userp) {
            userp->append((char*)contents, size * nmemb);
            return size * nmemb;
        }

        // Function to extract tracks from search results
        std::vector<Track> extractTracks(const std::string& html) {
            std::vector<Track> tracks;
            // Regex pattern to match SoundCloud track URLs in search results
            // std::regex pattern("href=\"(/[^\"]*?)\"");
            std::regex pattern("<li><h2>.*?href=\"([^\"]*)\"");

            auto matches_begin = std::sregex_iterator(html.begin(), html.end(), pattern);
            auto matches_end = std::sregex_iterator();

            for (std::sregex_iterator i = matches_begin; i != matches_end; ++i) {
                std::smatch match = *i;
                Track track;
                track.url = "https://soundcloud.com" + match[1].str();
                // Extract name and artist from URL
                std::string path = match[1];
                size_t lastSlash = path.find_last_of('/');
                if (lastSlash != std::string::npos) {
                    track.artist = path.substr(1, lastSlash - 1);  // Remove leading /
                    track.name = path.substr(lastSlash + 1);
                    track.name = std::regex_replace(track.name, std::regex("-"), " "); // Replace - with spaces
                }
                tracks.push_back(track);
            }
            return tracks;
        }

        // Function to extract tracks from user profile
        std::vector<Track> extractUserTracks(const std::string& html) {
            std::vector<Track> tracks;
            // Regex pattern for user profile tracks
            std::regex pattern("itemprop=\"url\"[^>]*href=\"([^\"]*)\"");

            auto matches_begin = std::sregex_iterator(html.begin(), html.end(), pattern);
            auto matches_end = std::sregex_iterator();

            for (std::sregex_iterator i = matches_begin; i != matches_end; ++i) {
                std::smatch match = *i;
                std::cout << "URL: "<<match[1] << std::endl;
                Track track;
                track.url = match[1];
                tracks.push_back(track);
            }

            return tracks;
        }

        // Main function to fetch tracks from search
        std::vector<Track> fetch_tracks(const std::string& search_query, bool is_user_profile = false) {
            CURL* curl = curl_easy_init();
            std::string readBuffer;
            std::vector<Track> tracks;

            if(curl) {
                std::string url;
                if (is_user_profile) {
                    url = "https://soundcloud.com/" + search_query;
                } else {
                    url = "https://soundcloud.com/search?q=";
                    url += curl_easy_escape(curl, search_query.c_str(), search_query.length());
                }

                struct curl_slist *headers = NULL;
                headers = curl_slist_append(headers, "Accept: text/html,application/xhtml+xml,application/xml");
                headers = curl_slist_append(headers, "User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/88.0.4324.182 Safari/537.36");

                curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
                curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
                curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
                curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
                curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

                CURLcode res = curl_easy_perform(curl);

                if(res == CURLE_OK) {
                    tracks = is_user_profile ? extractUserTracks(readBuffer) : extractTracks(readBuffer);
                }

                curl_easy_cleanup(curl);
                curl_slist_free_all(headers);
            }

            return tracks;
        }

};

// int main (int argc, char *argv[]) {
//     // std::vector<SoundCloudTrack> tracks = fetch_soundcloud_tracks("https://soundcloud.com/bonesla");
//     SoundCloud fetch;
//     auto tracks = fetch.fetch_soundcloud_tracks("bones");
//     // auto user_tracks = fetch_soundcloud_tracks("bonesla", true);

//     for (const auto& track : tracks) {
//         std::cout << track.url << std::endl;
//     }

//     // for (const auto& track : user_tracks) {
//     //     std::cout << track.url << std::endl;
//     // }
//     return 0;
// }
