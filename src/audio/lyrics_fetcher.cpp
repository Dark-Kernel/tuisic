#include "lyrics_fetcher.hpp"
#include <curl/curl.h>
#include <sstream>
#include <algorithm>

namespace tuisic {

LyricsFetcher::LyricsFetcher() {
    curl_global_init(CURL_GLOBAL_DEFAULT);
}

LyricsFetcher::~LyricsFetcher() {
    curl_global_cleanup();
}

size_t LyricsFetcher::write_callback(void* contents, size_t size, size_t nmemb, void* userp) {
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

std::string LyricsFetcher::url_encode(const std::string& value) {
    CURL* curl = curl_easy_init();
    if (!curl) return value;

    char* encoded = curl_easy_escape(curl, value.c_str(), value.length());
    std::string result(encoded);
    curl_free(encoded);
    curl_easy_cleanup(curl);

    return result;
}

std::optional<std::string> LyricsFetcher::fetch_lyrics(const std::string& artist, const std::string& track_name) {
    if (artist.empty() || track_name.empty()) {
        return std::nullopt;
    }

    CURL* curl = curl_easy_init();
    if (!curl) {
        return std::nullopt;
    }

    std::string response;
    std::string url = "https://lrclib.net/api/get?artist_name=" +
                      url_encode(artist) + "&track_name=" + url_encode(track_name);

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);

    // Set user agent
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "tuisic/1.0");

    CURLcode res = curl_easy_perform(curl);
    long http_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK || http_code != 200) {
        return std::nullopt;
    }

    // Parse JSON response to extract syncedLyrics or plainLyrics
    // Look for "syncedLyrics" field first
    size_t synced_pos = response.find("\"syncedLyrics\":");
    if (synced_pos != std::string::npos) {
        size_t start = response.find("\"", synced_pos + 15);
        if (start != std::string::npos) {
            start++; // Move past the opening quote
            size_t end = start;
            // Find the closing quote, skipping escaped quotes
            while (end < response.length()) {
                if (response[end] == '"' && (end == start || response[end-1] != '\\')) {
                    break;
                }
                end++;
            }
            if (end < response.length()) {
                std::string lyrics = response.substr(start, end - start);
                // Unescape JSON string
                size_t pos = 0;
                while ((pos = lyrics.find("\\n", pos)) != std::string::npos) {
                    lyrics.replace(pos, 2, "\n");
                    pos += 1;
                }
                pos = 0;
                while ((pos = lyrics.find("\\\"", pos)) != std::string::npos) {
                    lyrics.replace(pos, 2, "\"");
                    pos += 1;
                }
                pos = 0;
                while ((pos = lyrics.find("\\\\", pos)) != std::string::npos) {
                    lyrics.replace(pos, 2, "\\");
                    pos += 1;
                }
                if (!lyrics.empty() && lyrics != "null") {
                    return lyrics;
                }
            }
        }
    }

    // Fallback to plain lyrics
    size_t plain_pos = response.find("\"plainLyrics\":");
    if (plain_pos != std::string::npos) {
        size_t start = response.find("\"", plain_pos + 14);
        if (start != std::string::npos) {
            start++; // Move past the opening quote
            size_t end = start;
            // Find the closing quote, skipping escaped quotes
            while (end < response.length()) {
                if (response[end] == '"' && (end == start || response[end-1] != '\\')) {
                    break;
                }
                end++;
            }
            if (end < response.length()) {
                std::string lyrics = response.substr(start, end - start);
                // Unescape JSON string
                size_t pos = 0;
                while ((pos = lyrics.find("\\n", pos)) != std::string::npos) {
                    lyrics.replace(pos, 2, "\n");
                    pos += 1;
                }
                pos = 0;
                while ((pos = lyrics.find("\\\"", pos)) != std::string::npos) {
                    lyrics.replace(pos, 2, "\"");
                    pos += 1;
                }
                pos = 0;
                while ((pos = lyrics.find("\\\\", pos)) != std::string::npos) {
                    lyrics.replace(pos, 2, "\\");
                    pos += 1;
                }
                if (!lyrics.empty() && lyrics != "null") {
                    return lyrics;
                }
            }
        }
    }

    return std::nullopt;
}

std::vector<LyricLine> LyricsFetcher::parse_lrc(const std::string& lrc_content) {
    std::vector<LyricLine> lyrics;

    // LRC format: [mm:ss.xx]Lyric text
    std::istringstream stream(lrc_content);
    std::string line;

    while (std::getline(stream, line)) {
        // Find timestamp pattern [mm:ss.xx]
        if (line.empty() || line[0] != '[') continue;

        size_t close_bracket = line.find(']');
        if (close_bracket == std::string::npos) continue;

        std::string timestamp_str = line.substr(1, close_bracket - 1);
        std::string text = line.substr(close_bracket + 1);

        // Parse timestamp mm:ss.xx
        size_t colon_pos = timestamp_str.find(':');
        size_t dot_pos = timestamp_str.find('.');

        if (colon_pos == std::string::npos || dot_pos == std::string::npos) continue;

        try {
            int minutes = std::stoi(timestamp_str.substr(0, colon_pos));
            int seconds = std::stoi(timestamp_str.substr(colon_pos + 1, dot_pos - colon_pos - 1));
            int centiseconds = std::stoi(timestamp_str.substr(dot_pos + 1));

            double timestamp = minutes * 60.0 + seconds + centiseconds / 100.0;

            if (!text.empty()) {
                lyrics.emplace_back(timestamp, text);
            }
        } catch (const std::exception&) {
            // Skip malformed lines
            continue;
        }
    }

    // Sort by timestamp
    std::sort(lyrics.begin(), lyrics.end(),
              [](const LyricLine& a, const LyricLine& b) {
                  return a.timestamp < b.timestamp;
              });

    return lyrics;
}

std::string LyricsFetcher::get_current_lyric(const std::vector<LyricLine>& lyrics, double current_time) {
    if (lyrics.empty()) {
        return "";
    }

    // Find the lyric line that should be displayed at current_time
    for (size_t i = lyrics.size(); i > 0; --i) {
        if (lyrics[i - 1].timestamp <= current_time) {
            return lyrics[i - 1].text;
        }
    }

    return "";
}

} // namespace tuisic
