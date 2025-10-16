#pragma once

#include "../cava/cavacore.h"
#include <memory>
#include <stdexcept>
#include <vector>

class AudioVisualizer {
private:
    struct cava_plan* plan;
    std::vector<double> input_buffer;
    std::vector<double> output_buffer;
    static constexpr int BARS = 16;  // Number of visualization bars
    static constexpr int SAMPLE_RATE = 44100;
    static constexpr int CHANNELS = 2;
    static constexpr double NOISE_REDUCTION = 0.77;
    static constexpr int LOW_CUTOFF = 50;
    static constexpr int HIGH_CUTOFF = 10000;

public:
    AudioVisualizer() {
        // Initialize cava
        plan = cava_init(BARS, SAMPLE_RATE, CHANNELS, 1, 
                        NOISE_REDUCTION, LOW_CUTOFF, HIGH_CUTOFF);
        if (plan->status < 0) {
            throw std::runtime_error(plan->error_message);
        }

        input_buffer.resize(plan->input_buffer_size);
        output_buffer.resize(BARS * CHANNELS);
    }

    ~AudioVisualizer() {
        if (plan) {
            cava_destroy(plan);
            plan = nullptr;
        }
    }

    // Process audio data and return visualization values
    std::vector<double> process(const std::vector<double>& audio_data) {
        if (audio_data.empty()) {
            return std::vector<double>(BARS * CHANNELS, 0.0);
        }

        // Copy input data
        size_t samples_to_process = std::min(audio_data.size(), input_buffer.size());
        std::copy(audio_data.begin(), audio_data.begin() + samples_to_process, 
                 input_buffer.begin());

        // Process through cava
        cava_execute(input_buffer.data(), samples_to_process, 
                    output_buffer.data(), plan);

        return output_buffer;
    }

    int get_num_bars() const { return BARS; }
};
