#pragma once
#include <string>

struct Track {
    std::string name;
    std::string artist;
    std::string url;
    std::string id;
    std::string source;
    std::string coverImage;
    std::string language;
    
    // Convert to display string for FTXUI menu
    std::string to_string() const {
        return name + " - " + artist;
    }
};
