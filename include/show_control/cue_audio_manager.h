#pragma once

#include "shared_audio/shared_audio_core.h"
#include <vector>
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

    // Audio cue information structure
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

    // Professional Audio Cue Manager
    class CueAudioManager {
    public:
        CueAudioManager();
        ~CueAudioManager();

        // Initialization and shutdown
        bool initialize(int sample_rate, int buffer_size);
        void shutdown();
        bool is_initialized() const;

        // Cue loading and management
        bool load_audio_cue(const std::string& cue_id, const std::string& file_path);
        bool unload_audio_cue(const std::string& cue_id);
        void clear_all_cues();

        // Basic playback control
        bool start_cue(const std::string& cue_id);
        bool stop_cue(const std::string& cue_id);
        bool pause_cue(const std::string& cue_id);
        bool resume_cue(const std::string& cue_id);

        // Advanced playback control
        bool set_cue_position(const std::string& cue_id, double position_seconds);
        bool set_cue_volume(const std::string& cue_id, float volume);
        bool set_cue_pan(const std::string& cue_id, float pan);
        bool set_cue_loop(const std::string& cue_id, bool loop);

        // Fade operations
        bool fade_in_cue(const std::string& cue_id, double duration_seconds);
        bool fade_out_cue(const std::string& cue_id, double duration_seconds);
        bool crossfade_cues(const std::string& from_cue_id, const std::string& to_cue_id, double duration_seconds);

        // Global operations
        void stop_all_cues();
        void pause_all_cues();
        void resume_all_cues();
        void set_master_volume(float volume);
        float get_master_volume() const;

        // Information and status
        std::vector<AudioCueInfo> get_all_cues() const;
        std::vector<AudioCueInfo> get_active_cues() const;
        AudioCueInfo get_cue_info(const std::string& cue_id) const;
        bool is_cue_loaded(const std::string& cue_id) const;
        bool is_cue_playing(const std::string& cue_id) const;
        int get_active_cue_count() const;

        // Audio processing (called from audio callback thread)
        void process_audio(const AudioBuffer& inputs, AudioBuffer& outputs, int num_samples);

        // Performance optimization
        void set_lookahead_samples(int samples);
        void set_thread_priority(int priority);

    private:
        class Impl;
        std::unique_ptr<Impl> impl_;
    };

} // namespace SharedAudio