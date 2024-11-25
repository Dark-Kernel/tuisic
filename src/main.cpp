#include <ftxui/component/component.hpp> // for Renderer, Input, Menu, etc.
#include <ftxui/component/component_base.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/component/screen_interactive.hpp> // for ScreenInteractive
#include "ftxui/dom/elements.hpp"
#include <iostream>
#include <string>
#include <curl/curl.h>
#include <vector>
#include "fetch.cpp"
#include "soundcloud.cpp"

// #include "fetch.h"

std::vector<std::string> track_strings;
// Vector to store full track objects
std::vector<Track> track_data;
std::vector<Track> track_data_soundcloud;
Fetch fetch;
SoundCloud soundcloud;

auto fetch_main(const std::string& query) {
    track_data = fetch.fetch_tracks(query);
    // track_data.insert(track_data.end(), soundcloud.fetch_soundcloud_tracks(query).begin(), soundcloud.fetch_soundcloud_tracks(query).end());
    track_data_soundcloud = soundcloud.fetch_soundcloud_tracks(query);
    for(const auto& track : track_data_soundcloud) {
        track_data.push_back(track);
    }
    track_strings.clear();
    
    // Convert tracks to display strings for menu
    for(const auto& track : track_data) {
        track_strings.push_back(track.to_string());
    }
    
    // Update the menu's data source
    return track_strings;
}

int main() {
  using namespace ftxui;

  // Placeholder for search input
  std::string search_query;

  // Placeholder track list
  std::vector<std::string> tracks = {};

  // Selected track index
  int selected = 0;

  // Components
  Component input_search = Input(&search_query, "Search for music...");
  auto menu = Menu(&tracks, &selected);
  auto play_button = Button("Play", [&] { 
      if(selected >= 0 && selected < track_data.size()) {
          // Play the selected track URL using mpv or your preferred method
          std::string url = track_data[selected].url;
          system(("mpv --no-video --quiet \"" + url + "\"").c_str());
      }
  });
  
  // tracks = fetch_main(search_query);


// In your button handler, you can access the selected track's URL:

  // Component tree
  auto component = Container::Vertical({
              input_search,
              menu,
              play_button
          });

component = component | CatchEvent([&](Event event) {
  if (event == Event::Return) { // Enter key
    bool search_triggered = true;
    // Perform the search action here
    tracks = fetch_main(search_query);
    text("Search triggered for: " + search_query);
    return true; // Event handled
  }
  return false; // Pass unhandled events
});      
      

  // Layout
  auto renderer = Renderer(component, [&] {
    return vbox({
        text("🎵 Music Streaming App 🎵") | bold | center,
        separator(),
        hbox({
            text("Search: ") | bold,
            input_search->Render(),
        }) | border,
        separator(),
        text("Available Tracks:") | bold,
        menu->Render() | frame | flex,
        separator(),
        hbox({
            filler(),
            play_button->Render(),
            filler(),
        }),
        separator(),
        text("Press Enter to search, Space to play.") | dim | center,
    });
  });

  auto screen = ScreenInteractive::Fullscreen();
  screen.Loop(renderer);

  return 0;
}

