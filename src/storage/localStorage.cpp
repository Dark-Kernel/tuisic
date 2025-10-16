#include "../common/Track.h"
#include "../common/paths.hpp"
#include "rapidjson/document.h"
#include "rapidjson/filewritestream.h"
#include "rapidjson/filereadstream.h"
#include "rapidjson/writer.h"
#include <cstdio>
#include <iostream>
#include <ostream>
#include <vector>
#include <filesystem>

void saveData(const std::vector<Track> &recentlyPlayed,
              const std::vector<Track> &favorites_tracks) {
    rapidjson::Document data;
    data.SetObject();
    rapidjson::Document::AllocatorType& allocator = data.GetAllocator();

    // Create JSON array for recently played tracks
    rapidjson::Value recentlyPlayedArray(rapidjson::kArrayType);
    for (const auto& track : recentlyPlayed) {
        rapidjson::Value trackObj(rapidjson::kObjectType);
        trackObj.AddMember("name", rapidjson::StringRef(track.name.c_str()), allocator);
        trackObj.AddMember("artist", rapidjson::StringRef(track.artist.c_str()), allocator);
        trackObj.AddMember("url", rapidjson::StringRef(track.url.c_str()), allocator);
        recentlyPlayedArray.PushBack(trackObj, allocator);
    }
    data.AddMember("recentlyPlayed", recentlyPlayedArray, allocator);

    // Create JSON array for favorite tracks (same process)
    rapidjson::Value favoritesArray(rapidjson::kArrayType);
    for (const auto& track : favorites_tracks) {
        rapidjson::Value trackObj(rapidjson::kObjectType);
        trackObj.AddMember("name", rapidjson::StringRef(track.name.c_str()), allocator);
        trackObj.AddMember("artist", rapidjson::StringRef(track.artist.c_str()), allocator);
        trackObj.AddMember("url", rapidjson::StringRef(track.url.c_str()), allocator);
        favoritesArray.PushBack(trackObj, allocator);
    }
    data.AddMember("favorites", favoritesArray, allocator);

    // Write to file
    std::string data_dir = paths::get_data_dir();
    paths::ensure_directory_exists(data_dir);
    std::string tracks_file = data_dir + "/tracks.json";
    FILE* outFile = fopen(tracks_file.c_str(), "wb");
    if (!outFile) {
        std::cerr << "Failed to open file for writing" << std::endl;
        return;
    }

    char writeBuffer[65536];
    rapidjson::FileWriteStream os(outFile, writeBuffer, sizeof(writeBuffer));
    rapidjson::Writer<rapidjson::FileWriteStream> writer(os);
    data.Accept(writer);

    fclose(outFile);
}


bool loadData(std::vector<Track> &recentlyPlayed,
              std::vector<Track> &favorites_tracks) {
    std::string data_dir = paths::get_data_dir();
    std::string tracks_file = data_dir + "/tracks.json";
    FILE* inFile = fopen(tracks_file.c_str(), "rb");
    if (!inFile) {
        std::cerr << "Failed to open tracks file" << std::endl;
        return false;
    }

    char readBuffer[65536];
    rapidjson::FileReadStream is(inFile, readBuffer, sizeof(readBuffer));
    rapidjson::Document doc;
    doc.ParseStream(is);
    fclose(inFile);

    if (doc.HasParseError()) {
        std::cerr << "JSON parse error" << std::endl;
        return false;
    }

    // Clear existing vectors
    recentlyPlayed.clear();
    favorites_tracks.clear();

    // Load recently played tracks
    if (doc.HasMember("recentlyPlayed") && doc["recentlyPlayed"].IsArray()) {
        for (const auto& trackJson : doc["recentlyPlayed"].GetArray()) {
            Track track;
            track.name = trackJson["name"].GetString();
            track.artist = trackJson["artist"].GetString();
            track.url = trackJson["url"].GetString();
            recentlyPlayed.push_back(track);
        }
    }

    // Load favorite tracks
    if (doc.HasMember("favorites") && doc["favorites"].IsArray()) {
        for (const auto& trackJson : doc["favorites"].GetArray()) {
            Track track;
            track.name = trackJson["name"].GetString();
            track.artist = trackJson["artist"].GetString();
            track.url = trackJson["url"].GetString();
            favorites_tracks.push_back(track);
        }
    }

    return true;
}
