#include <ftxui/component/component.hpp> // for Renderer, Input, Menu, etc.
#include <ftxui/component/component_base.hpp>
#include <ftxui/component/component_options.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/component/screen_interactive.hpp> // for ScreenInteractive
#include "ftxui/dom/elements.hpp"
#include <iostream>
#include <string>
#include <curl/curl.h>
#include <vector>
#include "fetch.cpp"
#include "soundcloud.cpp"
#include "player.cpp"


std::vector<std::string> track_strings;
std::vector<Track> track_data;
std::vector<Track> track_data_soundcloud;
Fetch fetch;
SoundCloud soundcloud;

#include <fmt/format.h>
#include <fmt/core.h>

std::string format_time(double seconds) {
    int minutes = static_cast<int>(seconds) / 60;
    int secs = static_cast<int>(seconds) % 60;
    return fmt::format("{:02d}:{:02d}", minutes, secs);
}

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
    return track_strings;
}

int main() {
    using namespace ftxui;

    auto screen = ScreenInteractive::Fullscreen();
    // Placeholder for search input
    std::string search_query;

    // Placeholder track list
    std::vector<std::string> tracks = {};

    // Selected track index
    int selected = 0;

    // for progress
    int progress = 0;
       
    // Volume control
    int volume = 100;

    auto player = std::make_shared<MusicPlayer>();

    // Components
    Component input_search = Input(&search_query, "Search for music...");
    auto menu = Menu(&tracks, &selected);
    // auto play_button = Button("Play", [&] { 
    //         if(selected >= 0 && selected < track_data.size()) {
    //         // Play the selected track URL using mpv
    //         std::string url = track_data[selected].url;
    //         system(("mpv --no-video --quiet \"" + url + "\"").c_str());
    //         }
    // });
    // Component progress_slider = Slider( "Val", 0, track_data.size() - 1, &selected);

    Component volume_slider = Slider("", &volume, 0, 100, 1);

    std::string button_text = "Play";
    auto play_button = Button(&button_text, [&] {
        if(player->is_playing_state()) {
            player->pause();            
        }else if(selected >= 0 && selected < track_data.size()) {
            player->play(track_data[selected].url);            
        } 
    });

    // player callbacks
    player->set_state_callback([&]{
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
            // progress_percentage = std::min(100, std::max(0, progress_percentage));
            // progress_percentage = std::min(100, std::max(0, static_cast<int>((pos / dur) * 100.0)));
            screen.PostEvent(Event::Custom);
        }
    });

    Component progress_slider = Slider("", &progress_percentage, 0, 100, 1);
    // progress_slider |= CatchEvent([&](Event event) {
    // if (event == Event::Custom) {
    //     // Seek to the new position when slider is moved
    //     if (total_duration > 0) {
    //         double seek_position = (progress_percentage / 100.0) * total_duration;
    //         seek_position = std::min(seek_position, total_duration);
    //         // seek_position = std::min(total_duration, std::max(0.0, seek_position));
    //         player->seek(seek_position);
    //     }
    //     return true;
    // }
    // return false;
    // });

    auto button_prev = Button("<-", [&] {
            player->previous_track(track_data, selected);
            text("chaing");
            });

    auto button_next = Button("->", [&] {
            player->next_track(track_data, selected);
            text("chaing");
            });

    player->set_end_of_track_callback([&] {
            // Automatically play next track
            player->next_track(track_data, selected);
            });

    // Component tree
    auto component = Container::Vertical({
            input_search,
            menu,
            progress_slider,
            play_button
            });

    component = component | CatchEvent([&](Event event) {
            if (event == Event::Return) { // Enter key
            bool search_triggered = true;
            // search action
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
                            text(format_time(current_position)),
                            progress_slider->Render() | flex,
                            text(format_time(total_duration)),
                }),
                    hbox({
                            filler(),
                            button_prev->Render(),
                            play_button->Render(),
                            button_next->Render(),
                            filler(),
                            }) | center,
                    separator(),
                    text("Press Enter to search, Space to play.") | dim | center,
                    });
            });

    progress_slider |= CatchEvent([&](Event event) {
        if (event == Event::Custom) {
            if(player->get_duration() > 0) {
                // double seek_pos = static_cast<double>(progress) / 100.0 * player->get_duration();
                double seek_pos = (progress / 100.0) * player->get_duration();
                player->seek(seek_pos);
            }
            return true;
        }
        return false;
    });
    screen.Loop(renderer);

    return 0;
}

