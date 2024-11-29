#ifndef MAIN_HPP
#define MAIN_HPP

#include "fetch.hpp"
#include "justmusic.hpp"
#include "player.hpp"
#include "soundcloud.hpp"
#include <curl/curl.h>
#include <curl/urlapi.h>
#include <ftxui/component/component.hpp> // for Renderer, Input, Menu, etc.
#include <ftxui/component/component_base.hpp>
#include <ftxui/component/component_options.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/component/screen_interactive.hpp> // for ScreenInteractive
#include <ftxui/screen/color.hpp>
#include <fmt/core.h>
#include <fmt/format.h>
#include <iostream>
#include <map>
#include <string>
#include <vector>

// Enums
enum class PlaylistSource { None, Search, ForestFM };

// Functions
std::string format_time(double seconds);

// Globals
extern std::vector<std::string> track_strings;
extern std::vector<std::string> track_strings_forestfm;
extern std::vector<Track> track_data;
extern std::vector<Track> track_data_soundcloud;
extern std::vector<Track> track_data_forestfm;
extern std::string current_track;
extern std::string current_artist;
extern int current_track_index;

extern Fetch fetch;
extern SoundCloud soundcloud;
extern std::shared_ptr<MusicPlayer> player;

extern PlaylistSource current_source;
extern ftxui::ScreenInteractive screen;
extern int selected;

extern std::map<std::string, std::vector<std::string>> ascii_art;
std::vector<std::string> get_track_ascii_art(const Track &track);
std::vector<std::string> fetch_main(const std::string &query);
void switch_playlist_source(const std::vector<Track> &new_tracks);

#endif // MAIN_HPP

