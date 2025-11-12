#pragma once

#include <iostream>
#include <string>
#include <memory>
#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>
#include "command_handler.hpp"

namespace ai {

class MCPServer {
private:
    std::shared_ptr<CommandHandler> command_handler;

public:
    MCPServer(std::shared_ptr<CommandHandler> handler)
        : command_handler(handler) {}

    // Start MCP server (reads from stdin, writes to stdout)
    void run() {
        std::string line;
        while (std::getline(std::cin, line)) {
            if (line.empty()) continue;

            std::string response = handle_request(line);
            std::cout << response << std::endl;
            std::cout.flush();
        }
    }

private:
    std::string handle_request(const std::string& request_str) {
        rapidjson::Document request;
        if (request.Parse(request_str.c_str()).HasParseError()) {
            return create_error_response(-1, "Invalid JSON");
        }

        if (!request.HasMember("method") || !request["method"].IsString()) {
            return create_error_response(-1, "Missing method");
        }

        std::string method = request["method"].GetString();
        int id = request.HasMember("id") && request["id"].IsInt() ? request["id"].GetInt() : -1;

        if (method == "initialize") {
            return handle_initialize(id);
        }
        else if (method == "tools/list") {
            return handle_tools_list(id);
        }
        else if (method == "tools/call") {
            return handle_tools_call(id, request);
        }
        else {
            return create_error_response(id, "Unknown method: " + method);
        }
    }

    std::string handle_initialize(int id) {
        rapidjson::Document doc;
        doc.SetObject();
        auto& allocator = doc.GetAllocator();

        doc.AddMember("jsonrpc", "2.0", allocator);
        doc.AddMember("id", id, allocator);

        rapidjson::Value result(rapidjson::kObjectType);

        // Use the current MCP protocol version (date-based)
        result.AddMember("protocolVersion", "2024-11-05", allocator);

        rapidjson::Value serverInfo(rapidjson::kObjectType);
        serverInfo.AddMember("name", "tuisic", allocator);
        serverInfo.AddMember("version", "1.0.0", allocator);
        result.AddMember("serverInfo", serverInfo, allocator);

        rapidjson::Value capabilities(rapidjson::kObjectType);
        rapidjson::Value tools(rapidjson::kObjectType);
        capabilities.AddMember("tools", tools, allocator);
        result.AddMember("capabilities", capabilities, allocator);

        doc.AddMember("result", result, allocator);

        std::cerr << "[MCP] Initialized with protocol version 2024-11-05" << std::endl;
        return document_to_string(doc);
    }

    std::string handle_tools_list(int id) {
        rapidjson::Document doc;
        doc.SetObject();
        auto& allocator = doc.GetAllocator();

        doc.AddMember("jsonrpc", "2.0", allocator);
        doc.AddMember("id", id, allocator);

        rapidjson::Value result(rapidjson::kObjectType);
        rapidjson::Value tools(rapidjson::kArrayType);

        // Define all available tools
        tools.PushBack(create_tool("music_play", "Play a song or resume playback",
            R"json({"query": {"type": "string", "description": "Song name, artist, or search query. Leave empty to resume."}})json", allocator), allocator);

        tools.PushBack(create_tool("music_pause", "Pause current playback", "{}", allocator), allocator);

        tools.PushBack(create_tool("music_next", "Skip to next track", "{}", allocator), allocator);

        tools.PushBack(create_tool("music_previous", "Go to previous track", "{}", allocator), allocator);

        tools.PushBack(create_tool("music_stop", "Stop playback", "{}", allocator), allocator);

        tools.PushBack(create_tool("music_search", "Search for music",
            R"json({"query": {"type": "string", "description": "Search query for songs"}})json", allocator), allocator);

        tools.PushBack(create_tool("music_status", "Get current playback status", "{}", allocator), allocator);

        tools.PushBack(create_tool("music_volume", "Set volume level",
            R"json({"level": {"type": "number", "description": "Volume level (0-100)"}})json", allocator), allocator);

        tools.PushBack(create_tool("music_seek", "Seek to position",
            R"json({"position": {"type": "number", "description": "Position in seconds"}})json", allocator), allocator);

        result.AddMember("tools", tools, allocator);
        doc.AddMember("result", result, allocator);

        return document_to_string(doc);
    }

    std::string handle_tools_call(int id, rapidjson::Document& request) {
        if (!request.HasMember("params") || !request["params"].IsObject()) {
            return create_error_response(id, "Missing params");
        }

        auto& params = request["params"];
        if (!params.HasMember("name") || !params["name"].IsString()) {
            return create_error_response(id, "Missing tool name");
        }

        std::string tool_name = params["name"].GetString();
        rapidjson::Value arguments(rapidjson::kObjectType);
        if (params.HasMember("arguments") && params["arguments"].IsObject()) {
            arguments.CopyFrom(params["arguments"], request.GetAllocator());
        }

        // Map tool names to commands
        std::string command;
        if (tool_name == "music_play") {
            std::string query = arguments.HasMember("query") && arguments["query"].IsString() ?
                arguments["query"].GetString() : "";
            command = query.empty() ? "play" : "play " + query;
        }
        else if (tool_name == "music_pause") {
            command = "pause";
        }
        else if (tool_name == "music_next") {
            command = "next";
        }
        else if (tool_name == "music_previous") {
            command = "previous";
        }
        else if (tool_name == "music_stop") {
            command = "stop";
        }
        else if (tool_name == "music_search") {
            std::string query = arguments.HasMember("query") && arguments["query"].IsString() ?
                arguments["query"].GetString() : "";
            command = "search " + query;
        }
        else if (tool_name == "music_status") {
            command = "status";
        }
        else if (tool_name == "music_volume") {
            int level = arguments.HasMember("level") && arguments["level"].IsInt() ?
                arguments["level"].GetInt() : 50;
            command = "volume " + std::to_string(level);
        }
        else if (tool_name == "music_seek") {
            double pos = arguments.HasMember("position") && arguments["position"].IsDouble() ?
                arguments["position"].GetDouble() : 0.0;
            command = "seek " + std::to_string(pos);
        }
        else {
            return create_error_response(id, "Unknown tool: " + tool_name);
        }

        // Execute command
        std::string result_str = command_handler->execute(command);

        // Create MCP response
        rapidjson::Document doc;
        doc.SetObject();
        auto& allocator = doc.GetAllocator();

        doc.AddMember("jsonrpc", "2.0", allocator);
        doc.AddMember("id", id, allocator);

        rapidjson::Value result(rapidjson::kObjectType);
        rapidjson::Value content(rapidjson::kArrayType);

        rapidjson::Value text_content(rapidjson::kObjectType);
        text_content.AddMember("type", "text", allocator);
        text_content.AddMember("text", rapidjson::Value(result_str.c_str(), allocator), allocator);
        content.PushBack(text_content, allocator);

        result.AddMember("content", content, allocator);
        doc.AddMember("result", result, allocator);

        return document_to_string(doc);
    }

    rapidjson::Value create_tool(
        const std::string& name,
        const std::string& description,
        const std::string& input_schema,
        rapidjson::Document::AllocatorType& allocator
    ) {
        rapidjson::Value tool(rapidjson::kObjectType);
        tool.AddMember("name", rapidjson::Value(name.c_str(), allocator), allocator);
        tool.AddMember("description", rapidjson::Value(description.c_str(), allocator), allocator);

        // Parse input schema
        rapidjson::Document schema_doc;
        schema_doc.Parse(input_schema.c_str());
        rapidjson::Value input_schema_obj(rapidjson::kObjectType);
        input_schema_obj.AddMember("type", "object", allocator);
        input_schema_obj.AddMember("properties", rapidjson::Value(schema_doc, allocator), allocator);

        tool.AddMember("inputSchema", input_schema_obj, allocator);

        return tool;
    }

    std::string create_error_response(int id, const std::string& message) {
        rapidjson::Document doc;
        doc.SetObject();
        auto& allocator = doc.GetAllocator();

        doc.AddMember("jsonrpc", "2.0", allocator);
        if (id >= 0) {
            doc.AddMember("id", id, allocator);
        }

        rapidjson::Value error(rapidjson::kObjectType);
        error.AddMember("code", -32600, allocator);
        error.AddMember("message", rapidjson::Value(message.c_str(), allocator), allocator);

        doc.AddMember("error", error, allocator);

        return document_to_string(doc);
    }

    std::string document_to_string(const rapidjson::Document& doc) {
        rapidjson::StringBuffer buffer;
        rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
        doc.Accept(writer);
        return buffer.GetString();
    }
};

} // namespace ai
