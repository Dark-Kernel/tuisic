#pragma once
#include <string>

struct Track {
    std::string name;
    std::string artist;
    std::string url;
    std::string id;
    
    // Convert to display string for FTXUI menu
    std::string to_string() const {
        return name + " - " + artist;
    }
};
