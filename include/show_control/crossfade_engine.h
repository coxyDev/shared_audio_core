#pragma once

#include "shared_audio/shared_audio_core.h"
#include <memory>
#include <string>

namespace SharedAudio {

    // Crossfade curve types
    enum class CrossfadeCurve {
        LINEAR,
        LOGARITHMIC,
        EQUAL_POWER,
        SINE_COSINE,
        EXPONENTIAL
    };

    // Crossfade status
    struct CrossfadeStatus {
        bool is_active;
        std::string from_cue;
        std::string to_cue;
        double duration_seconds;
        double elapsed_seconds;
        double progress; // 0.0 to 1.0
        CrossfadeCurve curve;
    };

    // Professional crossfade engine
    class CrossfadeEngine {
    public:
        CrossfadeEngine();
        ~CrossfadeEngine();

        // Initialization
        bool initialize(int sample_rate);
        void shutdown();

        // Crossfade operations
        bool start_crossfade(const std::string& from_cue, const std::string& to_cue,
            double duration_seconds, CrossfadeCurve curve = CrossfadeCurve::EQUAL_POWER);
        bool stop_crossfade();
        bool is_crossfading() const;

        // Settings
        void set_default_curve(CrossfadeCurve curve);
        CrossfadeCurve get_default_curve() const;

        // Status
        CrossfadeStatus get_status() const;
        double get_progress() const;

        // Audio processing (called from audio callback)
        void process_audio(AudioBuffer& outputs, int num_samples);

    private:
        class Impl;
        std::unique_ptr<Impl> impl_;
    };

}