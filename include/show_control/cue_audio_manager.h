#pragma once

#include "shared_audio/shared_audio_core.h"
#include <memory>
#include <string>
#include <vector>

namespace SharedAudio {

    // Cue state enum
    enum class CueState {
        STOPPED,
        PLAYING,
        PAUSED,
        FADING_IN,
        FADING_OUT
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
        bool is_loaded;
    };

    // Cue audio manager class
    class CueAudioManager {
    public:
        CueAudioManager();
        ~CueAudioManager();

        // Initialization
        bool initialize(int sample_rate, int buffer_size);
        void shutdown();

        // Cue management
        bool load_audio_cue(const std::string& cue_id, const std::string& file_path);
        bool unload_audio_cue(const std::string& cue_id);

        // Playback control
        bool start_cue(const std::string& cue_id);
        bool stop_cue(const std::string& cue_id);
        bool pause_cue(const std::string& cue_id);
        bool resume_cue(const std::string& cue_id);

        // Cue properties
        bool set_cue_volume(const std::string& cue_id, float volume);
        bool set_cue_pan(const std::string& cue_id, float pan);
        bool set_cue_loop(const std::string& cue_id, bool loop);
        bool seek_cue(const std::string& cue_id, double position_seconds);

        // Fading
        bool fade_in_cue(const std::string& cue_id, double fade_time_seconds);
        bool fade_out_cue(const std::string& cue_id, double fade_time_seconds);
        bool crossfade_cues(const std::string& from_cue, const std::string& to_cue, double fade_time_seconds);

        // Bulk operations
        void stop_all_cues();
        void pause_all_cues();
        void resume_all_cues();

        // Information
        std::vector<AudioCueInfo> get_active_cues() const;
        AudioCueInfo get_cue_info(const std::string& cue_id) const;
        bool is_cue_loaded(const std::string& cue_id) const;
        bool is_cue_playing(const std::string& cue_id) const;

        // Audio processing (called from audio callback)
        void process_audio(const AudioBuffer& inputs, AudioBuffer& outputs, int num_samples);

    private:
        class Impl;
        std::unique_ptr<Impl> impl_;
    };

}