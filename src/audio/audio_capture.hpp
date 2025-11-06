#pragma once

#ifdef WITH_CAVA

#include <pulse/pulseaudio.h>
#include <pulse/simple.h>
#include <atomic>
#include <functional>
#include <thread>
#include <vector>
#include <cstring>
#include <iostream>

class AudioCapture {
private:
    pa_simple* pulse_connection = nullptr;
    std::thread capture_thread;
    std::atomic<bool> should_stop{false};
    std::function<void(const std::vector<double>&)> callback;

    static constexpr int SAMPLE_RATE = 44100;
    static constexpr int CHANNELS = 2;
    static constexpr int BUFFER_SIZE = 2048;

public:
    AudioCapture() = default;

    ~AudioCapture() {
        stop();
    }

    void set_callback(std::function<void(const std::vector<double>&)> cb) {
        callback = std::move(cb);
    }

    bool start(const char* device_name = nullptr) {
        if (pulse_connection) {
            return true; // Already started
        }

        // PulseAudio sample spec
        pa_sample_spec sample_spec;
        sample_spec.format = PA_SAMPLE_FLOAT32LE;
        sample_spec.rate = SAMPLE_RATE;
        sample_spec.channels = CHANNELS;

        // Buffer attributes for low latency
        pa_buffer_attr buffer_attr;
        buffer_attr.maxlength = (uint32_t) -1;
        buffer_attr.tlength = (uint32_t) -1;
        buffer_attr.prebuf = (uint32_t) -1;
        buffer_attr.minreq = (uint32_t) -1;
        buffer_attr.fragsize = BUFFER_SIZE * sizeof(float);

        int error;

        // Use monitor source to capture audio output
        // @DEFAULT_MONITOR@ is the monitor of the default sink
        const char* source = device_name ? device_name : "@DEFAULT_MONITOR@";

        std::cout << "[AudioCapture] Connecting to PulseAudio source: " << source << std::endl;

        // Connect to PulseAudio monitor (capture from audio output)
        pulse_connection = pa_simple_new(
            nullptr,                          // Use default server
            "tuisic",                         // Application name
            PA_STREAM_RECORD,                 // Record stream
            source,                           // Monitor source
            "Music Visualization",            // Stream description
            &sample_spec,                     // Sample format
            nullptr,                          // Use default channel map
            &buffer_attr,                     // Buffer attributes
            &error                            // Error code
        );

        if (!pulse_connection) {
            std::cerr << "[AudioCapture] Failed to connect to PulseAudio: " << pa_strerror(error) << std::endl;
            std::cerr << "[AudioCapture] Make sure PulseAudio is running and you have a default sink" << std::endl;
            return false;
        }

        std::cout << "[AudioCapture] Successfully connected to PulseAudio!" << std::endl;

        // Start capture thread
        should_stop = false;
        capture_thread = std::thread([this]() {
            this->capture_loop();
        });

        std::cout << "[AudioCapture] Capture thread started" << std::endl;

        return true;
    }

    void stop() {
        should_stop = true;

        if (capture_thread.joinable()) {
            capture_thread.join();
        }

        if (pulse_connection) {
            pa_simple_free(pulse_connection);
            pulse_connection = nullptr;
        }
    }

private:
    void capture_loop() {
        std::vector<float> buffer(BUFFER_SIZE * CHANNELS);
        std::vector<double> double_buffer(BUFFER_SIZE * CHANNELS);

        int error;
        int read_count = 0;

        std::cout << "[AudioCapture] Capture loop started, waiting for audio data..." << std::endl;

        while (!should_stop) {
            // Read audio data from PulseAudio
            if (pa_simple_read(pulse_connection, buffer.data(),
                              buffer.size() * sizeof(float), &error) < 0) {
                std::cerr << "[AudioCapture] PulseAudio read error: " << pa_strerror(error) << std::endl;
                break;
            }

            read_count++;
            if (read_count % 100 == 0) {
                std::cout << "[AudioCapture] Read " << read_count << " audio buffers" << std::endl;
            }

            // Convert float to double and pass to callback
            if (callback) {
                for (size_t i = 0; i < buffer.size(); i++) {
                    double_buffer[i] = static_cast<double>(buffer[i]);
                }
                callback(double_buffer);
            } else {
                if (read_count == 1) {
                    std::cerr << "[AudioCapture] Warning: No callback set!" << std::endl;
                }
            }
        }

        std::cout << "[AudioCapture] Capture loop stopped" << std::endl;
    }
};

#endif // WITH_CAVA
