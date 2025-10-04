#include "Track.h"
#include <curl/curl.h>
#include <iostream>
#include <mpv/client.h>
#include <rapidjson/document.h>
#include <string>
#include <vector>

class Saavn {
    public:
        // Callback for CURL
        static size_t WriteCallback(void *contents, size_t size, size_t nmemb,
                std::string *userp) {
            userp->append((char *)contents, size * nmemb);
            return size * nmemb;
        }

        std::vector<Track> extractNextTracks(const std::string &json) {
            std::vector<Track> tracks;
            rapidjson::Document doc;
            doc.Parse(json.c_str());
            for(const auto &result : doc.GetArray()){
                Track track;
                track.name = result["title"].GetString();
                track.id = result["id"].GetString();
                track.url = result["perma_url"].GetString();
                track.artist = result["more_info"]["artistMap"]["primary_artists"][0]["name"].GetString();
                track.source = "saavn";
                tracks.push_back(track);
            }
            // std::cout << "NEXT TRACKS \n" << tracks[0].name << std::endl;
            return tracks;
        }

        std::vector<Track> extractTrendingTracks(const std::string &json) {
            std::vector<Track> tracks;
            rapidjson::Document doc;
            doc.Parse(json.c_str());
            for(const auto &result : doc.GetArray()){
                Track track;
                track.name = result["title"].GetString();
                track.url = result["perma_url"].GetString();
                track.artist = result["more_info"]["artistMap"]["artists"][0]["name"].GetString();
                track.source = "saavn";
                tracks.push_back(track);
            }
            return tracks;
        }

        std::vector<Track> extractTracks(const std::string &json) {
            std::vector<Track> tracks;
            rapidjson::Document doc;
            doc.Parse(json.c_str());
            if (doc.HasMember("results") && doc["results"].IsArray()) {
                for (const auto &result : doc["results"].GetArray()) {
                    Track track;
                    if (result.HasMember("title") && result["title"].IsString()) {
                        track.name = result["title"].GetString();
                    }
                    if (result.HasMember("id") && result["id"].IsString()) {
                        track.id = result["id"].GetString();
                    }
                    if (result.HasMember("perma_url") && result["perma_url"].IsString()) {
                        track.url = result["perma_url"].GetString();
                    }
                    if (result.HasMember("more_info") && result["more_info"].IsObject()) {
                        const auto &more_info = result["more_info"];
                        if (more_info.HasMember("artistMap") &&
                                more_info["artistMap"].IsObject()) {
                            const auto &artistMap = more_info["artistMap"];
                            if (artistMap.HasMember("primary_artists") &&
                                    artistMap["primary_artists"].IsArray()) {
                                for (const auto &artist :
                                        artistMap["primary_artists"].GetArray()) {
                                    if (artist.HasMember("name") && artist["name"].IsString()) {
                                        /* track.artist.push_back(artist["name"].GetString()); */
                                        track.artist = artist["name"].GetString();
                                    }
                                }
                            }
                        }
                    }
                    track.source = "saavn";
                    tracks.push_back(track);
                }
            }
            return tracks;
        }

        std::string make_request(const std::string &url) {
            CURL *curl = curl_easy_init();
            std::string readBuffer;

            if (curl) {
                struct curl_slist *headers = NULL;
                headers = curl_slist_append(
                        headers, "Accept: text/html,application/xhtml+xml,application/xml");
                headers = curl_slist_append(headers, "Accept-Language: en-US,en;q=0.9");

                curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
                curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
                curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
                curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
                curl_easy_setopt(curl, CURLOPT_USERAGENT, "Mozilla/5.0");
                curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

                CURLcode res = curl_easy_perform(curl);
                if (res != CURLE_OK) {
                    fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
                    readBuffer = "Error";
                }
                curl_easy_cleanup(curl);
                curl_slist_free_all(headers);
            } else {
                readBuffer = "Error: Failed to initialize CURL.";
            }
            return readBuffer;
        }


        std::vector<Track> fetch_tracks(const std::string &search_query) {
            CURL *curl = curl_easy_init();
            if (!curl) {
                throw std::runtime_error("CURL initialization failed.");
            }
            std::string url = "https://www.jiosaavn.com/api.php?p=1&q=" +
                std::string(curl_easy_escape(curl, search_query.c_str(), search_query.length())) +
                "&_format=json&_marker=0&api_version=4&ctx=web6dot0&n=20&__call=search.getResults";
            curl_easy_cleanup(curl);

            std::string readBuffer = make_request(url);
            return extractTracks(readBuffer);
        }

        std::vector<Track> fetch_trending() {
            std::string url = "https://www.jiosaavn.com/api.php?__call=content.getTrending&api_version=4&_format=json&_marker=0&ctx=web6dot0&entity_type=album&entity_language=english";
            std::string readBuffer = make_request(url);
            return extractTrendingTracks(readBuffer);
        }

        std::vector<Track> fetch_next_tracks(std::string id) {
            CURL *curl = curl_easy_init();
            if (!curl) {
                throw std::runtime_error("CURL initialization failed.");
            }

            std::string url = "https://www.jiosaavn.com/api.php?__call=reco.getreco&api_version=4&_format=json&_marker=0&ctx=web6dot0&pid=" + std::string(curl_easy_escape(curl, id.c_str(), id.length()));
            // std::cout << url << std::endl;
            curl_easy_cleanup(curl);
            std::string readBuffer = make_request(url);
            // std::cout << readBuffer  << readBuffer.size()<< std::endl;

            if(readBuffer.size() == 2) {
                // std::cout << "in fi";
                std::string url = "https://www.jiosaavn.com/api.php?__call=content.getTrending&api_version=4&_format=json&_marker=0&ctx=web6dot0&entity_type=song&entity_language=english";
                curl_easy_cleanup(curl);
                std::string readBuffer = make_request(url);
                return extractTrendingTracks(readBuffer);
            }
            return extractNextTracks(readBuffer);
        }


};

//int main(int argc, char *argv[]) {
//    Saavn saavn;
//    //auto tracks = saavn.fetch_tracks("sugarcane");
//    std::string search_query = "bones";
//    std::vector<Track> tracks = saavn.fetch_tracks(search_query);

//    for (const auto &track : tracks) {
//        std::cout << "Title: " << track.name << "\n";
//        std::cout << "URL: " << track.url << "\n";
//        std::cout << "Primary Artists: " << track.artist << "\n";
//         for (const auto& artist : track.artist) {
//             std::cout << artist << "";
//         }
//        std::cout << "\n\n";
//    }
//    std::cout << "NEXT TRACKS \n";
//    std::vector<Track> next_tracks = saavn.fetch_next_tracks(tracks[0].id);

//    for (const auto &track : next_tracks) {
//        std::cout << "Title: " << track.name << "\n";
//        std::cout << "URL: " << track.url << "\n";
//        std::cout << "Primary Artists: " << track.artist << "\n";
//         for (const auto& artist : track.artist) {
//             std::cout << artist << "";
//         }
//        std::cout << "\n\n";
//    }
//    return 0;
//   // return 0;
//}
