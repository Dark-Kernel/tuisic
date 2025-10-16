#pragma once

#include <mpv/client.h>
#include <string>
#include <vector>
#include <memory>

class MusicPlayer {
public:
    MusicPlayer();
    ~MusicPlayer();

    void play(const std::string& url);
    void pause();
    void resume();
    void stop();
    void seek(double seconds);
    void set_volume(int volume);
    int get_volume() const;
    double get_position() const;
    double get_duration() const;
    bool is_playing() const;

private:
    void initialize();
    void shutdown();

    mpv_handle* mpv;
    bool playing;
};

