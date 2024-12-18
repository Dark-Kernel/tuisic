#include "ftxui/dom/elements.hpp"
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
#include <string>
#include <vector>

#include "justmusic.cpp"
#include "lastfm.cpp"
#include "localStorage.cpp"
#include "player.cpp"
#include "playlist_handler.cpp"
#include "saavn.cpp"
#include "soundcloud.cpp"

// Data in string to render in UI
std::vector<std::string> track_strings;
std::vector<std::string> home_track_strings;
std::vector<std::string> recently_played_strings;

// Actual track data
std::vector<Track> track_data;
std::vector<Track> home_track_data;
std::vector<Track> track_data_saavn;
std::vector<Track> track_data_lastfm;
std::vector<Track> track_data_soundcloud;
std::vector<Track> track_data_forestfm;
std::vector<Track> next_tracks;
std::vector<Track> recently_played;

std::string current_track = "🎵 Music Streaming App 🎵";
std::string current_artist = "Welcome back, User!";

// Playlist track index
int current_track_index = 0;

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
  for (const auto &track : track_data_lastfm) {
    track_data.push_back(track);
  }
  for (const auto &track : track_data_soundcloud) {
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

int main() {

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
    current_track = "🎵 Music Streaming App 🎵";
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
      CatchEvent([&button_text, &next_playlist, &playlist_mutex](Event event) {
        if (event == Event::Return) {
          std::cerr << "Selected: " << selected << std::endl;
          if (selected >= 0 && selected < track_data.size()) {
            current_source = PlaylistSource::Search;
            if (!track_data_forestfm.empty()) {
              player->stop();
            }
            /* player->stop(); */
            player->play(track_data[selected].url);
            recently_played.push_back(track_data[selected]);
            current_track = track_data[selected].name;
            current_artist = track_data[selected].artist;
            button_text = "Pause";
            screen.PostEvent(Event::Custom);

            if (track_data[selected].id != "") {

              std::thread next_tracks_thread([&]() {
                try {
                  std::cerr << "Fetttgiing nextttttttttttttt";
                  next_tracks =
                      saavn.fetch_next_tracks(track_data[selected].id);
                  /* next_tracks.insert(next_tracks.begin(),
                   * track_data[selected]); */
                  std::vector<std::string> next_track_urls;
                  for (const auto &track : next_tracks) {
                    next_track_urls.push_back(track.url);
                  }

                  /* { */
                  /* std::lock_guard<std::mutex> lock(playlist_mutex); */
                  /* next_playlist = next_track_urls; */
                  /* } */
                  player->create_playlist(next_track_urls);

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
            std::string url = track_data[selected].url;
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

        return false;
      });

  Component volume_slider = Slider("", &volume, 0, 100, 1);

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

            // Assuming you have a way to queue events or update on the main thread
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
    if (!track_data_forestfm.empty()) {
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
    player->next_track(track_data, selected);
  });

  // Component tree
  auto component = Container::Vertical({
      // Top section
      Container::Horizontal({
          Container::Vertical({
              playlist_menu,
              play_forestfm,
              play_classic,
              volume_slider,
          }),

          Container::Vertical({
              input_search,
              menu,
              menu2,
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
                }) | center,
                separator(),
                hbox({
                    text("♪ ") | color(Color::Blue),
                    volume_slider->Render(),
                }),
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
                hbox({
                    vbox({
                        menu->Render() | frame | size(HEIGHT, EQUAL, 22) |
                            size(WIDTH, EQUAL, 90),
                    }),
                    separator(),
                    vbox({
                        menu2->Render(),
                    }),
                }),
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
        hbox({text(" λ arch music ") | bgcolor(Color::Blue) |
                  color(Color::White),
              filler(),
              hbox({
                  text("⌨️ ") | dim,
                  text("Space:Play ") | dim,
                  text("↑/↓:Navigate ") | dim,
                  text("+/-:Volume ") | dim,
                  text("S:Shuffle ") | dim,
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
