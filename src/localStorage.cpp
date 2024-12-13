#include "Track.h"
#include "rapidjson/document.h"
#include "rapidjson/filewritestream.h"
#include "rapidjson/writer.h"
#include <cstdio>
#include <iostream>
#include <ostream>
#include <vector>

/* void saveData(const std::vector<Track> &recentlyPlayed, */
/*               const std::vector<Track> &favorites_tracks) { */

/*   rapidjson::Document data; */
/*   data.SetObject(); */

/*   data.AddMember("recentlyPlayed", recentlyPlayed, data.GetAllocator()); */
/*   data.AddMember("favorites", favorites_tracks, data.GetAllocator()); */

/*   FILE *outFile = fopen("config.json", "wb"); */

/*   char writeBuffer[65536]; */
/*   rapidjson::FileWriteStream os(outFile, writeBuffer, sizeof(writeBuffer)); */
/*   rapidjson::Writer<rapidjson::FileWriteStream> writer(os); */
/*   data.Accept(writer); */
/*   fclose(outFile); */
/* } */

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
    FILE* outFile = fopen("config.json", "wb");
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
