#pragma once
#include <string>

struct Track {
    std::string name;
    std::string artist;
    std::string url;
    
    // Convert to display string for FTXUI menu
    std::string to_string() const {
        return name + " - " + artist;
    }
};
