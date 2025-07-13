#pragma once

#include "shared_audio/shared_audio_core.h"
#include <string>
#include <memory>
#include <vector>

namespace SharedAudio {

    // Crossfade curve types
    enum class CrossfadeCurveType {
        LINEAR,           // Linear crossfade
        EXPONENTIAL,      // Exponential curve (more natural sounding)
        LOGARITHMIC,      // Logarithmic curve
        SINE_COSINE,      // Sine/cosine curve (constant power)
        CUSTOM            // User-defined curve
    };

    // Crossfade information
    struct CrossfadeInfo {
        std::string from_cue_id;
        std::string to_cue_id;
        double duration_seconds;
        double current_position_seconds;
        double progress_normalized;  // 0.0 to 1.0
        CrossfadeCurveType curve_type;
        bool is_active;
    };

    // Professional Crossfade Engine
    class CrossfadeEngine {
    public:
        CrossfadeEngine();
        ~CrossfadeEngine();

        // Initialization and shutdown
        bool initialize(int sample_rate);
        void shutdown();
        bool is_initialized() const;

        // Crossfade operations
        bool start_crossfade(const std::string& from_cue_id, const std::string& to_cue_id, double duration_seconds);
        bool stop_crossfade();
        bool pause_crossfade();
        bool resume_crossfade();
        bool is_crossfading() const;

        // Crossfade curve settings
        void set_crossfade_curve(CrossfadeCurveType curve_type);
        void set_crossfade_curve(float curve_parameter); // -1.0 (linear) to 1.0 (exponential)
        void set_custom_curve(const std::vector<float>& curve_points);
        CrossfadeCurveType get_crossfade_curve() const;

        // Crossfade parameters
        void set_default_duration(double duration_seconds);
        double get_default_duration() const;
        void set_auto_start_target_cue(bool auto_start);
        bool get_auto_start_target_cue() const;

        // Information and status
        CrossfadeInfo get_crossfade_info() const;
        double get_crossfade_progress() const;       // 0.0 to 1.0
        double get_remaining_time() const;           // seconds remaining
        double get_elapsed_time() const;             // seconds elapsed

        // Advanced features
        bool queue_crossfade(const std::string& from_cue_id, const std::string& to_cue_id, double duration_seconds);
        void clear_crossfade_queue();
        int get_queued_crossfade_count() const;

        // Audio processing (called from audio callback thread)
        void process_audio(AudioBuffer& outputs, int num_samples);

        // Performance monitoring
        struct CrossfadeMetrics {
            double cpu_usage_percent;
            int samples_processed;
            bool is_realtime_safe;
        };
        CrossfadeMetrics get_performance_metrics() const;

    private:
        class Impl;
        std::unique_ptr<Impl> impl_;
    };

    // Utility functions for crossfade curves
    namespace CrossfadeCurves {
        float linear_curve(float position);
        float exponential_curve(float position, float parameter = 1.0f);
        float logarithmic_curve(float position, float parameter = 1.0f);
        float sine_cosine_curve(float position);
        float custom_curve(float position, const std::vector<float>& curve_points);
    }

} // namespace SharedAudio
private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

}