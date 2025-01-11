#pragma once

#include <filesystem>
#include <fstream>
#include <iostream>
#include <mutex>
#include <rapidjson/document.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

namespace fs = std::filesystem;

class Config {
private:
  rapidjson::Document config;
  std::string config_path;
  mutable std::mutex config_mutex;

  void create_default_config() {
    config.SetObject();
    auto &allocator = config.GetAllocator();

    const char *home = getenv("HOME");
    std::string home_dir = home ? home : "/tmp";

    rapidjson::Value downloads(rapidjson::kObjectType);
    std::string default_music_path = home_dir + "/Music";
    downloads.AddMember("path",
                        rapidjson::Value(default_music_path.c_str(), allocator),
                        allocator);
    downloads.AddMember("format", "mp3", allocator);
    downloads.AddMember("quality", "best", allocator);
    config.AddMember("downloads", downloads, allocator);

    /* // Downloads section */
    /* rapidjson::Value downloads(rapidjson::kObjectType); */
    /* downloads.AddMember("path", rapidjson::Value((std::string(getenv("HOME"))
     * + "/Music").c_str(), allocator), allocator); */
    /* downloads.AddMember("format", "mp3", allocator); */
    /* downloads.AddMember("quality", "best", allocator); */
    /* config.AddMember("downloads", downloads, allocator); */

    // Player section
    rapidjson::Value player(rapidjson::kObjectType);
    player.AddMember("volume", 100, allocator);
    player.AddMember("subtitle_enabled", true, allocator);
    player.AddMember("repeat_enabled", false, allocator);
    config.AddMember("player", player, allocator);

    // UI section
    rapidjson::Value ui(rapidjson::kObjectType);
    ui.AddMember("theme", "dark", allocator);
    ui.AddMember("show_notifications", true, allocator);
    ui.AddMember("notification_timeout", 3000, allocator);
    config.AddMember("ui", ui, allocator);

    // Cache section
    rapidjson::Value cache(rapidjson::kObjectType);
    cache.AddMember("enabled", true, allocator);
    cache.AddMember("max_size_mb", 100, allocator);
    cache.AddMember(
        "path",
        rapidjson::Value(
            (std::string(getenv("HOME")) + "/.cache/musicplayer").c_str(),
            allocator),
        allocator);
    config.AddMember("cache", cache, allocator);

    // Create directories if they don't exist
    /* std::filesystem::create_directories( */
    /*     std::filesystem::path(get_download_path()).parent_path() */
    /* ); */
    namespace fs = std::filesystem;

    try {
      fs::create_directories(default_music_path);
    } catch (const std::exception &e) {
      std::cerr << "Failed to create download directory: " << e.what()
                << std::endl;
    }

    save_config();
  }

  void save_config() {
    rapidjson::StringBuffer buffer;
    rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buffer);
    config.Accept(writer);

    std::ofstream file(config_path);
    file << buffer.GetString() << std::endl;
  }

  std::string get_string_value(const char *section, const char *key,
                               const std::string &default_value = "") const {
    if (config.HasMember(section) && config[section].HasMember(key) &&
        config[section][key].IsString()) {
      return config[section][key].GetString();
    }
    return default_value;
  }

  bool get_bool_value(const char *section, const char *key,
                      bool default_value = false) const {
    if (config.HasMember(section) && config[section].HasMember(key) &&
        config[section][key].IsBool()) {
      return config[section][key].GetBool();
    }
    return default_value;
  }

  int get_int_value(const char *section, const char *key,
                    int default_value = 0) const {
    if (config.HasMember(section) && config[section].HasMember(key) &&
        config[section][key].IsInt()) {
      return config[section][key].GetInt();
    }
    return default_value;
  }

public:
  Config(const std::string &path = std::string(getenv("HOME")) +
                                   "/.config/tuisic/config.json")
      : config_path(path) {
    try {
      // Create config directory if it doesn't exist
      std::filesystem::create_directories(
          std::filesystem::path(config_path).parent_path());

      // Try to read existing config
      std::ifstream file(config_path);
      if (file.good()) {
        std::string json_str((std::istreambuf_iterator<char>(file)),
                             std::istreambuf_iterator<char>());

        if (config.Parse(json_str.c_str()).HasParseError() ||
            !config.IsObject()) {
          throw std::runtime_error("Invalid config file format");
        }
      } else {
        create_default_config();
      }
    } catch (const std::exception &e) {
      std::cerr << "Config error: " << e.what() << std::endl;
      create_default_config();
    }
  }


  std::string get_download_path() const {
    return get_string_value("downloads", "path", std::string(getenv("HOME"))
    + "/Music"); 
    /* return get_string_value("downloads", "path"); */
          /* std::lock_guard<std::mutex> lock(config_mutex); */
        
        /* try { */
          /*   if (config.HasMember("downloads") && */ 
          /*       config["downloads"].HasMember("path") && */ 
          /*       config["downloads"]["path"].IsString()) { */
                
          /*       std::string path = config["downloads"]["path"].GetString(); */
                
          /*       // Ensure the directory exists */
          /*       if (!fs::exists(path)) { */
          /*           fs::create_directories(path); */
          /*       } */
          /*       return path; */
          /*   } */
            
          /*   // Fallback to default path if config value is invalid */
          /*   std::string default_path = std::string(getenv("HOME") ? getenv("HOME") : "/tmp") + "/Music"; */
          /*   fs::create_directories(default_path); */
          /*   return default_path; */
            
        /* } catch (const std::exception& e) { */
          /*   std::cerr << "Error in get_download_path: " << e.what() << std::endl; */
          /*   // Return a safe fallback path */
          /*   return "/tmp"; */
        /* } */
  }

  std::string get_download_format() const {
    return get_string_value("downloads", "format", "mp3");
  }

  bool get_subtitle_enabled() const {
    return get_bool_value("player", "subtitle_enabled", true);
  }

  int get_volume() const { return get_int_value("player", "volume", 100); }

  bool get_notifications_enabled() const {
    return get_bool_value("ui", "show_notifications", true);
  }

  void set_download_path(const std::string &path) {
    if (config.HasMember("downloads")) {
      config["downloads"]["path"].SetString(path.c_str(),
                                            config.GetAllocator());
      save_config();
    }
  }
};
