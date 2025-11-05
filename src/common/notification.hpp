#pragma once

#include <string>
#include <cstdlib>
#include "../core/config/config.hpp"

namespace notifications {

// Global config pointer - to be set by main()
inline Config* g_config = nullptr;

inline void init(Config* cfg) {
    g_config = cfg;
}

inline void send(const std::string& message) {
    // Check if config is initialized
    if (g_config == nullptr) {
        // Fallback: send notification anyway if config not available
        system(("notify-send \"tuisic\" \"" + message + "\"").c_str());
        return;
    }

    // Check config setting
    if (g_config->get_notifications_enabled()) {
        system(("notify-send \"tuisic\" \"" + message + "\"").c_str());
    }
}

inline void send_download_complete(const std::string& track_name) {
    send("Download completed: " + track_name);
}

inline void send_download_failed(const std::string& error) {
    send("Download failed: " + error);
}

inline void send_download_started(const std::string& track_name) {
    send("Started downloading: " + track_name);
}

inline void send_url_copied() {
    send("URL copied to clipboard");
}

inline void send_daemon_started() {
    send("tuisic daemon started");
}

} // namespace notifications
