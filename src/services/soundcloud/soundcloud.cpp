#include <algorithm>
#include <cstdint>
#include <curl/curl.h>
#include <iostream>
#include <string>
#include <vector>
#include <regex>
#include <rapidjson/document.h>
#include "../../common/Track.h"

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
                    track.id = track.name;
                    track.name = std::regex_replace(track.name, std::regex("-"), " "); // Replace - with spaces
                    track.source = "soundcloud";
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

        std::string fetch_url(const std::string& url) {
            std::string readBuffer;
            CURL* curl = curl_easy_init();
            if (!curl) return "";

            struct curl_slist *headers = NULL;
            headers = curl_slist_append(headers, "User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36");

            curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
            curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
            curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

            CURLcode res = curl_easy_perform(curl);
            if (res != CURLE_OK) {
                fprintf(stderr, "fetch_url failed: %s\n", curl_easy_strerror(res));
            }

            curl_easy_cleanup(curl);
            curl_slist_free_all(headers);
            return readBuffer;
        }

        std::string get_client_id() {
            static std::string cid;
            if (!cid.empty()) return cid;                       // cache
            std::string home, js;                               // buffers

            std::string url = "https://soundcloud.com";
            home = fetch_url(url);

            // 2. locate the first “0-*.js”
            std::regex r_script("src=\"(https://[^\"]+/0-[^\"]+?\\.js)\"");
            std::smatch m;
            if (!std::regex_search(home, m, r_script)) return "";
            js = fetch_url(m[1].str());

            // 3. pick the 32-char token
            std::regex r_id("client_id\\s*:\\s*\"([a-zA-Z0-9]{32})\"");
            if (!std::regex_search(js, m, r_id)) return "";
            cid = m[1];
            return cid;
        }

        std::string resolve_id(const std::string& url) {
            std::string api = "https://api-v2.soundcloud.com/resolve?url=";

            CURL* curl = curl_easy_init();
            char* escaped = curl_easy_escape(curl, url.c_str(), url.length());
            api += escaped;
            curl_free(escaped);
            curl_easy_cleanup(curl);

            api += "&client_id=" + get_client_id();
            std::string j = fetch_url(api);

            // load with rapidjson
            rapidjson::Document document;
            document.Parse(j.c_str());
            if (document.HasParseError()) {
                std::cerr << "JSON parsing error: " << document.GetParseError() << std::endl;
            }

            if (document.HasMember("id") && document["id"].IsInt64()) {
                int64_t id_num = document["id"].GetInt64();
                return std::to_string(id_num);
            }

            return "";
        }

        std::vector<Track> fetch_next_tracks(std::string url, int limit = 10) {
            std::string id = resolve_id(url);
            std::cout << id << std::endl;
            std::vector<Track> next_tracks;
            std::string j;
            uint32_t anon = 10000000 + (std::rand() % 89999999);   // eight-digit anon_user_id
            std::string api = "https://api-v2.soundcloud.com/tracks/" +
                id +
                "/related?client_id=" + get_client_id() +
                "&anon_user_id=" + std::to_string(anon) +
                "&limit=" + std::to_string(limit) +
                "&offset=0&linked_partitioning=1";
            j =fetch_url(api);

            rapidjson::Document document;
            document.Parse(j.c_str());

            if(document.HasParseError()) {
                std::cerr << "JSON parsing error: " << document.GetParseError() << std::endl;
            }

            if(document.HasMember("collection") && document["collection"].IsArray()) {
                for(const auto &result : document["collection"].GetArray()) {
                    Track track;
                    if(result.HasMember("id") && result["id"].IsInt64()) {
                        track.id = std::to_string(result["id"].GetInt64());
                    }
                    track.name = result["title"].GetString();
                    track.url = result["permalink_url"].GetString();
                    track.artist = result["user"]["username"].GetString();
                    track.source = "soundcloud";
                    next_tracks.push_back(track);
                }
            }
            return next_tracks;
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
//     auto tracks = fetch.fetch_tracks("meeting after a decade");
//     // auto user_tracks = fetch_soundcloud_tracks("bonesla", true);

//     for (const auto& track : tracks) {
//         std::cout << track.url << std::endl;
//     }
//     auto user_tracks = fetch.fetch_next_tracks("https://soundcloud.com/rjpp2ovwcy3r/imagine-dragons-bones-5");

//     // for (const auto& track : tracks) {
//     //     std::cout << track.id << std::endl;
//     // }


//     // for (const auto& track : user_tracks) {
//     //     std::cout << track.url << std::endl;
//     // }
//     return 0;
// }

