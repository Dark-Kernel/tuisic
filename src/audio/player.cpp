#pragma once

#include "../common/Track.h"
#include "../core/config/config.hpp"
#include "../common/notification.hpp"
#include "lyrics_fetcher.hpp"
#ifdef WITH_CAVA
#include "visualizer.hpp"
#include "audio_capture.hpp"
#endif
#include <algorithm>
#include <atomic>
#include <cmath>
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

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

class MusicPlayer {
private:
#ifdef WITH_CAVA
  std::shared_ptr<AudioVisualizer> visualizer;
  std::shared_ptr<AudioCapture> audio_capture;
#endif
  std::vector<double> audio_buffer;
  std::function<void(const std::vector<double> &)> on_audio_data;
  std::vector<double> visualization_data;  // Store processed visualization data


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
  std::atomic_bool subtitles_enabled{true}; // Toggle for showing/hiding subtitles

  // Lyrics management
  std::unique_ptr<tuisic::LyricsFetcher> lyrics_fetcher;
  std::vector<tuisic::LyricLine> current_lyrics;
  std::atomic_bool has_lyrics{false};
  std::atomic_bool fetching_lyrics{false};

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
    // std::cerr << "[MusicPlayer Error] " << message << std::endl;
      notifications::send("[MusicPlayer Error] " + message);
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
  MusicPlayer() : lyrics_fetcher(std::make_unique<tuisic::LyricsFetcher>()) {
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

#ifdef WITH_CAVA
    try {
      visualizer = std::make_shared<AudioVisualizer>();
      audio_capture = std::make_shared<AudioCapture>();

      // Set up audio capture callback
      audio_capture->set_callback([this](const std::vector<double>& audio_data) {
        static int callback_count = 0;
        callback_count++;

        if (callback_count == 1) {
          std::cout << "[Player] Audio capture callback working! Received "
                    << audio_data.size() << " samples" << std::endl;
        }

        if (visualizer && on_audio_data) {
          // Process through visualizer
          std::vector<double> viz_data;
          {
            std::lock_guard<std::mutex> lock(player_mutex);
            viz_data = visualizer->process(audio_data);
            visualization_data = viz_data;
          }

          if (callback_count % 100 == 0) {
            std::cout << "[Player] Processed " << callback_count
                      << " buffers, viz_data size: " << viz_data.size() << std::endl;
          }

          // Send to UI callback
          if (!viz_data.empty()) {
            on_audio_data(viz_data);
          }
        } else {
          if (callback_count == 1) {
            std::cerr << "[Player] Visualizer or on_audio_data callback missing!" << std::endl;
          }
        }
      });

    } catch (const std::exception& e) {
      log_error(std::string("Failed to initialize visualizer: ") + e.what());
    }
#endif

    // Property observation
    mpv_observe_property(mpv.get(), 0, "time-pos", MPV_FORMAT_DOUBLE);
    mpv_observe_property(mpv.get(), 0, "duration", MPV_FORMAT_DOUBLE);
    mpv_observe_property(mpv.get(), 0, "sub-text", MPV_FORMAT_STRING);
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

    // Only update if subtitles are enabled
    if (subtitles_enabled) {
      current_subtitle = new_subtitle ? new_subtitle : "";
    } else {
      current_subtitle = "";
    }

    if (on_subtitle_change) {
      try {
        on_subtitle_change(current_subtitle);
      } catch (const std::exception &e) {
        log_error("Subtitle callback failed: " + std::string(e.what()));
      }
    }
  }

  void toggle_subtitles() {
    subtitles_enabled = !subtitles_enabled;

    // Clear subtitle immediately when disabling
    if (!subtitles_enabled) {
      std::lock_guard<std::mutex> lock(player_mutex);
      current_subtitle = "";
      if (on_subtitle_change) {
        try {
          on_subtitle_change(current_subtitle);
        } catch (const std::exception &e) {
          log_error("Subtitle callback failed: " + std::string(e.what()));
        }
      }
    }

    notifications::send(subtitles_enabled ? "Subtitles enabled" : "Subtitles disabled");
  }

  bool are_subtitles_enabled() const {
    return subtitles_enabled;
  }

  // Fetch lyrics asynchronously for the current track
  void fetch_lyrics_async() {
    if (current_track_index < 0 || current_track_index >= current_track_data.size()) {
      return;
    }

    if (fetching_lyrics.exchange(true)) {
      return; // Already fetching
    }

    // Get track info
    Track current_track = current_track_data[current_track_index];

    // Fetch in a separate thread to avoid blocking
    std::thread([this, current_track]() {
      try {
        auto lyrics_opt = lyrics_fetcher->fetch_lyrics(current_track.artist, current_track.name);

        if (lyrics_opt.has_value()) {
          auto parsed_lyrics = lyrics_fetcher->parse_lrc(lyrics_opt.value());

          std::lock_guard<std::mutex> lock(player_mutex);
          current_lyrics = std::move(parsed_lyrics);
          has_lyrics = !current_lyrics.empty();

          if (has_lyrics) {
            notifications::send("Lyrics loaded for: " + current_track.name);
          } else {
            notifications::send("No synced lyrics available for: " + current_track.name);
          }
        } else {
          notifications::send("Failed to fetch lyrics for: " + current_track.name);
        }
      } catch (const std::exception& e) {
        log_error("Lyrics fetch error: " + std::string(e.what()));
      }

      fetching_lyrics = false;
    }).detach();
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
        // std::cerr << "Failed to load first track. Error code: " << result
        //           << std::endl;
          notifications::send("Failed to load first track. Error code: " + std::to_string(result));
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

#ifdef WITH_CAVA
      // Start audio capture when playback begins
      if (audio_capture) {
        audio_capture->start();
      }
#endif
    }

    // If paused, unpause
    if (is_paused) {
      const char *cmd[] = {"cycle", "pause", NULL};
      mpv_command_async(mpv.get(), 0, cmd);
      is_paused = false;

#ifdef WITH_CAVA
      // Resume audio capture
      if (audio_capture) {
        audio_capture->start();
      }
#endif
    }
  }

  // Overload that accepts Track object for lyrics support
  void play(const Track &track) {
    {
      std::lock_guard<std::mutex> lock(player_mutex);
      // Update current track data for lyrics fetching
      current_track_data.clear();
      current_track_data.push_back(track);
      current_track_index = 0;
    }
    // Call the regular play method
    play(track.url);
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
        std::string command = "yt-dlp -q -x --audio-format mp3 -o \"" +
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
        // system(("notify-send \"Download completed: " + final_path + "\"").c_str());
        notifications::send_download_complete("" + final_path);

      } catch (const std::exception &e) {
        log_error(e.what());
        // system(
        //     ("notify-send \"Download failed: " + std::string(e.what()) + "\"")
        //         .c_str());
        notifications::send_download_failed("" + std::string(e.what()));
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

  std::vector<double> get_visualization_data() const {
    std::lock_guard<std::mutex> lock(player_mutex);
    return visualization_data;
  }

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
        // Priority: fetched lyrics > mpv subtitles
        if (has_lyrics && !current_lyrics.empty()) {
          // Use fetched lyrics synced with playback position
          double pos = get_position();
          std::string lyric_text;
          {
            std::lock_guard<std::mutex> lock(player_mutex);
            lyric_text = lyrics_fetcher->get_current_lyric(current_lyrics, pos);
          }
          if (!lyric_text.empty() && lyric_text != current_subtitle) {
            update_subtitle(lyric_text.c_str());
          }
        } else {
          // Fallback to mpv subtitles (for YouTube)
          char *sub_text = NULL;
          if (mpv_get_property(mpv.get(), "sub-text", MPV_FORMAT_STRING,
                               &sub_text) >= 0) {
            if (sub_text) {
              update_subtitle(sub_text);
              mpv_free(sub_text);
            }
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

    // Clear previous lyrics and subtitle, then fetch new ones
    {
      std::lock_guard<std::mutex> lock(player_mutex);
      current_lyrics.clear();
      has_lyrics = false;
      current_subtitle = "";

      // Notify UI to clear subtitle display
      if (on_subtitle_change) {
        try {
          on_subtitle_change(current_subtitle);
        } catch (const std::exception &e) {
          log_error("Subtitle callback failed: " + std::string(e.what()));
        }
      }
    }
    fetch_lyrics_async();

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
