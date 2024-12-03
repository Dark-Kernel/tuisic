#include "Track.h"
#include <atomic>
#include <condition_variable>
#include <cstring>
#include <functional>
#include <iostream>
#include <memory>
#include <mpv/client.h>
#include <mutex>
#include <queue>
#include <thread>

using std::make_unique;
using std::string;

class MusicPlayer {
private:
  mpv_handle *mpv;
  std::thread event_thread;
  std::atomic<bool> running{true};
  std::atomic<bool> is_playing{false};
  std::atomic<double> current_position{0.0};
  std::atomic<double> duration{0.0};
  std::mutex player_mutex;

  std::string current_url;
  std::atomic<bool> is_paused{false};
  std::atomic<bool> is_loaded{false};

  // Playlist management
  std::vector<std::string> playlist;
  int current_playlist_index = -1;

  // Callback function for state changes
  std::function<void()> on_state_change;
  std::function<void(double, double)> on_time_update;
  std::function<void()> on_end_of_track_callback;

  std::atomic<std::string *> current_subtitle{new std::string("")};
  std::function<void(const std::string &)> on_subtitle_change;

public:
  MusicPlayer() {
    // Initialize mpv
    mpv = mpv_create();
    if (!mpv) {
      throw std::runtime_error("Failed to create MPV instance");
    }

    // Configure mpv
    mpv_set_option_string(mpv, "video", "no"); // Disable video
    mpv_set_option_string(mpv, "audio-display", "no");
    mpv_set_option_string(mpv, "terminal", "no");
    mpv_set_option_string(mpv, "quiet", "yes");
    // mpv_set_option_string(mpv, "sub-auto","fuzzy");
    // mpv_set_option_string(mpv, "sub-font-size", "40");
        const char *sub_auto = "fuzzy";
    mpv_set_property(mpv, "sub-auto", MPV_FORMAT_STRING, &sub_auto);

    mpv_observe_property(mpv, 0, "time-pos", MPV_FORMAT_DOUBLE);
    mpv_observe_property(mpv, 0, "duration", MPV_FORMAT_DOUBLE);
    mpv_observe_property(mpv, 0, "sub-text", MPV_FORMAT_STRING);

    // Initialize mpv
    if (mpv_initialize(mpv) < 0) {
      mpv_destroy(mpv);
      throw std::runtime_error("Failed to initialize MPV");
    }

    // Start event handling thread
    event_thread = std::thread([this] { event_loop(); });
  }

  ~MusicPlayer() {
    running = false;
    if (event_thread.joinable()) {
      event_thread.join();
    }
    if (mpv) {
      mpv_terminate_destroy(mpv);
    }
  }

  void update_subtitle(const char *new_subtitle) {
    // Ensure thread-safe update of the subtitle
    std::string *old_subtitle = current_subtitle.load();
    std::cerr << "Updating subtitle: " << new_subtitle << std::endl;
    current_subtitle.store(new std::string(new_subtitle ? new_subtitle : ""));
    delete old_subtitle;

    if (on_subtitle_change) {
      on_subtitle_change(*current_subtitle);
    }
  }

  void
  set_subtitle_callback(std::function<void(const std::string &)> callback) {
    on_subtitle_change = std::move(callback);
  }

  // Method to toggle subtitles
  void toggle_subtitles() {
    std::lock_guard<std::mutex> lock(player_mutex);
    const char *cmd[] = {"cycle", "sub", NULL};
    mpv_command_async(mpv, 0, cmd);
  }

  std::string get_current_subtitle() const {
    std::string *subtitle_ptr = current_subtitle.load();
    return *subtitle_ptr;
  }

  void create_playlist(const std::vector<std::string> &urls) {
    try {
      std::lock_guard<std::mutex> lock(player_mutex);

      if (urls.empty()) {
        std::cerr << "No URLs provided for playlist" << std::endl;
        return;
      }

      // Clear existing playlist
      playlist.clear();
      current_playlist_index = 0;

      // Clear MPV playlist
      mpv_command_string(mpv, "playlist-clear");

      // Simple playlist creation
      playlist = urls;

      // Attempt to play the first track
      if (!playlist.empty()) {
        const char *cmd[] = {"loadfile", playlist[0].c_str(), NULL};
        int result = mpv_command(mpv, cmd);

        if (result < 0) {
          std::cerr << "Failed to load first track. Error code: " << result
                    << std::endl;
          return;
        }
      }
    } catch (const std::exception &e) {
      std::cerr << "Exception in create_playlist: " << e.what() << std::endl;
    }
  }

  // New playlist methods

  void play_playlist() {
    if (playlist.empty())
      return;

    // If no track is currently selected, start from the beginning
    if (current_playlist_index == -1) {
      current_playlist_index = 0;
    }

    play(playlist[current_playlist_index]);
  }

  void next_trackk() {
    std::lock_guard<std::mutex> lock(player_mutex);

    if (playlist.empty())
      return;

    // Move to next track in playlist
    current_playlist_index = (current_playlist_index + 1) % playlist.size();

    // Load and play the next track
    const char *cmd[] = {"loadfile", playlist[current_playlist_index].c_str(),
                         NULL};
    mpv_command(mpv, cmd);
  }

  // void playlist_next_track(std::vector<Track>& track_data, int&
  // current_index) {
  //     std::lock_guard<std::mutex> lock(player_mutex);

  //     if (playlist.empty()) return;

  //     // Move to next track in playlist
  //     current_playlist_index = (current_playlist_index + 1) %
  //     playlist.size();

  //     // Update track_data index if possible
  //     if (current_playlist_index < track_data.size()) {
  //         current_index = current_playlist_index;
  //     }

  //     // Play the next track
  //     play(playlist[current_playlist_index]);
  // }

  // void playlist_previous_track(std::vector<Track>& track_data, int&
  // current_index) {
  //     std::lock_guard<std::mutex> lock(player_mutex);

  //     if (playlist.empty()) return;

  //     // Move to previous track in playlist
  //     current_playlist_index = (current_playlist_index - 1 + playlist.size())
  //     % playlist.size();

  //     // Update track_data index if possible
  //     if (current_playlist_index < track_data.size()) {
  //         current_index = current_playlist_index;
  //     }

  //     // Play the previous track
  //     play(playlist[current_playlist_index]);
  // }

  void play(const std::string &url) {
    std::lock_guard<std::mutex> lock(player_mutex);
    // If it's a new URL, load it
    if (url != current_url) {
      const char *cmd[] = {"loadfile", url.c_str(), NULL};
      mpv_command_async(mpv, 0, cmd);
      current_url = url;
      is_loaded = true;
      is_playing = true;
    }

    // If paused, unpause
    if (is_paused) {
      const char *cmd[] = {"cycle", "pause", NULL};
      mpv_command_async(mpv, 0, cmd);
      is_paused = false;
    }
    // const char* cmd[] = {"loadfile", url.c_str(), NULL};
    // mpv_command_async(mpv, 0, cmd);
    // is_playing = true;
    // if (on_state_change) on_state_change();
  }

  void pause() {
    std::lock_guard<std::mutex> lock(player_mutex);
    if (is_loaded) {
      const char *cmd[] = {"cycle", "pause", NULL};
      mpv_command_async(mpv, 0, cmd);
      is_paused = !is_paused;
    }
    // is_playing = !is_playing;
    // if (on_state_change) on_state_change();
  }

  void seek(double position) {
    std::lock_guard<std::mutex> lock(player_mutex);
    std::string pos = std::to_string(position);
    const char *cmd[] = {"seek", pos.c_str(), "absolute", NULL};
    mpv_command_async(mpv, 0, cmd);
  }

  void stop() {
    std::lock_guard<std::mutex> lock(player_mutex);
    const char *cmd[] = {"stop", NULL};
    mpv_command_async(mpv, 0, cmd);
    current_url.clear();
    is_loaded = false;
    is_playing = false;
    is_paused = false;
  }

  // // Add methods for previous and next track
  void previous_track(const std::vector<Track> &track_data,
                      int &current_index) {
    if (track_data.empty())
      return;

    // Circular previous
    current_index = (current_index - 1 + track_data.size()) % track_data.size();
    play(track_data[current_index].url);
  }

  void next_track(const std::vector<Track> &track_data, int &current_index) {
    if (track_data.empty())
      return;

    // Circular next
    // write data to file:

    current_index = (current_index + 1) % track_data.size();
    play(track_data[current_index].url);
  }

  void set_volume(int volume) {
    std::lock_guard<std::mutex> lock(player_mutex);
    mpv_set_property_async(mpv, 0, "volume", MPV_FORMAT_INT64, &volume);
  }

  bool is_playing_state() const {
    // return is_playing;
    return is_loaded && !is_paused;
  }

  double get_position() const { return current_position; }

  double get_duration() const { return duration; }

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
      mpv_event *event = mpv_wait_event(mpv, 0.1);
      if (event->event_id == MPV_EVENT_NONE) {
        continue;
      }

      switch (event->event_id) {
      case MPV_EVENT_PROPERTY_CHANGE: {
        mpv_event_property *prop = (mpv_event_property *)event->data;
        if (strcmp(prop->name, "time-pos") == 0 &&
            prop->format == MPV_FORMAT_DOUBLE) {
          current_position = *(double *)prop->data;
          if (on_time_update) {
            on_time_update(current_position, duration);
          }
        } else if (strcmp(prop->name, "duration") == 0 &&
                   prop->format == MPV_FORMAT_DOUBLE) {
          duration = *(double *)prop->data;

          
        } else if (strcmp(prop->name, "sub-text") == 0 && prop->format == MPV_FORMAT_STRING) {

          std::cerr << "sub-text raw: " << *(std::string*)prop->data << std::endl;
          if (prop->data) {
            std::cerr << "sub-text raw memory: "
                      << static_cast<const void *>(prop->data) << std::endl;
            std::cerr << "sub-text raw: "
                      << static_cast<const char *>(prop->data) << std::endl;
            update_subtitle(static_cast<const char *>(prop->data));
          } else {
            std::cerr << "sub-text is NULL!" << std::endl;
          }
        }
        break;
      }
      case MPV_EVENT_PLAYBACK_RESTART:
        is_playing = true;
        if (on_state_change)
          on_state_change();
        break;
      case MPV_EVENT_END_FILE: {
        // is_playing = false;
        // is_loaded = false;
        // is_paused = false;
        // if (on_state_change)
        //   on_state_change();
        // break;
        auto *prop = (mpv_event_end_file *)event->data;
        if (prop->reason == MPV_END_FILE_REASON_EOF) {
          next_trackk();
          // Automatically play next track if in playlist mode
          // if (!playlist.empty()) {
          //     current_playlist_index = (current_playlist_index + 1) %
          //     playlist.size(); play(playlist[current_playlist_index]);
          // }

          // // Trigger end of track callback
          // if (on_end_of_track_callback) {
          //     on_end_of_track_callback();
          // }
        }
        break;
      }

      case MPV_EVENT_FILE_LOADED: {
        // Ensure playback starts
        is_loaded = true;
        is_playing = true;
        is_paused = false;
        if (on_state_change)
          on_state_change();
        break;
      }
      }
    }
  }

  // Add playlist-related getters
  std::vector<std::string> get_playlist() const { return playlist; }
  int get_current_playlist_index() const { return current_playlist_index; }
};
