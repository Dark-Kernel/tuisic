#pragma once

#include <string>
#include <filesystem>

#ifdef _WIN32
#include <windows.h>
#include <shlobj.h>
#elif defined(__APPLE__)
#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>
#else
#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>
#endif

namespace paths {

inline std::string get_config_dir() {
#ifdef _WIN32
    char* appdata = nullptr;
    size_t len = 0;
    if (_dupenv_s(&appdata, &len, "APPDATA") == 0 && appdata != nullptr) {
        std::string result = std::string(appdata) + "\\tuisic";
        free(appdata);
        return result;
    }
    return ".\\config";
#elif defined(__APPLE__)
    const char* home = getenv("HOME");
    if (home) {
        return std::string(home) + "/Library/Application Support/tuisic";
    }
    return "./config";
#else
    // Linux/Unix - XDG Base Directory Specification
    const char* xdg_config = getenv("XDG_CONFIG_HOME");
    if (xdg_config) {
        return std::string(xdg_config) + "/tuisic";
    }
    
    const char* home = getenv("HOME");
    if (home) {
        return std::string(home) + "/.config/tuisic";
    }
    return "./config";
#endif
}

inline std::string get_data_dir() {
#ifdef _WIN32
    char* appdata = nullptr;
    size_t len = 0;
    if (_dupenv_s(&appdata, &len, "LOCALAPPDATA") == 0 && appdata != nullptr) {
        std::string result = std::string(appdata) + "\\tuisic";
        free(appdata);
        return result;
    }
    return ".\\data";
#elif defined(__APPLE__)
    const char* home = getenv("HOME");
    if (home) {
        return std::string(home) + "/Library/Application Support/tuisic";
    }
    return "./data";
#else
    // Linux/Unix - XDG Base Directory Specification
    const char* xdg_data = getenv("XDG_DATA_HOME");
    if (xdg_data) {
        return std::string(xdg_data) + "/tuisic";
    }
    
    const char* home = getenv("HOME");
    if (home) {
        return std::string(home) + "/.local/share/tuisic";
    }
    return "./data";
#endif
}

inline std::string get_cache_dir() {
#ifdef _WIN32
    char* temp = nullptr;
    size_t len = 0;
    if (_dupenv_s(&temp, &len, "TEMP") == 0 && temp != nullptr) {
        std::string result = std::string(temp) + "\\tuisic";
        free(temp);
        return result;
    }
    return ".\\cache";
#elif defined(__APPLE__)
    const char* home = getenv("HOME");
    if (home) {
        return std::string(home) + "/Library/Caches/tuisic";
    }
    return "./cache";
#else
    // Linux/Unix - XDG Base Directory Specification
    const char* xdg_cache = getenv("XDG_CACHE_HOME");
    if (xdg_cache) {
        return std::string(xdg_cache) + "tuisic";
    }
    
    const char* home = getenv("HOME");
    if (home) {
        return std::string(home) + "/.cache/tuisic";
    }
    return "./cache";
#endif
}

inline std::string get_music_dir() {
#ifdef _WIN32
    char* profile = nullptr;
    size_t len = 0;
    if (_dupenv_s(&profile, &len, "USERPROFILE") == 0 && profile != nullptr) {
        std::string result = std::string(profile) + "\\Music";
        free(profile);
        return result;
    }
    return ".\\Music";
#elif defined(__APPLE__)
    const char* home = getenv("HOME");
    if (home) {
        return std::string(home) + "/Music";
    }
    return "./Music";
#else
    const char* home = getenv("HOME");
    if (home) {
        return std::string(home) + "/Music";
    }
    return "./Music";
#endif
}

// Ensure directory exists
inline void ensure_directory_exists(const std::string& path) {
    try {
        std::filesystem::create_directories(path);
    } catch (const std::exception& e) {
        // Silently fail - the application will handle this gracefully
    }
}

} // namespace paths
