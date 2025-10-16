#pragma once

#include "../common/Track.h"
#include "../core/config/config.hpp"
// #include "visualizer.hpp"
#include <algorithm>
#include <atomic>
#include <cstdlib>
#include <cstring>
#include <curl/curl.h>
#include <fmt/format.h>
#include <functional>
#include <iostream>
#include <memory>
#include <mpv/client.h>
#include <mutex>
#include <random>
#include <thread>

class MusicPlayer {
private:
  // std::shared_ptr<AudioVisualizer> visualizer;
  std::vector<double> audio_buffer;
  std::function<void(const std::vector<double> &)> on_audio_data;
    // std::vector<double> visualization_data;  // Store processed visualization data


  // Smart pointer with custom deleter for mpv handle
  std::unique_ptr<mpv_handle, decltype(&mpv_destroy)> mpv{nullptr, mpv_destroy};
  std::shared_ptr<Config> config;

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
  std::atomic_bool is_downloading{false};

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

  static size_t write_callback(void *ptr, size_t size, size_t nmemb,
                               FILE *stream) {
    return fwrite(ptr, size, nmemb, stream);
  }

  // Progress callback
  static int progress_callback(void *clientp, curl_off_t dltotal,
                               curl_off_t dlnow, curl_off_t ultotal,
                               curl_off_t ulnow) {
    // You can add progress tracking here if needed
    return 0; // Return non-zero to abort transfer
  }

public:
  MusicPlayer() {
    // Create MPV handle with error checking
    mpv.reset(mpv_create());
    if (!mpv) {
      log_error("Failed to create MPV instance");
      throw std::runtime_error("MPV initialization failed");
    }

    // Configure MPV options from config
    const std::vector<std::pair<std::string, std::string>> mpv_options = {
        {"video", "no"},
        {"audio-display", "no"}, 
        {"terminal", "no"},
        {"quiet", "yes"}, 
        {"sub-auto", "fuzzy"},   
        {"sub-codepage", "UTF-8"}
    };

    for (const auto &[option, default_value] : mpv_options) {
      // For now use defaults, but this allows easy config integration later
      if (mpv_set_option_string(mpv.get(), option.c_str(), default_value.c_str()) < 0) {
        log_error("Failed to set option: " + option);
      }
    }

    // visualizer = std::make_shared<AudioVisualizer>();

    // Property observation
    mpv_observe_property(mpv.get(), 0, "time-pos", MPV_FORMAT_DOUBLE);
    mpv_observe_property(mpv.get(), 0, "duration", MPV_FORMAT_DOUBLE);
    mpv_observe_property(mpv.get(), 0, "sub-text", MPV_FORMAT_STRING);
    mpv_observe_property(mpv.get(), 0, "audio-data", MPV_FORMAT_NODE);
    // Set audio output based on platform
#ifdef _WIN32
    mpv_set_option_string(mpv.get(), "ao", "wasapi");
#elif defined(__APPLE__)
    mpv_set_option_string(mpv.get(), "ao", "coreaudio");
#else
    mpv_set_option_string(mpv.get(), "ao", "pulse");
#endif


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

  /* MusicPlayer(std::shared_ptr<Config> cfg) : config(cfg) { */
  /*   set_volume(config->get_volume()); */

  /* } */

  // Destructor with RAII principles
  ~MusicPlayer() {
    running = false;
    if (event_thread && event_thread->joinable()) {
      event_thread->join();
    }
  }

  void set_audio_callback(
      std::function<void(const std::vector<double> &)> callback) {
    on_audio_data = std::move(callback);
  }

  // Subtitle management methods
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
    current_index = (current_index) % track_data.size();
    play(track_data[current_index].url);
  }

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

  void resume() {
    std::lock_guard<std::mutex> lock(player_mutex);
    if (is_paused) {
      const char *cmd[] = {"cycle", "pause", NULL};
      mpv_command_async(mpv.get(), 0, cmd);
      is_paused = false;
    }
  }

  void togglePlayPause() {
    std::lock_guard<std::mutex> lock(player_mutex);
    if (is_paused) {
      const char *cmd[] = {"cycle", "pause", NULL};
      mpv_command_async(mpv.get(), 0, cmd);
      is_paused = false;
    } else {
      const char *cmd[] = {"cycle", "pause", NULL};
      mpv_command_async(mpv.get(), 0, cmd);
      is_paused = !is_paused;
    }

  }

  std::string get_current_track() const {
    std::lock_guard<std::mutex> lock(player_mutex);
    return current_url;
  }
  
  std::string get_current_track_data() const {
    std::lock_guard<std::mutex> lock(player_mutex);
    return current_track_data[current_track_index].id;
  }

  void seek(double position) {
    std::lock_guard<std::mutex> lock(player_mutex);
    std::string pos = std::to_string(position);
    const char *cmd[] = {"seek", pos.c_str(), "absolute", NULL};
    mpv_command_async(mpv.get(), 0, cmd);
  }

  void skip_forward() {
    std::lock_guard<std::mutex> lock(player_mutex);
    const char *cmd[] = {"seek", "5", "relative", NULL};
    mpv_command_async(mpv.get(), 0, cmd);
  }

  void skip_backward() {
    std::lock_guard<std::mutex> lock(player_mutex);
    const char *cmd[] = {"seek", "-5", "relative", NULL};
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

  bool download_track(const std::string &url, const std::string path,
                      const std::string &filename) {
    std::lock_guard<std::mutex> lock(player_mutex);
    /* std::string final_path = config->get_download_path(); */

    std::string final_path = path + "/" + filename;
    /* std::string final_path = "/home/dk/Music/" + filename; */

    if (is_downloading) {
      log_error("Already another download in progress");
      return false;
    }

    // Start download in a separate thread
    std::thread download_thread([this, url, final_path]() {
      is_downloading = true;

      try {
        // Construct yt-dlp command
        // -x: Extract audio
        // --audio-format mp3: Convert to MP3
        // -o: Output template
        std::string command = "yt-dlp -x --audio-format mp3 -o \"" +
                              final_path + "\" \"" + url + "\"";

        /* std::string command = */
        /*     fmt::format("yt-dlp -x --audio-format {} -o \"{}\" \"{}\"", */
        /*                 config->get_download_format(), final_path, url); */

        // Execute the command
        int result = system(command.c_str());

        if (result != 0) {
          throw std::runtime_error("yt-dlp failed to download the track");
        }

        // Notify completion
        system(
            ("notify-send \"Download completed: " + final_path + "\"").c_str());

      } catch (const std::exception &e) {
        log_error(e.what());
        system(
            ("notify-send \"Download failed: " + std::string(e.what()) + "\"")
                .c_str());
      }

      is_downloading = false;
    });

    download_thread.detach();

    return true;
  }

  bool is_download_in_progress() const { return is_downloading; }

  void on_track_end() {
    std::lock_guard<std::mutex> lock(player_mutex);
    current_playlist_index++;
    if (current_playlist_index < playlist.size()) {
      // Play next track
      const char *cmd[] = {"loadfile", playlist[current_playlist_index].c_str(),
                           NULL};
      mpv_command(mpv.get(), cmd);
    }
  }

  void toggle_repeat() {
    std::lock_guard<std::mutex> lock(player_mutex);
    const char *cmd[] = {"cycle", "repeat", NULL};
    mpv_command_async(mpv.get(), 0, cmd);
  }

  int get_current_playlist_index() const {
    std::lock_guard<std::mutex> lock(player_mutex);
    return current_playlist_index;
  }

  void set_volume(int volume) {
    std::lock_guard<std::mutex> lock(player_mutex);
    int64_t mpv_volume = std::clamp(volume, 0, 100);
    mpv_set_property_async(mpv.get(), 0, "volume", MPV_FORMAT_INT64,
                           &mpv_volume);
  }

  int get_volume() const {
    std::lock_guard<std::mutex> lock(player_mutex);
    int64_t volume = 100;
    mpv_get_property(mpv.get(), "volume", MPV_FORMAT_INT64, &volume);
    return static_cast<int>(volume);
  }

  // std::vector<double> get_visualization_data() const {
  //   std::lock_guard<std::mutex> lock(player_mutex);
  //   return visualization_data;
// }

  // Callback setters

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
    if (strcmp(prop->name, "audio-data") == 0 &&
        prop->format == MPV_FORMAT_NODE) {
      auto *node = static_cast<mpv_node *>(prop->data);
      if (node->format == MPV_FORMAT_BYTE_ARRAY) {
        // Convert audio data to doubles
        auto *bytes = node->u.ba->data;
        size_t num_samples = node->u.ba->size / sizeof(float);
        audio_buffer.resize(num_samples);

        for (size_t i = 0; i < num_samples; i++) {
          audio_buffer[i] =
              static_cast<double>(reinterpret_cast<float *>(bytes)[i]);
        }

        // Process through visualizer
        // auto viz_data = visualizer->process(audio_buffer);
         {
            std::lock_guard<std::mutex> lock(player_mutex);
            // visualization_data = visualizer->process(audio_buffer);
        }
         // In the audio data handling section:
std::cerr << "Audio data received: " << num_samples << " samples" << std::endl;
// std::cerr << "Visualization data size: " << visualization_data.size() << std::endl;


        // Send to callback
        if (on_audio_data) {
          // on_audio_data(visualization_data);
        }
      }
    } else if (strcmp(prop->name, "time-pos") == 0 &&
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
      if (on_end_of_track_callback) {
        on_end_of_track_callback();
      }
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
  bool is_playing_state() const { return is_loaded && !is_paused; }
  bool is_paused_state() const { return is_loaded && is_paused; }
  double get_position() const { return current_position; }
  double get_duration() const { return duration; }
};
