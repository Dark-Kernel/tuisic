#pragma once
#include "player.cpp"
// #include "../../common/Track.h"
#include <functional>
#include <map>
#include <memory>
#include <sdbus-c++/sdbus-c++.h>
#include <string>
#include <vector>

class MPRISHandler {
private:
  std::shared_ptr<MusicPlayer> player;
  std::unique_ptr<sdbus::IConnection> connection;
  std::unique_ptr<sdbus::IObject> object;
  std::thread event_thread;
  std::atomic<bool> should_stop{false};

  // Track data callbacks
  std::function<std::string()> get_track_id;
  std::function<std::string()> get_track_name;
  std::function<std::string()> get_track_artist;
  std::function<void()> on_next_track;
  std::function<void()> on_previous_track;

  double current_position = 0.0;
  double total_duration = 0.0;

public:
  MPRISHandler(std::shared_ptr<MusicPlayer> player_instance)
      : player(player_instance) {}

  ~MPRISHandler() { cleanup(); }

  void initialize() {
    sdbus::ServiceName serviceName{"org.mpris.MediaPlayer2.tuisic"};
    connection = sdbus::createSessionBusConnection(serviceName);
//  connection->requestName(serviceName);
    sdbus::ObjectPath objectPath{"/org/mpris/MediaPlayer2"};
    object = sdbus::createObject(*connection, std::move(objectPath));

    setupInterfaces();
    setupCallbacks();
  }

  void setTrackCallbacks(std::function<std::string()> track_id_cb,
                         std::function<std::string()> track_name_cb,
                         std::function<std::string()> track_artist_cb,
                         std::function<void()> next_track_cb,
                         std::function<void()> prev_track_cb = nullptr) {
    get_track_id = track_id_cb;
    get_track_name = track_name_cb;
    get_track_artist = track_artist_cb;
    on_next_track = next_track_cb;
    on_previous_track = prev_track_cb;
  }

  void startEventLoop() {
    std::thread([this] { connection->enterEventLoop(); }).detach();
    updateMetadata();
    updatePlaybackStatus();
  }

  void cleanup() {
    should_stop = true;

    if (connection) {
      connection->leaveEventLoop();
    }

    if (event_thread.joinable()) {
      event_thread.join();
    }

    // Clear objects in correct order
    object.reset();
    connection.reset();
  }

      void notifyTrackChange() {
        if (object) {
            updateMetadata();
        }
    }
    
    void notifyPlaybackChange() {
        if (object) {
            updatePlaybackStatus();
        }
    }

  void setupCallbacks() {
    // Don't set time callback - use player's getter methods instead
    // This prevents overriding the main TUI's time callback
  }

private:
  void setupInterfaces() {
    sdbus::InterfaceName interfaceName{"org.mpris.MediaPlayer2"};
    sdbus::InterfaceName interfaceName2{"org.mpris.MediaPlayer2.Player"};

    // Player methods
    object
        ->addVTable(sdbus::MethodVTableItem{sdbus::MethodName{"Stop"},
                                            sdbus::Signature{""},
                                            {},
                                            sdbus::Signature{""},
                                            {},
                                            [this](sdbus::MethodCall call) {
                                              player->stop();
                                              updatePlaybackStatus();
                                              auto reply = call.createReply();
                                              reply.send();
                                            },
                                            {}},
                    sdbus::MethodVTableItem{sdbus::MethodName{"PlayPause"},
                                            sdbus::Signature{""},
                                            {},
                                            sdbus::Signature{""},
                                            {},
                                            [this](sdbus::MethodCall call) {
                                              player->togglePlayPause();
                                              updatePlaybackStatus();
                                              auto reply = call.createReply();
                                              reply.send();
                                            },
                                            {}},
                    sdbus::MethodVTableItem{sdbus::MethodName{"Next"},
                                            sdbus::Signature{""},
                                            {},
                                            sdbus::Signature{""},
                                            {},
                                            [this](sdbus::MethodCall call) {
                                              player->next_track();
                                              if (on_next_track)
                                                on_next_track();
                                              updateMetadata();
                                              updatePlaybackStatus();
                                              auto reply = call.createReply();
                                              reply.send();
                                            },
                                            {}},
                    sdbus::MethodVTableItem{sdbus::MethodName{"Pause"},
                                            sdbus::Signature{""},
                                            {},
                                            sdbus::Signature{""},
                                            {},
                                            [this](sdbus::MethodCall call) {
                                              player->pause();
                                              updatePlaybackStatus();
                                              auto reply = call.createReply();
                                              reply.send();
                                            },
                                            {}},
                    sdbus::MethodVTableItem{sdbus::MethodName{"Play"},
                                            sdbus::Signature{""},
                                            {},
                                            sdbus::Signature{""},
                                            {},
                                            [this](sdbus::MethodCall call) {
                                              player->resume();
                                              updatePlaybackStatus();
                                              auto reply = call.createReply();
                                              reply.send();
                                            },
                                            {}},
                    sdbus::MethodVTableItem{sdbus::MethodName{"Previous"},
                                            sdbus::Signature{""},
                                            {},
                                            sdbus::Signature{""},
                                            {},
                                            [this](sdbus::MethodCall call) {
                                              // player->previous_track();
                                              if (on_previous_track)
                                                on_previous_track();
                                              updateMetadata();
                                              updatePlaybackStatus();
                                              auto reply = call.createReply();
                                              reply.send();
                                            },
                                            {}},
                    sdbus::MethodVTableItem{sdbus::MethodName{"Seek"},
                                            sdbus::Signature{"x"},
                                            {"Offset"},
                                            sdbus::Signature{""},
                                            {},
                                            [this](sdbus::MethodCall call) {
                                              // int64_t offset;
                                              // call >> offset;
                                              // double seek_seconds = offset / 1000000.0;
                                              // player->seek(player->get_position() + seek_seconds);
                                              auto reply = call.createReply();
                                              reply.send();
                                            },
                                            {}},
                    sdbus::MethodVTableItem{sdbus::MethodName{"SetPosition"},
                                            sdbus::Signature{"ox"},
                                            {"TrackId", "Position"},
                                            sdbus::Signature{""},
                                            {},
                                            [this](sdbus::MethodCall call) {
                                              // sdbus::ObjectPath trackId;
                                              // int64_t position;
                                              // call >> trackId >> position;
                                              // double pos_seconds = position / 1000000.0;
                                              // player->seek(pos_seconds);
                                              auto reply = call.createReply();
                                              reply.send();
                                            },
                                            {}})
        .forInterface(interfaceName2);

    // MediaPlayer22 properties
    object
        ->addVTable(sdbus::registerProperty("Identity").withGetter([]() {
          return "tuisic";
        }),
                    sdbus::registerProperty("CanQuit").withGetter(
                        []() { return true; }),
                    sdbus::registerProperty("CanRaise").withGetter([]() {
                      return false;
                    }),
                    sdbus::registerProperty("DesktopEntry").withGetter([]() {
                      return "tuisic";
                    }))
        .forInterface(interfaceName);

    // Player properties
    object
        ->addVTable(
            sdbus::registerProperty("PlaybackStatus").withGetter([this]() {
              return getPlaybackStatus();
            }),
            sdbus::registerProperty("CanGoNext").withGetter([]() {
              return true;
            }),
            sdbus::registerProperty("CanGoPrevious").withGetter([]() {
              return true;
            }),
            sdbus::registerProperty("CanPause").withGetter([]() {
              return true;
            }),
            sdbus::registerProperty("CanPlay").withGetter(
                []() { return true; }),
            sdbus::registerProperty("CanSeek").withGetter(
                []() { return true; }),
            sdbus::registerProperty("Position").withGetter([this]() {
             // return int64_t(player->get_position() * 1000000);
              return int64_t(current_position * 1000000);

            }),
            sdbus::registerProperty("Volume").withGetter([]() { return 1.0; }),
            sdbus::registerProperty("Metadata").withGetter([this]() {
              return getMetadata();
            }),
            sdbus::SignalVTableItem{sdbus::SignalName{"PropertiesChanged"},
                                    sdbus::Signature{"sa{sv}as"},
                                    {}})
        .forInterface(interfaceName2);
  }

  std::string getPlaybackStatus() {
    if (player->is_playing_state()) {
      return "Playing";
    } else if (player->is_paused_state()) {
      return "Paused";
    } else {
      return "Stopped";
    }
  }

  std::map<std::string, sdbus::Variant> getMetadata() {
    std::map<std::string, sdbus::Variant> metadata;

    if (get_track_id) {
      metadata["mpris:trackid"] = sdbus::Variant(get_track_id());
    }
    if (get_track_name) {
      metadata["xesam:title"] = sdbus::Variant(get_track_name());
    }
    if (get_track_artist) {
      metadata["xesam:artist"] =
          sdbus::Variant(std::vector<std::string>{get_track_artist()});
    }

    metadata["position"] = sdbus::Variant(int64_t(player->get_position() * 1000000));
    metadata["mpris:length"] =
        sdbus::Variant(int64_t(player->get_duration() * 1000000));
        //sdbus::Variant(int64_t(total_duration * 1000000));


    return metadata;
  }

  void updatePlaybackStatus() {
    sdbus::InterfaceName interfaceName2{"org.mpris.MediaPlayer2.Player"};
    object->emitPropertiesChangedSignal(interfaceName2);
  }

  void updateMetadata() {
    sdbus::InterfaceName interfaceName2{"org.mpris.MediaPlayer2.Player"};
    object->emitPropertiesChangedSignal(interfaceName2);
  }
};

// TUI Mode Usage:
class TUIMPRISIntegration {
private:
  std::unique_ptr<MPRISHandler> mpris_handler;
  std::vector<Track> &track_data; // Your TUI track data
  std::vector<Track> &next_tracks; // The actual playing playlist
  int &current_track_index;

public:
  TUIMPRISIntegration(std::vector<Track> &tracks, std::vector<Track> &next_track_list, int &track_index)
      : track_data(tracks), next_tracks(next_track_list), current_track_index(track_index) {}

  void setup(std::shared_ptr<MusicPlayer> player) {
    mpris_handler = std::make_unique<MPRISHandler>(player);
    mpris_handler->initialize();

    mpris_handler->setTrackCallbacks(
        [this]() -> std::string {
          // Use next_tracks if available, otherwise fall back to track_data
          auto &source = !next_tracks.empty() ? next_tracks : track_data;
          return (source.empty() || current_track_index >= source.size())
                     ? ""
                     : source[current_track_index].id;
        },
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
        [this]() {
          // Next track via MPRIS
          auto &source = !next_tracks.empty() ? next_tracks : track_data;
          if (current_track_index < source.size() - 1) {
            current_track_index++;
          }
        },
        [this]() {
          // Previous track via MPRIS
          if (current_track_index > 0) {
            current_track_index--;
          }
        });

    mpris_handler->startEventLoop();
  }

  void notifyTrackChange() {
    if (mpris_handler) {
      mpris_handler->notifyTrackChange();
    }
  }

  void notifyPlaybackChange() {
    if (mpris_handler) {
      mpris_handler->notifyPlaybackChange();
    }
  }

  ~TUIMPRISIntegration() {
    // Destructor handles cleanup automatically
  }
};
