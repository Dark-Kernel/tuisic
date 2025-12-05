#pragma once

#include <string>
#include <vector>
#include <optional>
#include <memory>

namespace tuisic {

struct LyricLine {
    double timestamp; // Time in seconds
    std::string text;

    LyricLine(double ts, std::string txt) : timestamp(ts), text(std::move(txt)) {}
};

class LyricsFetcher {
public:
    LyricsFetcher();
    ~LyricsFetcher();

    // Fetch lyrics from LRCLIB API
    // Returns synced lyrics if available, otherwise returns plain lyrics
    std::optional<std::string> fetch_lyrics(const std::string& artist, const std::string& track_name);

    // Parse LRC format lyrics into timestamped lines
    std::vector<LyricLine> parse_lrc(const std::string& lrc_content);

    // Get the current lyric line based on playback position
    std::string get_current_lyric(const std::vector<LyricLine>& lyrics, double current_time);

private:
    static size_t write_callback(void* contents, size_t size, size_t nmemb, void* userp);
    std::string url_encode(const std::string& value);
};

} // namespace tuisic
