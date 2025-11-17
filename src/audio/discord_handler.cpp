#pragma once
#include <discord_rpc.h>
#include <functional>
#include <memory>
#include <string>
#include <thread>
#include <chrono>
#include <atomic>

class DiscordRPCHandler {
private:
  std::string client_id;
  std::atomic<bool> is_initialized{false};

  // Track data callbacks
  std::function<std::string()> get_track_name;
  std::function<std::string()> get_track_artist;
  std::function<bool()> is_playing;

public:
  DiscordRPCHandler(const std::string& app_client_id)
      : client_id(app_client_id) {}

  ~DiscordRPCHandler() {
    cleanup();
  }

  void initialize() {
    if (is_initialized) return;

    DiscordEventHandlers handlers{};
    handlers.ready = [](const DiscordUser* user) {};
    handlers.disconnected = [](int errorCode, const char* message) {};
    handlers.errored = [](int errorCode, const char* message) {};

    Discord_Initialize(client_id.c_str(), &handlers, 1, nullptr);
    is_initialized = true;
  }

  void setTrackCallbacks(
      std::function<std::string()> track_name_cb,
      std::function<std::string()> track_artist_cb,
      std::function<bool()> is_playing_cb) {
    get_track_name = track_name_cb;
    get_track_artist = track_artist_cb;
    is_playing = is_playing_cb;
  }

  void updatePresence() {
    if (!is_initialized || !get_track_name || !get_track_artist) {
      return;
    }

    std::string track_name = get_track_name();
    std::string track_artist = get_track_artist();

    if (track_name.empty()) {
      // Clear presence if no track is playing
      Discord_ClearPresence();
      return;
    }

    DiscordRichPresence presence{};
    presence.details = track_name.c_str();
    presence.state = track_artist.c_str();
    presence.largeImageKey = "default";
    presence.largeImageText = "tuisic - TUI Music Player";

    // Optional: Add playback state icon
    if (is_playing && is_playing()) {
      presence.smallImageKey = "playing";
      presence.smallImageText = "Playing";
    } else {
      presence.smallImageKey = "paused";
      presence.smallImageText = "Paused";
    }

    Discord_UpdatePresence(&presence);

    // Run callbacks to maintain connection
    Discord_RunCallbacks();
  }

  void cleanup() {
    if (is_initialized) {
      Discord_ClearPresence();
      Discord_Shutdown();
      is_initialized = false;
    }
  }

  void notifyTrackChange() {
    // Immediately update presence when track changes
    updatePresence();
  }

  void notifyPlaybackChange() {
    // Update presence when playback state changes (play/pause)
    updatePresence();
  }
};

// TUI Integration wrapper
class TUIDiscordIntegration {
private:
  std::unique_ptr<DiscordRPCHandler> discord_handler;
  std::vector<Track> &track_data;
  std::vector<Track> &next_tracks;
  int &current_track_index;
  std::shared_ptr<MusicPlayer> player;

public:
  TUIDiscordIntegration(
      std::vector<Track> &tracks,
      std::vector<Track> &next_track_list,
      int &track_index,
      std::shared_ptr<MusicPlayer> player_instance)
      : track_data(tracks),
        next_tracks(next_track_list),
        current_track_index(track_index),
        player(player_instance) {}

  void setup(const std::string& client_id) {
    discord_handler = std::make_unique<DiscordRPCHandler>(client_id);
    discord_handler->initialize();

    discord_handler->setTrackCallbacks(
        [this]() -> std::string {
          // Use next_tracks if available, otherwise fall back to track_data
          auto &source = !next_tracks.empty() ? next_tracks : track_data;
          return (source.empty() || current_track_index >= source.size())
                     ? ""
                     : source[current_track_index].name;
        },
        [this]() -> std::string {
          // Use next_tracks if available, otherwise fall back to track_data
          auto &source = !next_tracks.empty() ? next_tracks : track_data;
          return (source.empty() || current_track_index >= source.size())
                     ? ""
                     : source[current_track_index].artist;
        },
        [this]() -> bool {
          return player && player->is_playing_state();
        });

    // Initial presence update
    discord_handler->updatePresence();
  }

  void notifyTrackChange() {
    if (discord_handler) {
      discord_handler->notifyTrackChange();
    }
  }

  void notifyPlaybackChange() {
    if (discord_handler) {
      discord_handler->notifyPlaybackChange();
    }
  }

  ~TUIDiscordIntegration() {
    // Destructor handles cleanup automatically
  }
};
