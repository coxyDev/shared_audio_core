#pragma once

#include "shared_audio/shared_audio_core.h"
#include <vector>
#include <map>
#include <string>
#include <memory>

namespace SharedAudio {

    // Cue state enumeration
    enum class CueState {
        STOPPED,
        PLAYING,
        PAUSED,
        FADING_IN,
        FADING_OUT,
        CROSSFADING
    };

    // Audio cue information
    struct AudioCueInfo {
        std::string cue_id;
        std::string file_path;
        CueState state;
        double duration_seconds;
        double current_position_seconds;
        float volume;
        float pan;
        bool is_looping;
        int sample_rate;
        int channels;
    };

class CrossfadeEngine {
public:
    CrossfadeEngine();
    ~CrossfadeEngine();

    // Initialization
    bool initialize(int sample_rate);
    void shutdown();

    // Crossfade operations
    bool start_crossfade(const std::string& from_cue_id, const std::string& to_cue_id, double duration_seconds);
    bool stop_crossfade();
    bool is_crossfading() const;

    // Settings
    void set_crossfade_curve(float curve); // -1.0 (linear) to 1.0 (exponential)
    float get_crossfade_curve() const;

    // Information
    double get_crossfade_progress() const; // 0.0 to 1.0
    double get_remaining_time() const;

    // Audio processing (called from audio callback)
    void process_audio(AudioBuffer& outputs, int num_samples);

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

}