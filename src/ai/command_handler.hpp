#pragma once

#include <string>
#include <vector>
#include <sstream>
#include <memory>
#include "json_output.hpp"

// Forward declarations
class MusicPlayer;
class SoundCloud;
class Saavn;
struct Track;

namespace ai {

class CommandHandler {
private:
    std::shared_ptr<MusicPlayer> player;
    SoundCloud& soundcloud;
    Saavn& saavn;

    // Current playback state
    std::vector<Track> current_tracks;  // Fetched next tracks (like UI playlist)
    int current_index = -1;
    std::string current_track_name;
    std::string current_artist;
    std::string current_source;  // "saavn", "soundcloud", or "lastfm"

public:
    CommandHandler(
        std::shared_ptr<MusicPlayer> player_ptr,
        SoundCloud& sc,
        Saavn& sv
    ) : player(player_ptr), soundcloud(sc), saavn(sv) {}

    // Execute a command and return JSON response
    std::string execute(const std::string& command) {
        std::istringstream iss(command);
        std::string cmd;
        iss >> cmd;

        try {
            if (cmd == "play") {
                std::string query;
                std::getline(iss, query);
                query = trim(query);
                return handle_play(query);
            }
            else if (cmd == "pause") {
                return handle_pause();
            }
            else if (cmd == "resume") {
                return handle_resume();
            }
            else if (cmd == "next") {
                return handle_next();
            }
            else if (cmd == "previous") {
                return handle_previous();
            }
            else if (cmd == "stop") {
                return handle_stop();
            }
            else if (cmd == "search") {
                std::string query;
                std::getline(iss, query);
                query = trim(query);
                return handle_search(query);
            }
            else if (cmd == "status") {
                return handle_status();
            }
            else if (cmd == "volume") {
                int vol;
                iss >> vol;
                return handle_volume(vol);
            }
            else if (cmd == "seek") {
                double pos;
                iss >> pos;
                return handle_seek(pos);
            }
            else {
                return JsonOutput::create_error("Unknown command: " + cmd);
            }
        } catch (const std::exception& e) {
            return JsonOutput::create_error(std::string("Error: ") + e.what());
        }
    }

private:
    std::string handle_play(const std::string& query) {
        if (query.empty()) {
            // Resume if paused
            player->resume();
            return JsonOutput::create_success("Resumed playback");
        }

        // Search for the track
        std::vector<Track> search_results = saavn.fetch_tracks(query);
        if (search_results.empty()) {
            search_results = soundcloud.fetch_tracks(query);
        }

        if (search_results.empty()) {
            return JsonOutput::create_error("No results found for: " + query);
        }

        // Get first result
        Track selected_track = search_results[0];
        current_track_name = selected_track.name;
        current_artist = selected_track.artist;
        current_source = selected_track.source;

        // Fetch next tracks like the UI does (line 737-779 in main.cpp)
        std::vector<Track> next_tracks;
        if (selected_track.id != "" && selected_track.source != "lastfm") {
            try {
                if (selected_track.source == "soundcloud") {
                    next_tracks = soundcloud.fetch_next_tracks(selected_track.url);
                } else if (selected_track.source == "saavn") {
                    next_tracks = saavn.fetch_next_tracks(selected_track.id);
                }
            } catch (const std::exception& e) {
                // If fetching next tracks fails, just play the selected track
            }
        }

        // Build playlist: selected track + next tracks
        current_tracks.clear();
        current_tracks.push_back(selected_track);
        current_tracks.insert(current_tracks.end(), next_tracks.begin(), next_tracks.end());
        current_index = 0;

        // Create playlist URLs for MPV
        std::vector<std::string> playlist_urls;
        for (const auto& track : current_tracks) {
            playlist_urls.push_back(track.url);
        }

        // Use create_playlist like the UI does (line 779 in main.cpp)
        player->create_playlist(playlist_urls);

        return JsonOutput::create_success("Now playing: " + selected_track.name + " - " + selected_track.artist);
    }

    std::string handle_pause() {
        player->pause();
        return JsonOutput::create_success("Playback paused");
    }

    std::string handle_resume() {
        player->resume();
        return JsonOutput::create_success("Playback resumed");
    }

    std::string handle_next() {
        if (current_tracks.empty() || current_index < 0) {
            return JsonOutput::create_error("No active playlist");
        }

        // Move to next track in playlist
        current_index = (current_index + 1) % current_tracks.size();

        // Use player's next_track() method which handles MPV playlist navigation
        player->next_track();

        // Update current track info
        current_track_name = current_tracks[current_index].name;
        current_artist = current_tracks[current_index].artist;

        return JsonOutput::create_success("Playing next: " + current_track_name + " - " + current_artist);
    }

    std::string handle_previous() {
        if (current_tracks.empty() || current_index < 0) {
            return JsonOutput::create_error("No active playlist");
        }

        // Move to previous track in playlist
        current_index = (current_index - 1 + current_tracks.size()) % current_tracks.size();

        // Manually play the previous track (player doesn't have previous_track method)
        player->play(current_tracks[current_index].url);

        // Update current track info
        current_track_name = current_tracks[current_index].name;
        current_artist = current_tracks[current_index].artist;

        return JsonOutput::create_success("Playing previous: " + current_track_name + " - " + current_artist);
    }

    std::string handle_stop() {
        player->stop();
        return JsonOutput::create_success("Playback stopped");
    }

    std::string handle_search(const std::string& query) {
        auto tracks = saavn.fetch_tracks(query);
        if (tracks.empty()) {
            tracks = soundcloud.fetch_tracks(query);
        }

        return JsonOutput::create_search_results(tracks);
    }

    std::string handle_status() {
        std::string status = player->is_playing_state() ? "playing" :
                           player->is_paused_state() ? "paused" : "stopped";

        return JsonOutput::create_status(
            status,
            current_track_name,
            current_artist,
            player->get_position(),
            player->get_duration(),
            player->get_volume()
        );
    }

    std::string handle_volume(int vol) {
        if (vol < 0 || vol > 100) {
            return JsonOutput::create_error("Volume must be between 0 and 100");
        }

        player->set_volume(vol);
        return JsonOutput::create_success("Volume set to " + std::to_string(vol));
    }

    std::string handle_seek(double pos) {
        player->seek(pos);
        return JsonOutput::create_success("Seeked to " + std::to_string(pos) + " seconds");
    }

    // Utility function to trim whitespace
    static std::string trim(const std::string& str) {
        size_t first = str.find_first_not_of(' ');
        if (first == std::string::npos) return "";
        size_t last = str.find_last_not_of(' ');
        return str.substr(first, (last - first + 1));
    }
};

} // namespace ai
