#pragma once

#include "Track.h"
#include <algorithm>
#include <atomic>
#include <condition_variable>
#include <cstring>
#include <fstream>
#include <functional>
#include <iostream>
#include <memory>
#include <mpv/client.h>
#include <mutex>
#include <queue>
#include <random>
#include <thread>

class MusicPlayer {
private:
  // Smart pointer with custom deleter for mpv handle
  std::unique_ptr<mpv_handle, decltype(&mpv_destroy)> mpv{nullptr, mpv_destroy};

  // Thread management with unique_ptr
  std::unique_ptr<std::thread> event_thread;

  // Atomic flags for thread-safe state management
  std::atomic_bool running{true};
  std::atomic_bool is_playing{false};
  std::atomic_bool is_paused{false};
  std::atomic_bool is_loaded{false};

  // Atomic properties with thread-safe access
  std::atomic<double> current_position{0.0};
  std::atomic<double> duration{0.0};

  // Mutex for thread-safe operations
  mutable std::mutex player_mutex;

  // Playlist management
  std::vector<std::string> playlist;
  std::atomic<int> current_playlist_index{-1};

  // Subtitle management - changed to avoid atomic<shared_ptr>
  std::string current_subtitle{""};

  // Callbacks
  std::function<void()> on_state_change;
  std::function<void(double, double)> on_time_update;
  std::function<void()> on_end_of_track_callback;
  std::function<void(const std::string &)> on_subtitle_change;

  // Current track data for navigation
  std::vector<Track> current_track_data;
  int current_track_index = -1;

  std::string current_url;

  // Logging utility
  void log_error(const std::string &message) {
    std::cerr << "[MusicPlayer Error] " << message << std::endl;
  }

public:
  MusicPlayer() {
    // Create MPV handle with error checking
    mpv.reset(mpv_create());
    if (!mpv) {
      log_error("Failed to create MPV instance");
      throw std::runtime_error("MPV initialization failed");
    }

    // Configure MPV options
    const std::vector<std::pair<const char *, const char *>> mpv_options = {
        {"video", "no"},  {"audio-display", "no"}, {"terminal", "no"},
        {"quiet", "yes"}, {"sub-auto", "fuzzy"},   {"sub-codepage", "UTF-8"}};

    for (const auto &[option, value] : mpv_options) {
      if (mpv_set_option_string(mpv.get(), option, value) < 0) {
        log_error("Failed to set option: " + std::string(option));
      }
    }

    // Property observation
    mpv_observe_property(mpv.get(), 0, "time-pos", MPV_FORMAT_DOUBLE);
    mpv_observe_property(mpv.get(), 0, "duration", MPV_FORMAT_DOUBLE);
    mpv_observe_property(mpv.get(), 0, "sub-text", MPV_FORMAT_STRING);
    mpv_set_property_string(mpv.get(), "sid", "1");
    mpv_request_event(mpv.get(), MPV_EVENT_TICK, true);

    // Initialize MPV
    if (mpv_initialize(mpv.get()) < 0) {
      log_error("MPV initialization failed");
      throw std::runtime_error("MPV initialization failed");
    }

    // Start event handling thread
    event_thread = std::make_unique<std::thread>([this] { event_loop(); });
  }

  // Destructor with RAII principles
  ~MusicPlayer() {
    running = false;
    if (event_thread && event_thread->joinable()) {
      event_thread->join();
    }
  }

  // Subtitle management methods
  // void update_subtitle(const char *new_subtitle) {
  //   current_subtitle = new_subtitle ? new_subtitle : "";
  //   // current_subtitle = new_subtitle ? std::string(new_subtitle) : "";
  //   // std::cerr << "Subtitle updated: " << current_subtitle << std::endl;
  //   // std::cerr.imbue(std::locale("en_US.UTF-8"));

  //   if (on_subtitle_change) {
  //     on_subtitle_change(current_subtitle);
  //   }
  // }

  void update_subtitle(const char *new_subtitle) {
    std::lock_guard<std::mutex> lock(player_mutex); // Thread-safe update
    current_subtitle = new_subtitle ? new_subtitle : "";
    if (on_subtitle_change) {
      try {
        on_subtitle_change(current_subtitle);
      } catch (const std::exception &e) {
        log_error("Subtitle callback failed: " + std::string(e.what()));
      }
    }
  }

  void toggle_subtitles() {
    std::lock_guard<std::mutex> lock(player_mutex);
    const char *cmd[] = {"cycle", "sub", NULL};
    mpv_command_async(mpv.get(), 0, cmd);
  }

  // std::string get_current_subtitle() const { return current_subtitle; }
  std::string get_current_subtitle() const {
    std::lock_guard<std::mutex> lock(player_mutex);
    return current_subtitle;
  }

  // Track navigation methods
  void previous_track(const std::vector<Track> &track_data,
                      int &current_index) {
    if (track_data.empty())
      return;

    // Update track data and current track index
    current_track_data = track_data;
    current_track_index = current_index;

    // Circular previous
    current_index = (current_index - 1 + track_data.size()) % track_data.size();
    play(track_data[current_index].url);
  }

  void next_track(const std::vector<Track> &track_data, int &current_index) {
    if (track_data.empty())
      return;

    // Update track data and current track index
    current_track_data = track_data;
    current_track_index = current_index;

    current_index = (current_index + 1) % track_data.size();
    play(track_data[current_index].url);
  }

  // Playlist management methods
  void create_playlist(const std::vector<std::string> &urls) {
    std::lock_guard<std::mutex> lock(player_mutex);

    if (urls.empty()) {
      log_error("No URLs provided for playlist");
      return;
    }

    // Clear and swap playlist
    playlist.clear();
    // playlist.swap(const_cast<std::vector<std::string> &>(urls));
    current_playlist_index = 0;
    playlist = urls;
    mpv_command_string(mpv.get(), "playlist-clear");
    if (!playlist.empty()) {

      const char *cmd[] = {"loadfile", playlist[0].c_str(), NULL};
      int result = mpv_command(mpv.get(), cmd);
      if (result < 0) {
        std::cerr << "Failed to load first track. Error code: " << result
                  << std::endl;
        return;
      }
    }

    // Load playlist
    // for (const auto &url : playlist) {
    //   const char *cmd[] = {"loadfile", url.c_str(), "append", NULL};
    //   mpv_command(mpv.get(), cmd);
    // }
  }

  void shuffle_playlist() {
    std::lock_guard<std::mutex> lock(player_mutex);
    if (playlist.size() > 1) {
      std::random_device rd;
      std::mt19937 g(rd());
      std::shuffle(playlist.begin(), playlist.end(), g);
      current_playlist_index = 0;
    }
  }

  void play_playlist() {
    if (playlist.empty())
      return;

    if (current_playlist_index == -1) {
      current_playlist_index = 0;
    }

    play(playlist[current_playlist_index]);
  }

  void next_track() {
    std::lock_guard<std::mutex> lock(player_mutex);

    if (playlist.empty())
      return;

    current_playlist_index = (current_playlist_index + 1) % playlist.size();
    const char *cmd[] = {"loadfile", playlist[current_playlist_index].c_str(),
                         NULL};
    mpv_command(mpv.get(), cmd);
  }

  void play(const std::string &url) {
    std::lock_guard<std::mutex> lock(player_mutex);
    if (url != current_url) {
      const char *cmd[] = {"loadfile", url.c_str(), NULL};
      mpv_command_async(mpv.get(), 0, cmd);
      current_url = url;
      is_loaded = true;
      is_playing = true;
    }

    // If paused, unpause
    if (is_paused) {
      const char *cmd[] = {"cycle", "pause", NULL};
      mpv_command_async(mpv.get(), 0, cmd);
      is_paused = false;
    }
  }

  void pause() {
    std::lock_guard<std::mutex> lock(player_mutex);
    if (is_loaded) {
      const char *cmd[] = {"cycle", "pause", NULL};
      mpv_command_async(mpv.get(), 0, cmd);
      is_paused = !is_paused;
    }
  }

  void resume(){
      std::lock_guard<std::mutex> lock(player_mutex);
      if (is_paused) {
        const char *cmd[] = {"cycle", "pause", NULL};
        mpv_command_async(mpv.get(), 0, cmd);
        is_paused = false;
      }
  }

  void seek(double position) {
    std::lock_guard<std::mutex> lock(player_mutex);
    std::string pos = std::to_string(position);
    const char *cmd[] = {"seek", pos.c_str(), "absolute", NULL};
    mpv_command_async(mpv.get(), 0, cmd);
  }

  void stop() {
    std::lock_guard<std::mutex> lock(player_mutex);
    const char *cmd[] = {"stop", NULL};
    mpv_command_async(mpv.get(), 0, cmd);

    is_loaded = false;
    is_playing = false;
    is_paused = false;
    current_playlist_index = -1;
  }

  // Callback setters
  // void
  // set_subtitle_callback(std::function<void(const std::string &)> callback) {
  //   on_subtitle_change = std::move(callback);
  // }
  void
  set_subtitle_callback(std::function<void(const std::string &)> callback) {
    if (!callback) {
      log_error("Invalid subtitle callback provided.");
      return;
    }
    std::lock_guard<std::mutex> lock(player_mutex);
    on_subtitle_change = std::move(callback);
  }

  void set_state_callback(std::function<void()> callback) {
    on_state_change = std::move(callback);
  }

  void set_time_callback(std::function<void(double, double)> callback) {
    on_time_update = std::move(callback);
  }

  void set_end_of_track_callback(std::function<void()> callback) {
    on_end_of_track_callback = std::move(callback);
  }

private:
  void event_loop() {
    while (running) {
      mpv_event *event = mpv_wait_event(mpv.get(), 0.1);
      if (event->event_id == MPV_EVENT_NONE) {
        continue;
      }

      switch (event->event_id) {
      case MPV_EVENT_PROPERTY_CHANGE: {
        auto *prop = static_cast<mpv_event_property *>(event->data);
        handle_property_change(prop);
        break;
      }
      case MPV_EVENT_PLAYBACK_RESTART:
        handle_playback_restart();
        break;
      case MPV_EVENT_END_FILE:
        handle_end_file(static_cast<mpv_event_end_file *>(event->data));
        break;
      case MPV_EVENT_FILE_LOADED:
        handle_file_loaded();
        break;
      case MPV_EVENT_TICK: {
        char *sub_text = NULL;
        if (mpv_get_property(mpv.get(), "sub-text", MPV_FORMAT_STRING,
                             &sub_text) >= 0) {
          if (sub_text) {
            update_subtitle(sub_text);
            mpv_free(sub_text);
          }
        }
        break;
      }
      }
    }
  }

  void handle_property_change(mpv_event_property *prop) {
    if (strcmp(prop->name, "time-pos") == 0 &&
        prop->format == MPV_FORMAT_DOUBLE) {
      current_position = *static_cast<double *>(prop->data);
      if (on_time_update) {
        on_time_update(current_position, duration);
      }
    } else if (strcmp(prop->name, "duration") == 0 &&
               prop->format == MPV_FORMAT_DOUBLE) {
      duration = *static_cast<double *>(prop->data);
    }
  }

  void handle_playback_restart() {
    is_playing = true;
    if (on_state_change) {
      on_state_change();
    }
  }

  void handle_end_file(mpv_event_end_file *prop) {
    if (prop->reason == MPV_END_FILE_REASON_EOF) {
      next_track();
    }
  }

  void handle_file_loaded() {
    is_loaded = true;
    is_playing = true;
    is_paused = false;
    if (on_state_change) {
      on_state_change();
    }
  }

public:
  // Getters
  bool is_playing_state() const { return is_loaded && !is_paused; }
  double get_position() const { return current_position; }
  double get_duration() const { return duration; }
  void set_volume(int volume) {
    std::lock_guard<std::mutex> lock(player_mutex);
    mpv_set_property_async(mpv.get(), 0, "volume", MPV_FORMAT_INT64, &volume);
  }
};
