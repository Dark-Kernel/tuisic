#include "ftxui/dom/elements.hpp"
#include "justmusic.cpp"
#include "lastfm.cpp"
#include "localStorage.cpp"
#include "player.cpp"
#include "playlist_handler.cpp"
#include "saavn.cpp"
#include "soundcloud.cpp"
#include <cstdio>
#include <cstdlib>
#include <curl/curl.h>
#include <curl/urlapi.h>
#include <exception>
#include <fmt/core.h>
#include <fmt/format.h>
#include <ftxui/component/component.hpp> // for Renderer, Input, Menu, etc.
#include <ftxui/component/component_base.hpp>
#include <ftxui/component/component_options.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/component/screen_interactive.hpp> // for ScreenInteractive
#include <ftxui/screen/color.hpp>
#include <iostream>
#include <map>
#include <sched.h>

#ifdef WITH_MPRIS
#include <sdbus-c++/IConnection.h>
#include <sdbus-c++/Message.h>
#include <sdbus-c++/Types.h>
#include <sdbus-c++/VTableItems.h>
#include <sdbus-c++/sdbus-c++.h>
#endif

#include <string>
#include <unistd.h>
#include <vector>

// Data in string to render in UI
std::vector<std::string> track_strings;
std::vector<std::string> home_track_strings;
std::vector<std::string> recently_played_strings;
std::vector<std::string> trending_track_strings;

// Actual track data
std::vector<Track> track_data;
std::vector<Track> home_track_data;
std::vector<Track> track_data_saavn;
std::vector<Track> track_data_lastfm;
std::vector<Track> track_data_soundcloud;
std::vector<Track> track_data_forestfm;
std::vector<Track> next_tracks;
std::vector<Track> recently_played;
std::vector<Track> trending_tracks;

std::string current_track = "🎵TUISIC 🎵";
std::string current_artist = "Welcome back, User!";

static std::atomic<bool> daemon_mode_active{false};

// Playlist track index
int current_track_index = 0;

int selected_trending = 0;

// Sources
Lastfm lastfm;
SoundCloud soundcloud;
Saavn saavn;
Justmusic justmusic;

// Player instance
auto player = std::make_shared<MusicPlayer>();

// Playlist source
enum class PlaylistSource { None, Search, ForestFM, ClassicFM };
PlaylistSource current_source = PlaylistSource::None;

// Screen
auto screen = ftxui::ScreenInteractive::Fullscreen();

// Menu selection
int selected = 0;
int selectedd = 0;

// For song duration and position formatting
std::string format_time(double seconds) {
  int minutes = static_cast<int>(seconds) / 60;
  int secs = static_cast<int>(seconds) % 60;
  return fmt::format("{:02d}:{:02d}", minutes, secs);
}

// ASCII Art Collection for different music genres/moods
std::map<std::string, std::vector<std::string>> ascii_art = {
    {"Equalizer",
     {
         R"(         ██            )",
         R"(         ██            )",
         R"(      ▁▁ ██ ▂▂ ▆▆      )",
         R"(      ██ ██ ██ ██      )",
         R"(      ██ ██ ██ ██      )",
         R"(   ██ ██ ██ ██ ██ ██   )",
         R"(██ ██ ██ ██ ██ ██ ██   )",
         R"(██ ██ ██ ██ ██ ██ ██ ██)",
     }},
    {"default",
     {
         R"(   /\___/\    )",
         R"(  ( o   o )   )",
         R"(  (  =^=  )   )",
         R"(  (        )  )",
         R"(  (         ) )",
         R"(  (          ))",
         R"(   ( ______ )  )",
     }}};

// Select ASCII art based on genre
std::vector<std::string> get_track_ascii_art(const Track &track) {
  // Yet to be implement
  std::string genre = "Equalizer";
  return ascii_art[genre];
}

auto searchQuery(const std::string &query) {
  track_data_saavn = saavn.fetch_tracks(query);
  track_data = track_data_saavn;
  track_data_lastfm = lastfm.fetch_tracks(query);
  track_data_soundcloud = soundcloud.fetch_tracks(query);
  for (const auto &track : track_data_soundcloud) {
    track_data.push_back(track);
  }
  for (const auto &track : track_data_lastfm) {
    track_data.push_back(track);
  }

  home_track_data = track_data;
  track_strings.clear();

  // Convert tracks to display strings for menu UI
  for (const auto &track : track_data) {
    track_strings.push_back(track.to_string());
  }
  home_track_strings = track_strings;
  return track_strings;
}

auto fetch_recent() {
  // Clear previous data
  recently_played_strings.clear();
  track_data.clear();

  // Limit recently played to last 10 unique tracks
  std::vector<Track> unique_recently_played;
  for (const auto &track : recently_played) {
    // Check if this exact track is not already in unique list
    auto it = std::find_if(unique_recently_played.begin(),
                           unique_recently_played.end(),
                           [&track](const Track &existing) {
                             return existing.to_string() == track.to_string();
                           });

    if (it == unique_recently_played.end()) {
      unique_recently_played.push_back(track);
    }
  }

  // Keep only the last 10 tracks
  if (unique_recently_played.size() > 10) {
    unique_recently_played = std::vector<Track>(
        unique_recently_played.end() - 10, unique_recently_played.end());
  }

  // Populate track_data and recently_played_strings
  track_data = unique_recently_played;
  for (const auto &recent : track_data) {
    recently_played_strings.push_back(recent.to_string());
  }

  return recently_played_strings;
}

void switch_playlist_source(const std::vector<Track> &new_tracks) {
  // Stop current playback
  player->stop();

  // Update track data and strings
  track_data = new_tracks;
  track_strings.clear();
  for (const auto &track : track_data) {
    track_strings.push_back(track.to_string());
  }

  // Reset selection and current track
  selected = 0;
  if (!track_data.empty()) {
    current_track = track_data[0].name;
    current_artist = track_data[0].artist;
  }

  // Update UI
  screen.PostEvent(ftxui::Event::Custom);
}

//// Visualizer
// ftxui::Component create_visualizer() {
//   return ftxui::Renderer([&] {
//     std::vector<ftxui::Element> bars;

//     // Get latest visualization data
//     auto viz_data = player->get_visualization_data();

//     // Create bars for visualization
//     for (size_t i = 0; i < viz_data.size(); i++) {
//       int height =
//           static_cast<int>(viz_data[i] * 8); // Scale to reasonable height
//       bars.push_back(ftxui::vbox({ftxui::text(std::string(height, '|')) |
//                                   color(ftxui::Color::Green)}));
//     }

//     return hbox(std::move(bars));
//   });
// }

#ifdef WITH_MPRIS
#include "mpris_handler.cpp"
std::unique_ptr<MPRISHandler> mpris_handler;

// In TUI mode initialization:
void setupMPRISForDaemon(std::shared_ptr<MusicPlayer> player,
                         std::vector<Track> next_tracks,
                         int current_track_index) {
  mpris_handler = std::make_unique<MPRISHandler>(player);
  mpris_handler->initialize();
  system("notify-send 'MPRIS integration initialized'");

  mpris_handler->setTrackCallbacks(
      [&]() -> std::string {
        return next_tracks.empty() ? "" : next_tracks[current_track_index].id;
      },
      [&]() -> std::string {
        return next_tracks.empty() ? "" : next_tracks[current_track_index].name;
      },
      [&]() -> std::string {
        return next_tracks.empty() ? ""
                                  : next_tracks[current_track_index].artist;
      },
      [&]() {
        // Update TUI state when next track is triggered via MPRIS
        if (current_track_index < track_data.size() - 1) {
          // current_track_index++;
          // current_track_index = (current_track_index + 1) %
          // track_data.size();

          current_track_index = (current_track_index + 1) % next_tracks.size();
          // screen.PostEvent(ftxui::Event::Custom);
        }
        // correct logic to next track:
        //
        //                                             current_track_name =
        //                                             next_tracks[current_track_indexx-1].name;
        //                                             current_track_artist =
        //                                             next_tracks[current_track_indexx-1].artist;

        current_track = next_tracks[current_track_index-1].name;
        current_artist = next_tracks[current_track_index-1].artist;
        // screen.PostEvent(ftxui::Event::Custom);
        //       current_track_index = (current_track_index + 1) %
        //       track_data.size();
        // player->play(track_data[current_track_index].url);

        // Trigger UI refresh if needed
      },
      [&]() {
        // Update TUI state when previous track is triggered via MPRIS
        if (current_track_index > 0) {
          // current_track_index--;
          // current_track_index = (current_track_index - 1) %
          // track_data.size();
          current_track_index = (current_track_index - 1) % next_tracks.size();
          // screen.PostEvent(ftxui::Event::Custom);
        }
      }
      );

  mpris_handler->startEventLoop();
}

std::unique_ptr<TUIMPRISIntegration> tui_mpris;
#endif



#ifdef WITH_MPRIS
sdbus::IObject *g_concatenator{};
#endif

int main(int argc, char *argv[]) {

#ifdef WITH_MPRIS
      tui_mpris = std::make_unique<TUIMPRISIntegration>(track_data, current_track_index);
      // tui_mpris->setup(player);
#endif

  if (argc >= 3 && std::string(argv[1]) == "--daemon") {
    auto player = std::make_shared<MusicPlayer>();
    std::string current_track_id = argv[2];
    std::string current_track_name = argv[3];
    std::string current_track_artist = argv[4];
    std::string current_track_url = argv[5];
    auto next_tracks = saavn.fetch_next_tracks(current_track_id.c_str());
    std::vector<std::string> next_track_urls;
    next_track_urls.push_back(current_track_url);
    for (const auto &track : next_tracks) {
      next_track_urls.push_back(track.url);
    }
    player->create_playlist(next_track_urls);
    player->play(current_track_url);
    int current_track_indexx = player->get_current_playlist_index();
    system(("notify-send 'Tuisic' 'Playing'" +
            std::to_string(current_track_indexx))
               .c_str());

    double current_position = 0.0;
    double total_duration = 0.0;
    int progress_percentage = 0;

    double seek_position = (progress_percentage / 100.0) * total_duration;
    seek_position = std::min(seek_position, total_duration);

    player->set_time_callback([&](double pos, double dur) {
      current_position = pos;
      total_duration = dur;
      // system("notify-send 'Tuisic' 'Changing position'");
      // Prevent division by zero and ensure valid percentage
      if (total_duration > 0) {
        progress_percentage = static_cast<int>((pos / dur) * 100.0);
        screen.PostEvent(ftxui::Event::Custom);
      }
    });

#ifdef WITH_MPRIS
    sdbus::ServiceName serviceName{"org.mpris.MediaPlayer2.tuisic"};
    auto connection = sdbus::createSessionBusConnection(serviceName);
    // connection->requestName(serviceName);
    sdbus::ObjectPath objectPath{"/org/mpris/MediaPlayer2"};

    auto object = sdbus::createObject(*connection, std::move(objectPath));
    g_concatenator = object.get();

    sdbus::InterfaceName interfaceName{"org.mpris.MediaPlayer2"};
    sdbus::InterfaceName interfaceName2{"org.mpris.MediaPlayer2.Player"};
    auto getPlaybackStatus = [&]() -> std::string {
      if (player->is_playing_state()) {
        return "Playing";
      } else if (player->is_paused_state()) {
        return "Paused";
      } else {
        return "Stopped";
      }
    };

    auto getMetadata = [&]() -> std::map<std::string, sdbus::Variant> {
      std::map<std::string, sdbus::Variant> metadata;
      metadata["mpris:trackid"] = sdbus::Variant(current_track_id);
      metadata["xesam:title"] = sdbus::Variant(current_track_name);
      metadata["position"] = sdbus::Variant(int64_t(current_position * 1000000));
      metadata["xesam:artist"] =
          sdbus::Variant(std::vector<std::string>{current_track_artist});
      metadata["mpris:length"] =
          sdbus::Variant(int64_t(total_duration * 1000000)); // 3 minutes in microseconds
      return metadata;
    };

    auto updatePlaybackStatus = [&]() {
      std::map<std::string, sdbus::Variant> properties;
      std::vector<sdbus::PropertyName> propNames;

      properties["PlaybackStatus"] = sdbus::Variant(getPlaybackStatus());
      properties["Position"] = sdbus::Variant(int64_t(current_position * 1000000));
      g_concatenator->emitPropertiesChangedSignal(interfaceName2);
      // g_concatenator->emitSignal(sdbus::SignalName{"PropertiesChanged"}).onInterface(interfaceName2).withArguments(sdbus::Signature{"sa{sv}as"}, properties, std::vector<std::string>{});
      // emitPropertiesChanged("org.mpris.MediaPlayer2.Player", properties);
    };

    auto updateMetadata = [&]() {
        system(("notify-send 'Tuisic' 'Updating metadata'"));
      std::map<std::string, sdbus::Variant> properties;
      properties["Metadata"] = sdbus::Variant(getMetadata());
      g_concatenator->emitPropertiesChangedSignal(interfaceName2);
      // emitPropertiesChanged("org.mpris.MediaPlayer2.Player", properties);
    };

    g_concatenator
        ->addVTable(sdbus::MethodVTableItem{sdbus::MethodName{"Stop"},
                                            sdbus::Signature{""},
                                            {},
                                            sdbus::Signature{""},
                                            {},
                                            [&](sdbus::MethodCall call) {
                                              player->stop();
                                              updatePlaybackStatus();
                                              auto reply = call.createReply();
                                              reply.send();
                                              exit(0);
                                            },
                                            {}},
                    sdbus::MethodVTableItem{sdbus::MethodName{"PlayPause"},
                                            sdbus::Signature{""},
                                            {},
                                            sdbus::Signature{""},
                                            {},
                                            [&](sdbus::MethodCall call) {
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
                                            [&](sdbus::MethodCall call) {
                                              player->next_track();
                                                current_track_indexx = (current_track_indexx + 1) % next_tracks.size();
                                                current_track_name = next_tracks[current_track_indexx-1].name;
                                                current_track_artist = next_tracks[current_track_indexx-1].artist;
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
                                            [&](sdbus::MethodCall call) {
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
                                            [&](sdbus::MethodCall call) {
                                              player->resume();
                                              updatePlaybackStatus();
                                              auto reply = call.createReply();
                                              reply.send();
                                            },
                                            {}})
        .forInterface(interfaceName2);

    // Properties for MediaPlayer2 interface
    g_concatenator
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

    // Properties for Player interface
    g_concatenator
        ->addVTable(
            sdbus::registerProperty("PlaybackStatus")
                .withGetter(
                    [getPlaybackStatus]() { return getPlaybackStatus(); }),
            sdbus::registerProperty("CanGoNext").withGetter([]() {
              return true;
            }),
            sdbus::registerProperty("CanGoPrevious").withGetter([]() {
              return true; // TODO
            }),
            sdbus::registerProperty("CanPause").withGetter([]() {
              return true;
            }),
            sdbus::registerProperty("CanPlay").withGetter(
                []() { return true; }),
            sdbus::registerProperty("CanSeek").withGetter(
                []() { return true; }),
            sdbus::registerProperty("Position").withGetter([&]() {
              return int64_t(current_position * 1000000);
            }),
            sdbus::registerProperty("Volume").withGetter([]() { return 1.0; }),
            sdbus::registerProperty("Metadata").withGetter([&]() {
              return getMetadata();
            }),
            sdbus::SignalVTableItem{sdbus::SignalName{"PropertiesChanged"},
                                    sdbus::Signature{"sa{sv}as"},
                                    {}})
        .forInterface(interfaceName2);

    player->set_end_of_track_callback([&] {
      current_track_indexx = (current_track_indexx + 1) % next_tracks.size();
      current_track_name = next_tracks[current_track_indexx-1].name;
      current_track_artist = next_tracks[current_track_indexx-1].artist;
    });

    // Run the I/O event loop on the bus connection.
    std::thread([&connection] { connection->enterEventLoop(); }).detach();
    updateMetadata();
    updatePlaybackStatus();

    #endif
    // keep alive
    std::this_thread::sleep_for(std::chrono::hours(24 * 365));
    return 0;
  }
  curl_global_init(CURL_GLOBAL_ALL);
  auto config = std::make_shared<Config>();

  using namespace ftxui;

  // Placeholder for search input
  std::string search_query;
  std::string current_album = "";
  std::string button_text = "Play";
  std::mutex playlist_mutex;
  std::vector<std::string> next_playlist;

  // Placeholder track list
  std::vector<std::string> tracks = {};

  // Selected track index
  auto reset_player_state = [&current_album, &button_text] {
    current_track = "🎵TUISIC 🎵";
    current_artist = "Welcome back, User!";
    current_album = "";
    button_text = "Play";
    selected = 0;
    current_track_index = 0;
  };

  // for progress
  int progress = 0;

  // Volume control
  int volume = 100;

  // std::vector<Element> trending_elements;
  // std::thread trending_thread([&]() {
  //   trending_tracks = saavn.fetch_trending();
  //   for (const auto &track : trending_tracks) {
  //     trending_track_strings.push_back(track.name);
  //   }

  //   for (const auto &track : trending_tracks) {
  //     trending_elements.push_back(
  //         vbox({
  //             hbox({vbox({
  //                       text(track.name) | bold | color(Color::White),
  //                       text(track.artist) | dim,
  //                   }) | flex,
  //                   text(" ▶ ") | color(Color::White) | border}),
  //         }) |
  //         bgcolor(Color::HSV(0.5, 0.2, 0.2)) | border | size(HEIGHT, EQUAL,
  //         5));
  //   }
  //   screen.PostEvent(Event::Custom);
  // });
  // trending_thread.detach();

  std::thread trending_thread([&]() {
    trending_tracks = saavn.fetch_trending();
    for (const auto &track : trending_tracks) {
      trending_track_strings.push_back(track.to_string());
    }
    screen.PostEvent(Event::Custom);
  });
  trending_thread.detach();

  // Components
  Component input_search = Input(&search_query, "Search for music...");
  input_search = Input(&search_query, "Search for music...") |
                 CatchEvent([&tracks, &search_query](Event event) {
                   if (event == Event::Return) {
                     tracks = searchQuery(search_query);
                     return true;
                   }
                   return false;
                 });

  std::vector<std::string> test_track = {"Track 1", "Track 2", "Track 3"};
  auto menu2 = Menu(&test_track, &selectedd, MenuOption::Horizontal());

  auto menu = Menu(&tracks, &selected);
  menu =
      Menu(&tracks, &selected) |
      CatchEvent([&button_text, &next_playlist, &playlist_mutex, &config,
                  argv](Event event) {
        if (event == Event::Return) {
          // std::cerr << "Selected: " << selected << std::endl;
          if (selected >= 0 && selected < track_data.size()) {
            current_source = PlaylistSource::Search;
            if (!track_data_forestfm.empty()) {
              player->stop();
            }
            /* player->stop(); */
            /* player->play(track_data[selected].url); */
            recently_played.push_back(track_data[selected]);
            current_track = track_data[selected].name;
            current_artist = track_data[selected].artist;
            button_text = "Pause";
            screen.PostEvent(Event::Custom);

            if (track_data[selected].id != "") {

              std::thread next_tracks_thread([&]() {
                try {
                  /* std::cerr << "Fetttchiing nextttttttttttttt"; */
                  system(("notify-send 'Tuisic' " + track_data[selected].id +
                          track_data[selected].url)
                             .c_str());
                if(track_data[selected].source=="soundcloud"){
                next_tracks = soundcloud.fetch_next_tracks(track_data[selected].url);
                }else{
                next_tracks = saavn.fetch_next_tracks(track_data[selected].id);
                }

                  // next_tracks =
                  //     saavn.fetch_next_tracks(track_data[selected].id);
                  std::vector<std::string> next_track_urls;
                  next_track_urls.push_back(track_data[selected].url);
                  for (const auto &track : next_tracks) {
                    next_track_urls.push_back(track.url);
                  }
                  next_tracks.insert(next_tracks.begin(), track_data[selected]);

                  {
                    std::lock_guard<std::mutex> lock(playlist_mutex);
                    next_playlist = next_track_urls;
                  }
                  player->create_playlist(next_track_urls);
                  current_track_index = 0;
                  player->play(next_tracks[0].url);

                  
                #ifdef WITH_MPRIS
                  tui_mpris->setup(player);
                #endif

                  screen.PostEvent(Event::Custom);
                } catch (const std::exception &e) {
                  std::cerr << e.what() << std::endl;
                }
              });
              next_tracks_thread.detach();
            }

            /* player->create_playlist(next_tracks_strings); */
            /* current_track_index = 0; */
            /* player->play(next_tracks[0].url); */
          }
          return true;
        }
        // press l on selected to copy url
        if (event == Event::AltL) {
          if (selected >= 0 && selected < track_data.size()) {
            /* std::string url = track_data[selected].url; */
            std::string url = player->get_current_track();
            std::string is_wayland = getenv("XDG_SESSION_TYPE");
            if (is_wayland == "wayland") {
              system(("echo \"" + url + "\" | wl-copy").c_str());
              system(("notify-send \"Copied URL: " + url + "\"").c_str());
              std::cout << "Copied URL: " << url << std::endl;
              return true;
            }
            system(
                ("echo \"" + url + "\" | xclip -selection clipboard").c_str());
            system(("notify-send \"Copied URL: " + url + "\"").c_str());
            std::cout << "Copied URL: " << url << std::endl;
            return true;
          }
        }
        if (event == Event::Character('L')) {
          std::cerr << "pressed L" << std::endl;
          player->toggle_subtitles();
        }

        if (event == Event::Character('a')) {
          if (selected >= 0 && selected < track_data.size()) {
            if (!isFavorite(track_data[selected].name)) {
              favorite_tracks.push_back(track_data[selected]);
            }
          }
        }

        if (event == Event::Character('d')) {
          if (selected >= 0 && selected < track_data.size()) {
            std::string current_song = track_data[selected].name + ".mp3";
            /* std::string current_song = player->get_current_track(); */
            std::replace(current_song.begin(), current_song.end(), '/', '_');
            std::replace(current_song.begin(), current_song.end(), '\\', '_');
            /* std::string final_path = "/home/dk/Music/" + current_song +
             * ".mp3"; */
            std::string path = config->get_download_path();

            // Start download
            if (player->download_track(track_data[selected].url, path,
                                       current_song)) {
              system(("notify-send \"Started downloading: " +
                      track_data[selected].name + "\"")
                         .c_str());
            } else {
              system(("notify-send \"Failed to start download: " +
                      track_data[selected].name + "\"")
                         .c_str());
            }
          }
          return true;
        }
        if (event == Event::Character('p')) {
          std::string some = config->get_download_path();
          system(("notify-send \"Download path: " + some + "\"").c_str());
        }

        if (event == Event::Character('r')) {
          player->toggle_repeat();
        }

        if (event == Event::Character("w")) {
        #ifdef WITH_MPRIS
          daemon_mode_active = true;
          // system(("notify-send \"Starting daemon..." + track[0].id.c_str() +
          // "\"").c_str());

          std::string url = player->get_current_track();
          pid_t pid = fork();
          if (pid < 0) {
            exit(1);
          }
          if (pid == 0) {
            // child: replace image with daemon invocation
            execl(argv[0], argv[0], "--daemon", track_data[0].id.c_str(),
                  track_data[0].name.c_str(), track_data[0].artist.c_str(),
                  url.c_str(), nullptr);
            _exit(1);
          }
          screen.Exit();
        #endif
        }

        return false;
      });

  auto trending_menu =
      Menu(&trending_track_strings, &selected_trending) |
      CatchEvent([&](Event event) {
        if (event == Event::Return) {
          if (selected_trending >= 0 &&
              selected_trending < trending_tracks.size()) {
            current_source = PlaylistSource::Search;
            if (!track_data_forestfm.empty()) {
              player->stop();
            }
            player->play(trending_tracks[selected_trending].url);
            current_track = trending_tracks[selected_trending].name;
            current_artist = trending_tracks[selected_trending].artist;
            button_text = "Pause";
            recently_played.push_back(trending_tracks[selected_trending]);
            screen.PostEvent(Event::Custom);
          }
          return true;
        }
        return false;
      });

  Component volume_slider =
      Slider("", &volume, 0, 100, 1) | CatchEvent([&](Event event) {
        if (event == Event::Custom) {
          player->set_volume(volume);
          return true;
        }
        return false;
      });

  auto play_button = Button(
      &button_text,
      [&] {
        if (current_source == PlaylistSource::ForestFM) {
          if (player->is_playing_state()) {
            player->pause();
            button_text = "Play";
          } else {
            player->play(track_data_forestfm[current_track_index].url);
            button_text = "Pause";
          }
        } else if (current_source == PlaylistSource::Search) {
          if (selected >= 0 && selected < track_data.size()) {
            current_source = PlaylistSource::Search;
            // Stop any ongoing ForestFM playback
            if (!track_data_forestfm.empty()) {
              player->stop();
            }
            player->play(track_data[selected].url);
            current_artist = track_data[selected].artist;
            current_track = track_data[selected].name;
            button_text = "Pause";
          }
        } else {
          if (selected >= 0 && selected < track_data.size()) {
            current_source = PlaylistSource::Search;
            player->play(track_data[selected].url);
            current_track = track_data[selected].name;
            current_artist = track_data[selected].artist;
            button_text = "Pause";
          }
        }
        screen.PostEvent(Event::Custom);
      },
      ButtonOption::Animated(Color::Default, Color::GrayDark, Color::Default,
                             Color::White));

  std::string button_text_forestfm = "▶";
  auto play_forestfm = Button(
      &button_text_forestfm,
      [&] {
        reset_player_state();

        if (player->is_playing_state()) {
          if (current_source == PlaylistSource::ForestFM) {

            player->pause();
            std::cerr << "TRIGGER paused" << std::endl;
            button_text_forestfm = "▶";
            return;
          }
        }

        // Use a flag to prevent multiple simultaneous requests
        static std::atomic<bool> is_fetching{false};

        if (is_fetching) {
          return; // Prevent multiple simultaneous fetch attempts
        }

        // Reset track information
        current_track = "Fetching tracks...";
        screen.PostEvent(Event::Custom);

        // Consider using a separate thread for fetching
        std::thread fetch_thread([&]() {
          is_fetching = true;
          try {
            track_data_forestfm = justmusic.getMP3URL();

            // Assuming you have a way to queue events or update on the main
            // thread
            current_track = track_data_forestfm.empty() ? "No tracks found"
                                                        : "Tracks fetched";

            is_fetching = false;
            if (!track_data_forestfm.empty()) {
              // Create playlist of URLs
              std::vector<std::string> track_urls;
              for (const auto &track : track_data_forestfm) {
                track_urls.push_back(track.url);
              }
              current_album = track_data_forestfm[0].name;

              // Stop current playback if needed
              if (player->is_playing_state()) {
                player->stop(); // Complete stop instead of just pausing
              }

              // Create playlist and start playing
              player->create_playlist(track_urls);
              current_track_index = 0;
              button_text_forestfm = "❚❚";
              current_source = PlaylistSource::ForestFM;

              // Update current track info
              current_track = track_data_forestfm[0].name;
              current_artist = track_data_forestfm[0].artist;
              player->play(track_data_forestfm[0].url);
              button_text = "Pause";
            }
            screen.PostEvent(Event::Custom);
          } catch (const std::exception &e) {
            current_track = "Error fetching tracks: " + std::string(e.what());
            screen.PostEvent(Event::Custom);
          }
        });
        fetch_thread.detach(); // Allow thread to run independently
      },
      ButtonOption::Animated(Color::Default, Color::GrayDark, Color::Default,
                             Color::White));

  std::string button_text_classic = "▶";
  auto play_classic = Button(
      &button_text_classic, [&] { reset_player_state(); },
      ButtonOption::Animated(Color::Default, Color::GrayDark, Color::Default,
                             Color::White));

  std::vector<std::string> playlist_items = {"Home", "Recently Played",
                                             "Favorites", "CustomPlaylist"};
  int selected_playlist = 0;
  auto playlist_menu =
      Menu(&playlist_items, &selected_playlist) | CatchEvent([&](Event event) {
        if (event == Event::Return) {
          if (selected_playlist == 0) {
            // current_source = PlaylistSource::Favorites;
            current_track = "Home";
            tracks.clear();
            track_data.clear();
            track_data = home_track_data;
            tracks = home_track_strings;
          } else if (selected_playlist == 1) {
            // current_source = PlaylistSource::Recent;
            current_track = "Recently Played";
            home_track_strings = tracks;
            tracks.clear();
            tracks = fetch_recent();
          } else if (selected_playlist == 2) {
            // current_source = PlaylistSource::Custom;
            if (!(home_track_strings.size() > 0)) {
              home_track_strings = tracks;
            }
            current_track = "Favorites";
            tracks.clear();
            tracks = fetch_favorites(track_data);
          } else if (selected_playlist == 3) {
            // current_source = PlaylistSource::Custom;
            current_track = "Custom Playlist";
          }
          screen.PostEvent(Event::Custom);
          return true;
        }
        return false;
      });

  // player callbacks
  player->set_state_callback([&] {
    button_text = player->is_playing_state() ? "Pause" : "Play";
    screen.PostEvent(Event::Custom);
  });
  double current_position = 0.0;
  double total_duration = 0.0;
  int progress_percentage = 0;

  // Update the time callback to correctly calculate progress
  player->set_time_callback([&](double pos, double dur) {
    current_position = pos;
    total_duration = dur;

    // Prevent division by zero and ensure valid percentage
    if (total_duration > 0) {
      progress_percentage = static_cast<int>((pos / dur) * 100.0);
      screen.PostEvent(Event::Custom);
    }
  });

  std::string current_subtitle_text = "No subtitle";
  player->set_subtitle_callback([&](const std::string &subtitle) {
    // Update your UI with the subtitle
    current_subtitle_text = subtitle;
    screen.PostEvent(Event::Custom); // Trigger screen update
  });

  Component progress_slider = Slider("", &progress_percentage, 0, 100, 1);
  progress_slider |= CatchEvent([&](Event event) {
    if (event == Event::Custom) {
      // Seek to the new position when slider is moved
      if (total_duration > 0) {
        double seek_position = (progress_percentage / 100.0) * total_duration;
        seek_position = std::min(seek_position, total_duration);
      }
      return true;
    }
    return false;
  });

  auto button_prev = Button("<-", [&] {
    if (current_source == PlaylistSource::ForestFM &&
        !track_data_forestfm.empty()) {
      current_track_index =
          (current_track_index - 1 + track_data_forestfm.size()) %
          track_data_forestfm.size();
      player->play(track_data_forestfm[current_track_index].url);

      current_track = track_data_forestfm[current_track_index].name;
      current_artist = track_data_forestfm[current_track_index].artist;
      button_text = "Pause";
    } else if (current_source == PlaylistSource::Search &&
               !track_data.empty()) {
      player->previous_track(track_data, selected);

      current_track = track_data[selected].name;
      current_artist = track_data[selected].artist;
      button_text = "Pause";
    }

    screen.PostEvent(Event::Custom);
  });

  auto button_next = Button("->", [&] {
    if (current_source == PlaylistSource::ForestFM &&
        !track_data_forestfm.empty()) {
      current_track_index =
          (current_track_index + 1) % track_data_forestfm.size();
      player->play(track_data_forestfm[current_track_index].url);

      current_track = track_data_forestfm[current_track_index].name;
      current_artist = track_data_forestfm[current_track_index].artist;
      button_text = "Pause";
    } else if (current_source == PlaylistSource::Search &&
               !track_data.empty()) {
      player->next_track(track_data, selected);

      current_track = track_data[selected].name;
      current_artist = track_data[selected].artist;
      button_text = "Pause";
    }

    screen.PostEvent(Event::Custom);
  });

  player->set_end_of_track_callback([&] {
    // Automatically update track info when a track ends
    if (!next_tracks.empty()) {
      /* current_track_index = player->get_current_playlist_index(); */
      // std::cerr << next_tracks.size() << std::endl;
      current_track_index = (current_track_index + 1) % next_tracks.size();
      current_track = next_tracks[current_track_index].name;
      current_artist = next_tracks[current_track_index].artist;
      player->next_track(next_tracks, current_track_index);
    } else if (!track_data_forestfm.empty()) {
      current_track_index =
          (current_track_index + 1) % track_data_forestfm.size();
      current_track = track_data_forestfm[current_track_index].name;
      current_artist = track_data_forestfm[current_track_index].artist;
    } else if (!track_data.empty()) {
      selected = (selected + 1) % track_data.size();
      current_track = track_data[selected].name;
      current_artist = track_data[selected].artist;
    }

    // Ensure UI updates
    screen.PostEvent(Event::Custom);

    // Continue playback
    /* player->next_track(track_data, selected); */
  });

  // Component tree
  auto component = Container::Vertical({
      // Top section
      Container::Horizontal({
          Container::Vertical({
              playlist_menu,
              play_forestfm,
              play_classic,
              // create_visualizer(),
              volume_slider,
          }),

          Container::Vertical({
              input_search,
              Container::Horizontal({
                  menu,
                  trending_menu,
              }) | flex,
              // menu,
              // trending_menu,
          }),

          Container::Vertical({
              // progress_slider,
              button_prev,
              play_button,
              button_next,
          }),

      }),
  });

  component =
      component | CatchEvent([&](Event event) {
        // Check if search input is focused
        bool is_search_focused = input_search->Focused();

        // Handle keyboard events with consideration of input focus
        if (is_search_focused) {
          // When search is focused, allow normal input behavior
          if (event == Event::Character('q')) {
            // Do nothing when 'q' is pressed in search input
            return false;
          }
          if (event == Event::Character(' ')) {
            // Allow space to be entered in search input
            return false;
          }

          if (event == Event::Escape) {
            menu->TakeFocus();
            return true;
          }
        } else {
          // Handle global keyboard shortcuts when search is not focused
          if (event == Event::Character(' ')) {
            // Space for play/pause
            if (current_source == PlaylistSource::Search &&
                !track_data.empty()) {
              if (player->is_playing_state()) {
                player->pause();
                button_text = "Play";
              } else {
                /* player->play(track_data[selected].url); */
                /* current_track = track_data[selected].name; */
                /* current_artist = track_data[selected].artist; */
                player->resume();
                button_text = "Pause";
              }
            } else if (current_source == PlaylistSource::ForestFM &&
                       !track_data_forestfm.empty()) {
              if (player->is_playing_state()) {
                player->pause();
                button_text = "Play";
                button_text_forestfm = "▶";
              } else {
                player->play(track_data_forestfm[current_track_index].url);
                current_track = track_data_forestfm[current_track_index].name;
                current_artist =
                    track_data_forestfm[current_track_index].artist;
                button_text = "Pause";
                button_text_forestfm = "❚❚";
              }
            }
            screen.PostEvent(Event::Custom);
            return true;
          }

          if (event == Event::Character('.')) {
            player->skip_forward();
            return true;
          }

          if (event == Event::Character(',')) {
            player->skip_backward();
            return true;
          }

          if (event == Event::Character('>')) { // Next track
            if (!next_tracks.empty()) {
              /* current_track_index = player->get_current_playlist_index(); */
              // std::cerr << next_tracks.size() << std::endl;
              current_track_index =
                  (current_track_index + 1) % next_tracks.size();
              current_track = next_tracks[current_track_index].name;
              current_artist = next_tracks[current_track_index].artist;
              player->next_track(next_tracks, current_track_index);
            } else if (!track_data_forestfm.empty()) {
              current_track_index =
                  (current_track_index + 1) % track_data_forestfm.size();
              current_track = track_data_forestfm[current_track_index].name;
              current_artist = track_data_forestfm[current_track_index].artist;
            } else if (!track_data.empty()) {
              selected = (selected + 1) % track_data.size();
              current_track = track_data[selected].name;
              current_artist = track_data[selected].artist;
            }

            // Ensure UI updates
            screen.PostEvent(Event::Custom);
          }
          if (event == Event::Character('<')) { // Previous track
            if (!next_tracks.empty()) {
              /* current_track_index = player->get_current_playlist_index(); */
              // std::cerr << next_tracks.size() << std::endl;
              current_track_index =
                  (current_track_index - 1) % next_tracks.size();
              current_track = next_tracks[current_track_index].name;
              current_artist = next_tracks[current_track_index].artist;
              player->next_track(next_tracks, current_track_index);
            } else if (!track_data_forestfm.empty()) {
              current_track_index =
                  (current_track_index - 1) % track_data_forestfm.size();
              current_track = track_data_forestfm[current_track_index].name;
              current_artist = track_data_forestfm[current_track_index].artist;
            } else if (!track_data.empty()) {
              selected = (selected - 1) % track_data.size();
              current_track = track_data[selected].name;
              current_artist = track_data[selected].artist;
            }
            screen.PostEvent(Event::Custom);
          }

          if (event == Event::Character('+') ||
              event == Event::Character('=')) {
            volume = std::min(100, volume + 5);
            player->set_volume(volume);
            screen.PostEvent(Event::Custom);
            return true;
          }
          if (event == Event::Character('-')) {
            volume = std::max(0, volume - 5);
            player->set_volume(volume);
            screen.PostEvent(Event::Custom);
            return true;
          }
          if (event == Event::Character('m')) { // Mute toggle
            static int previous_volume = 100;
            if (volume > 0) {
              previous_volume = volume;
              volume = 0;
            } else {
              volume = previous_volume;
            }
            player->set_volume(volume);
            screen.PostEvent(Event::Custom);
            return true;
          }

          // endof else
        }

        // Global quit shortcut
        if (event == Event::Character('q')) {
          saveData(recently_played, favorite_tracks);
          screen.Exit();
          return true;
        }

        // Escape to unfocus search box
        if (event == Event::Escape) {
          screen.PostEvent(Event::Custom);
          return true;
        }

        return false;
      });

  // Data Persistence
  loadData(recently_played, favorite_tracks);
  std::vector<Element> smoe;

  // Layout
  auto renderer = Renderer(component, [&] {
    return vbox({
        hbox({text(" λ ") | bgcolor(Color::Blue) | color(Color::White),
              text(" 🎵" + current_track + " 🎵") | bold | center,
              text(" " + current_artist + " ") | dim | center, filler(),
              text(fmt::format(" {} | {} ", format_time(current_position),
                               format_time(total_duration))) |
                  border}) |
            bold,
        separator(),
        filler(),
        hbox({
            vbox({
                // text("Library") | bold,
                vbox({
                    text("Playlists: ") | bold, separator(),
                    playlist_menu->Render(),
                    // }) | border,
                }),
                separator(),
                hbox({
                    play_forestfm->Render() | size(WIDTH, EQUAL, 5) | bold |
                        center,
                    text("Forest FM") | bold | center,
                }) | center,
                separator(),
                hbox({
                    play_classic->Render() | size(WIDTH, EQUAL, 5) | bold |
                        center,
                    text("ClassicFM") | bold | center,
                }) | center,
                separator(),
                hbox({
                    [&]() -> Element {
                      // Fetch the ASCII art for testing
                      auto art = get_track_ascii_art({});
                      // Convert each line into an FTXUI Element
                      std::vector<Element> art_elements;
                      for (const auto &line : art) {
                        art_elements.push_back(text(line) | color(Color::Blue));
                      }
                      return vbox(std::move(art_elements)) | center;
                    }(),
                    // create_visualizer()->Render(),
                    // [&]() -> Element {
                    //   auto viz_data = player->get_visualization_data();
                    //   std::vector<Element> bars;

                    //   // Create bars for visualization
                    //   for (size_t i = 0; i < viz_data.size(); i++) {
                    //     int height = static_cast<int>(
                    //         viz_data[i] * 8); // Scale to reasonable height
                    //     bars.push_back(vbox({text(std::string(height, '|')) |
                    //                          color(ftxui::Color::Green)}));
                    //   }

                    //   return hbox(std::move(bars)) | center;
                    // }(),
                }) | center,
                separator(),
                hbox({
                    text("♪ ") | color(Color::Blue),
                    volume_slider->Render(),
                }),
                //////// Volume slider with %
                ///////////////////////////////////////
                // hbox({
                //     text("🔊 ") | color(Color::Blue),
                //     volume_slider->Render() | flex,
                //     text(std::to_string(volume) + "%") | size(WIDTH, EQUAL,
                //     4),
                // }) | size(HEIGHT, EQUAL, 1),
                /////////////////////////////////////////////////////////////////
            }) | size(WIDTH, EQUAL, 30) |
                border,
            vbox({
                hbox({
                    text("Search: ") | bold,
                    input_search->Render(),
                }) | border,
                separator(),
                text("Available Tracks:") | bold,
                /* menu->Render() | frame | flex, */
                hbox(
                    {vbox({
                         menu->Render() | frame | size(HEIGHT, EQUAL, 22) |
                             size(WIDTH, EQUAL, 90),
                     }),
                     separator(),
                     /* vbox({ */
                     /*      menu2->Render(), */
                     /* }), */
                     vbox({hbox({
                               filler(),
                               text(" Trendings ") | bold | color(Color::White),
                               filler(),
                           }),
                           separator(), trending_menu->Render() | frame}) |
                         vscroll_indicator | yframe | flex |
                         size(HEIGHT, EQUAL, 22)}),

                // text("Subtitles: " + current_subtitle_text) |
                // color(Color::Blue),
                ////////////////////// Working subtitles
                ////////////////////////////
                /* hbox({ */
                /*     text("Subtitle: ") | bold | color(Color::LightSkyBlue1),
                 */
                /*     text(current_subtitle_text), */
                /* }), */

                separator(),
            }) | flex,
        }),
        filler(),
        vbox({
            hbox({
                progress_slider->Render() | flex,
            }),
            hbox({
                filler(),
                button_prev->Render(),
                play_button->Render(),
                button_next->Render(),
                filler(),
            }) | center,
        }),
        separator(),
        hbox({text(" λ TUISIC ") | bgcolor(Color::Blue) |
                  color(Color::White),
              filler(),
              hbox({
                  text("⌨️ ") | dim,
                  text("w:Daemon ") | dim,
                  text("Space:Play ") | dim,
                  text("↑/↓:Navigate ") | dim,
                  text("./,:Skip ") | dim,
                  text(">/<:Next/Prev ") | dim,
                  text("m:Mute ") | dim,
              }) | center,
              text(fmt::format(" {} Tracks ",
                               current_source == PlaylistSource::Search
                                   ? track_data.size()
                                   : track_data_forestfm.size())) |
                  dim}),
    });
  });

  screen.Loop(renderer);
  return 0;
}
