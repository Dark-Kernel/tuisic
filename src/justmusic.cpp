#include "Track.h"
#include <curl/curl.h>
#include <iostream>
#include <regex>
#include <string>
#include <vector>

class Justmusic {
public:
  static size_t WriteCallback(void *contents, size_t size, size_t nmemb,
                              std::string *userp) {
    userp->append((char *)contents, size * nmemb);
    return size * nmemb;
  };

  std::string fetchURL(const std::string &url) {
    CURL *curl;
    CURLcode res;
    std::string content;

    curl = curl_easy_init();
    if (curl) {
      curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
      curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
      curl_easy_setopt(curl, CURLOPT_WRITEDATA, &content);
      res = curl_easy_perform(curl);
      curl_easy_cleanup(curl);
    }
    return content;
  }

  std::string extractName(const std::string &url) {
    std::regex pattern(R"(/forest/(.+?)\.mp3)");
    std::smatch match;

    if (std::regex_search(url, match, pattern)) {
      return match[1];
    }
    return "";
  }
  std::vector<Track> extractMP3URL(const std::string &html) {

    std::vector<Track> tracks;
    std::regex pattern(R"(https://newnow\.cool/forest.*?\.mp3)");
    std::smatch match;

    // auto matches_begin = std::sregex_iterator(html.begin(), html.end(),
    // pattern); auto matches_end = std::sregex_iterator();
    if (std::regex_search(html, match, pattern)) {
      // std::cout << match.str() << std::endl;
      Track track;
      track.url = match.str();
      track.name = extractName(track.url);
      track.artist = "Forest FM";
      tracks.push_back(track);
      // return match.str();
    }

    // for (std::sregex_iterator i = matches_begin; i != matches_end &&
    // tracks.size() < 9; ++i) {
    //     std::smatch match = *i;
    //     Track track;
    //     std::cout << "Match: "<<match[1] << std::endl;
    //     track.url = match[1];
    //     track.name = extractName(track.url);
    //     track.artist = "Forest FM";
    //     tracks.push_back(track);
    // }
    return tracks;
  }

  std::vector<Track> tracks;

  std::vector<Track> getMP3URL() {
    for (int i = 40; i <= 65; i++) {
      std::string url = "https://www.tree.fm/forest/" + std::to_string(i);
      std::string html = fetchURL(url);
      // tracks.clear();
      std::vector<Track> temp_track = extractMP3URL(html);
      // for (auto tac: temp_track) {
      //     std::cout <<"tac"<< tac.url << std::endl;
      // }
      for (auto track : temp_track) {
        if (!temp_track.empty()) {
          tracks.push_back(track);
          // std::cout << tracks.back().url << std::endl;
        } else {
          std::cerr << "Not found at " << i << std::endl;
        }
      }
    }
    return tracks;
  }
};

// int main(int argc, char *argv[]) {
//     // auto player = std::make_shared<MusicPlayer>();
//     //     player->play("https://newnow.cool/forest/Soundsof.mp3");
//   Justmusic justmusic;
//   justmusic.getMP3URL();
//   return 0;
// }
