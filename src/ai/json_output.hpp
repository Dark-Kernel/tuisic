#pragma once

#include <rapidjson/document.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include <string>
#include <vector>
#include "../common/Track.h"

namespace ai {

class JsonOutput {
public:
    // Create status JSON
    static std::string create_status(
        const std::string& status,
        const std::string& track_name,
        const std::string& artist,
        double position,
        double duration,
        int volume
    ) {
        rapidjson::Document doc;
        doc.SetObject();
        auto& allocator = doc.GetAllocator();

        doc.AddMember("status", rapidjson::Value(status.c_str(), allocator), allocator);
        doc.AddMember("track", rapidjson::Value(track_name.c_str(), allocator), allocator);
        doc.AddMember("artist", rapidjson::Value(artist.c_str(), allocator), allocator);
        doc.AddMember("position", position, allocator);
        doc.AddMember("duration", duration, allocator);
        doc.AddMember("volume", volume, allocator);

        return document_to_string(doc);
    }

    // Create search results JSON
    static std::string create_search_results(const std::vector<Track>& tracks) {
        rapidjson::Document doc;
        doc.SetObject();
        auto& allocator = doc.GetAllocator();

        rapidjson::Value results(rapidjson::kArrayType);

        for (const auto& track : tracks) {
            rapidjson::Value track_obj(rapidjson::kObjectType);
            track_obj.AddMember("name", rapidjson::Value(track.name.c_str(), allocator), allocator);
            track_obj.AddMember("artist", rapidjson::Value(track.artist.c_str(), allocator), allocator);
            track_obj.AddMember("url", rapidjson::Value(track.url.c_str(), allocator), allocator);
            track_obj.AddMember("id", rapidjson::Value(track.id.c_str(), allocator), allocator);
            track_obj.AddMember("source", rapidjson::Value(track.source.c_str(), allocator), allocator);
            results.PushBack(track_obj, allocator);
        }

        doc.AddMember("results", results, allocator);
        doc.AddMember("count", static_cast<int>(tracks.size()), allocator);

        return document_to_string(doc);
    }

    // Create success response
    static std::string create_success(const std::string& message) {
        rapidjson::Document doc;
        doc.SetObject();
        auto& allocator = doc.GetAllocator();

        doc.AddMember("success", true, allocator);
        doc.AddMember("message", rapidjson::Value(message.c_str(), allocator), allocator);

        return document_to_string(doc);
    }

    // Create error response
    static std::string create_error(const std::string& error) {
        rapidjson::Document doc;
        doc.SetObject();
        auto& allocator = doc.GetAllocator();

        doc.AddMember("success", false, allocator);
        doc.AddMember("error", rapidjson::Value(error.c_str(), allocator), allocator);

        return document_to_string(doc);
    }

    // Create playlist JSON
    static std::string create_playlist(const std::vector<Track>& tracks) {
        rapidjson::Document doc;
        doc.SetObject();
        auto& allocator = doc.GetAllocator();

        rapidjson::Value playlist(rapidjson::kArrayType);

        for (const auto& track : tracks) {
            rapidjson::Value track_obj(rapidjson::kObjectType);
            track_obj.AddMember("name", rapidjson::Value(track.name.c_str(), allocator), allocator);
            track_obj.AddMember("artist", rapidjson::Value(track.artist.c_str(), allocator), allocator);
            playlist.PushBack(track_obj, allocator);
        }

        doc.AddMember("playlist", playlist, allocator);
        doc.AddMember("count", static_cast<int>(tracks.size()), allocator);

        return document_to_string(doc);
    }

private:
    static std::string document_to_string(const rapidjson::Document& doc, bool pretty = false) {
        rapidjson::StringBuffer buffer;

        if (pretty) {
            rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buffer);
            doc.Accept(writer);
        } else {
            rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
            doc.Accept(writer);
        }

        return buffer.GetString();
    }
};

} // namespace ai
