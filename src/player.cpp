#include <cstring>
#include <functional>
#include <mpv/client.h>
#include <thread>
#include <memory>
#include <queue>
#include <mutex>
#include <atomic>
#include <iostream>
#include "Track.h"

class MusicPlayer {
    private:
        mpv_handle* mpv;
        std::thread event_thread;
        std::atomic<bool> running{true};
        std::atomic<bool> is_playing{false};
        std::atomic<double> current_position{0.0};
        std::atomic<double> duration{0.0};
        std::mutex player_mutex;

        std::string current_url;
        std::atomic<bool> is_paused{false};
        std::atomic<bool> is_loaded{false};

        // Callback function for state changes
        std::function<void()> on_state_change;
        std::function<void(double, double)> on_time_update;
        std::function<void()> on_end_of_track_callback;

    public:
        MusicPlayer() {
            // Initialize mpv
            mpv = mpv_create();
            if (!mpv) {
                throw std::runtime_error("Failed to create MPV instance");
            }

            // Configure mpv
            mpv_set_option_string(mpv, "video", "no");  // Disable video
            mpv_set_option_string(mpv, "audio-display", "no");
            mpv_set_option_string(mpv, "terminal", "no");
            mpv_set_option_string(mpv, "quiet", "yes");
            mpv_observe_property(mpv, 0, "time-pos", MPV_FORMAT_DOUBLE);
            mpv_observe_property(mpv, 0, "duration", MPV_FORMAT_DOUBLE);

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

        void play(const std::string& url) {
            std::lock_guard<std::mutex> lock(player_mutex);
            // If it's a new URL, load it
            if (url != current_url) {
                const char* cmd[] = {"loadfile", url.c_str(), NULL};
                mpv_command_async(mpv, 0, cmd);
                current_url = url;
                is_loaded = true;
                is_playing = true;
            }

            // If paused, unpause
            if (is_paused) {
                const char* cmd[] = {"cycle", "pause", NULL};
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
                const char* cmd[] = {"cycle", "pause", NULL};
                mpv_command_async(mpv, 0, cmd);
                is_paused = !is_paused;
            }
            // is_playing = !is_playing;
            // if (on_state_change) on_state_change();
        }

        void seek(double position) {
            std::lock_guard<std::mutex> lock(player_mutex);
            std::string pos = std::to_string(position);
            const char* cmd[] = {"seek", pos.c_str(), "absolute", NULL};
            mpv_command_async(mpv, 0, cmd);
        }

        void stop() {
            std::lock_guard<std::mutex> lock(player_mutex);
            const char* cmd[] = {"stop", NULL};
            mpv_command_async(mpv, 0, cmd);
            current_url.clear();
            is_loaded = false;
            is_playing = false;
            is_paused = false;
        }

        // Add methods for previous and next track
        void previous_track(const std::vector<Track>& track_data, int& current_index) {
            if (track_data.empty()) return;

            // Circular previous
            current_index = (current_index - 1 + track_data.size()) % track_data.size();
            play(track_data[current_index].url);
        }

        void next_track(const std::vector<Track>& track_data, int& current_index) {
            if (track_data.empty()) std::cout <<"Retinngnng"; return;

            // Circular next
            std::cout << "Retinngnngddddddddddddddddddddddddddddd";
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

        double get_position() const {
            return current_position;
        }

        double get_duration() const {
            return duration;
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
                mpv_event* event = mpv_wait_event(mpv, 0.1);
                if (event->event_id == MPV_EVENT_NONE) {
                    continue;
                }

                switch (event->event_id) {
                    case MPV_EVENT_PROPERTY_CHANGE: {
                                                        mpv_event_property* prop = (mpv_event_property*)event->data;
                                                        if (strcmp(prop->name, "time-pos") == 0 && prop->format == MPV_FORMAT_DOUBLE) {
                                                            current_position = *(double*)prop->data;
                                                            if (on_time_update) {
                                                                on_time_update(current_position, duration);
                                                            }
                                                        }
                                                        else if (strcmp(prop->name, "duration") == 0 && prop->format == MPV_FORMAT_DOUBLE) {
                                                            duration = *(double*)prop->data;
                                                        }
                                                        break;
                                                    }
                    case MPV_EVENT_PLAYBACK_RESTART:
                                                    is_playing = true;
                                                    if (on_state_change) on_state_change();
                                                    break;
                    case MPV_EVENT_END_FILE:
                                                    is_playing = false;
                                                    is_loaded = false;
                                                    is_paused = false;
                                                    if (on_state_change) on_state_change();
                                                    break;

                    case MPV_EVENT_FILE_LOADED: {
                                                    // Ensure playback starts
                                                    is_loaded = true;
                                                    is_playing = true;
                                                    is_paused = false;
                                                    if (on_state_change) on_state_change();
                                                    break;
                                                }
                }
            }
        }
};
