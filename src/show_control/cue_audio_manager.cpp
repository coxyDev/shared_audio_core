#include "show_control/cue_audio_manager.h"
#include <iostream>
#include <algorithm>
#include <cmath>
#include <mutex>
#include <map>

// Fix M_PI for Windows
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace SharedAudio {

    // Audio cue class
    class AudioCue {
    public:
        AudioCue(const std::string& id, const std::string& file_path)
            : cue_id_(id)
            , file_path_(file_path)
            , state_(CueState::STOPPED)
            , current_position_(0)
            , volume_(1.0f)
            , pan_(0.0f)
            , target_volume_(1.0f)
            , fade_samples_remaining_(0)
            , fade_samples_total_(0)
            , is_looping_(false)
            , sample_rate_(48000)
        {
        }

        bool load_audio_file() {
            std::cout << "[LOAD] Loading audio file: " << file_path_ << std::endl;

            // TODO: Implement proper audio file loading with actual file format support
            // For now, generate test content based on filename
            duration_samples_ = sample_rate_ * 10; // 10 seconds
            audio_data_.resize(2); // Stereo
            audio_data_[0].resize(duration_samples_);
            audio_data_[1].resize(duration_samples_);

            // Generate test tone based on filename
            float frequency = 440.0f; // Default A4
            if (file_path_.find("880") != std::string::npos) frequency = 880.0f;
            if (file_path_.find("220") != std::string::npos) frequency = 220.0f;
            if (file_path_.find("background") != std::string::npos) frequency = 110.0f;

            for (size_t i = 0; i < duration_samples_; ++i) {
                float t = static_cast<float>(i) / sample_rate_;
                float sample = 0.3f * std::sin(2.0f * M_PI * frequency * t);

                // Add some envelope to prevent clicks
                if (i < 1000) sample *= static_cast<float>(i) / 1000.0f;
                if (i > duration_samples_ - 1000) {
                    sample *= static_cast<float>(duration_samples_ - i) / 1000.0f;
                }

                audio_data_[0][i] = sample;
                audio_data_[1][i] = sample;
            }

            std::cout << "[PASS] Audio cue loaded: " << cue_id_ << " (" << frequency << "Hz, "
                << duration_samples_ / sample_rate_ << "s)" << std::endl;
            return true;
        }

        void start() {
            state_ = CueState::PLAYING;
            current_position_ = 0;
            std::cout << "[PLAY] Started cue: " << cue_id_ << std::endl;
        }

        void stop() {
            state_ = CueState::STOPPED;
            current_position_ = 0;
            std::cout << "[STOP] Stopped cue: " << cue_id_ << std::endl;
        }

        void pause() {
            if (state_ == CueState::PLAYING) {
                state_ = CueState::PAUSED;
                std::cout << "[PAUSE] Paused cue: " << cue_id_ << std::endl;
            }
        }

        void resume() {
            if (state_ == CueState::PAUSED) {
                state_ = CueState::PLAYING;
                std::cout << "[PLAY] Resumed cue: " << cue_id_ << std::endl;
            }
        }

        void process_audio(AudioBuffer& outputs, int num_samples) {
            if (state_ != CueState::PLAYING && state_ != CueState::FADING_IN && state_ != CueState::FADING_OUT) {
                return;
            }

            if (audio_data_.empty() || current_position_ >= duration_samples_) {
                if (is_looping_) {
                    current_position_ = 0;
                }
                else {
                    stop();
                    return;
                }
            }

            for (int sample = 0; sample < num_samples; ++sample) {
                if (current_position_ >= duration_samples_) {
                    if (is_looping_) {
                        current_position_ = 0;
                    }
                    else {
                        break;
                    }
                }

                float current_volume = volume_;

                // Handle fading
                if (fade_samples_remaining_ > 0) {
                    float fade_progress = 1.0f - (static_cast<float>(fade_samples_remaining_) / fade_samples_total_);
                    if (state_ == CueState::FADING_IN) {
                        current_volume = target_volume_ * fade_progress;
                    }
                    else if (state_ == CueState::FADING_OUT) {
                        current_volume = volume_ * (1.0f - fade_progress);
                    }
                    fade_samples_remaining_--;

                    if (fade_samples_remaining_ == 0) {
                        if (state_ == CueState::FADING_OUT) {
                            stop();
                            break;
                        }
                        else {
                            state_ = CueState::PLAYING;
                            volume_ = target_volume_;
                        }
                    }
                }

                // Mix into output buffer
                if (outputs.size() >= 2 && current_position_ < audio_data_[0].size()) {
                    float left = audio_data_[0][current_position_] * current_volume;
                    float right = audio_data_[1][current_position_] * current_volume;

                    // Apply panning
                    if (pan_ < 0.0f) {
                        right *= (1.0f + pan_);
                    }
                    else if (pan_ > 0.0f) {
                        left *= (1.0f - pan_);
                    }

                    outputs[0][sample] += left;
                    outputs[1][sample] += right;
                }

                current_position_++;
            }
        }

        // Getters and setters
        const std::string& get_id() const { return cue_id_; }
        CueState get_state() const { return state_; }
        double get_duration_seconds() const { return static_cast<double>(duration_samples_) / sample_rate_; }
        double get_position_seconds() const { return static_cast<double>(current_position_) / sample_rate_; }
        float get_volume() const { return volume_; }
        float get_pan() const { return pan_; }
        bool is_looping() const { return is_looping_; }

        void set_volume(float volume) { volume_ = std::max(0.0f, std::min(1.0f, volume)); }
        void set_pan(float pan) { pan_ = std::max(-1.0f, std::min(1.0f, pan)); }
        void set_looping(bool loop) { is_looping_ = loop; }
        void seek(double position_seconds) {
            current_position_ = static_cast<size_t>(position_seconds * sample_rate_);
            current_position_ = std::min(current_position_, duration_samples_);
        }

        void fade_in(double fade_time_seconds) {
            state_ = CueState::FADING_IN;
            target_volume_ = volume_;
            volume_ = 0.0f;
            fade_samples_total_ = fade_samples_remaining_ = static_cast<int>(fade_time_seconds * sample_rate_);
        }

        void fade_out(double fade_time_seconds) {
            state_ = CueState::FADING_OUT;
            fade_samples_total_ = fade_samples_remaining_ = static_cast<int>(fade_time_seconds * sample_rate_);
        }

    private:
        std::string cue_id_;
        std::string file_path_;
        CueState state_;
        size_t current_position_;
        size_t duration_samples_;
        float volume_;
        float pan_;
        float target_volume_;
        int fade_samples_remaining_;
        int fade_samples_total_;
        bool is_looping_;
        int sample_rate_;
        AudioBuffer audio_data_;
    };

    // CueAudioManager implementation
    class CueAudioManager::Impl {
    public:
        Impl() : sample_rate_(48000), buffer_size_(256), initialized_(false) {}

        bool initialize(int sample_rate, int buffer_size) {
            sample_rate_ = sample_rate;
            buffer_size_ = buffer_size;
            initialized_ = true;

            std::cout << "[AUDIO] CueAudioManager initialized (SR: " << sample_rate
                << " Hz, Buffer: " << buffer_size << ")" << std::endl;
            return true;
        }

        void shutdown() {
            std::lock_guard<std::mutex> lock(cues_mutex_);
            audio_cues_.clear();
            initialized_ = false;
            std::cout << "[AUDIO] CueAudioManager shutdown" << std::endl;
        }

        bool load_audio_cue(const std::string& cue_id, const std::string& file_path) {
            std::lock_guard<std::mutex> lock(cues_mutex_);

            auto cue = std::make_unique<AudioCue>(cue_id, file_path);
            if (!cue->load_audio_file()) {
                return false;
            }

            audio_cues_[cue_id] = std::move(cue);
            return true;
        }

        bool unload_audio_cue(const std::string& cue_id) {
            std::lock_guard<std::mutex> lock(cues_mutex_);

            auto it = audio_cues_.find(cue_id);
            if (it != audio_cues_.end()) {
                it->second->stop();
                audio_cues_.erase(it);
                return true;
            }
            return false;
        }

        bool start_cue(const std::string& cue_id) {
            std::lock_guard<std::mutex> lock(cues_mutex_);

            auto it = audio_cues_.find(cue_id);
            if (it != audio_cues_.end()) {
                it->second->start();
                return true;
            }
            return false;
        }

        bool stop_cue(const std::string& cue_id) {
            std::lock_guard<std::mutex> lock(cues_mutex_);

            auto it = audio_cues_.find(cue_id);
            if (it != audio_cues_.end()) {
                it->second->stop();
                return true;
            }
            return false;
        }

        void process_audio(const AudioBuffer& inputs, AudioBuffer& outputs, int num_samples) {
            std::lock_guard<std::mutex> lock(cues_mutex_);

            for (auto& [cue_id, cue] : audio_cues_) {
                cue->process_audio(outputs, num_samples);
            }
        }

        // Additional methods for completeness...
        bool is_cue_loaded(const std::string& cue_id) const {
            std::lock_guard<std::mutex> lock(cues_mutex_);
            return audio_cues_.find(cue_id) != audio_cues_.end();
        }

    private:
        int sample_rate_;
        int buffer_size_;
        bool initialized_;
        mutable std::mutex cues_mutex_;
        std::map<std::string, std::unique_ptr<AudioCue>> audio_cues_;
    };

    // CueAudioManager public interface
    CueAudioManager::CueAudioManager() : impl_(std::make_unique<Impl>()) {}
    CueAudioManager::~CueAudioManager() = default;

    bool CueAudioManager::initialize(int sample_rate, int buffer_size) {
        return impl_->initialize(sample_rate, buffer_size);
    }

    void CueAudioManager::shutdown() {
        impl_->shutdown();
    }

    bool CueAudioManager::load_audio_cue(const std::string& cue_id, const std::string& file_path) {
        return impl_->load_audio_cue(cue_id, file_path);
    }

    bool CueAudioManager::start_cue(const std::string& cue_id) {
        return impl_->start_cue(cue_id);
    }

    bool CueAudioManager::stop_cue(const std::string& cue_id) {
        return impl_->stop_cue(cue_id);
    }

    void CueAudioManager::process_audio(const AudioBuffer& inputs, AudioBuffer& outputs, int num_samples) {
        impl_->process_audio(inputs, outputs, num_samples);
    }

    bool CueAudioManager::is_cue_loaded(const std::string& cue_id) const {
        return impl_->is_cue_loaded(cue_id);
    }

    bool CueAudioManager::start_cue_realtime(const char* cue_id) {
        // This is called from audio thread - NO LOCKS!
        auto* cue = find_cue_lockfree(cue_id);
        if (cue) {
            cue->start_realtime();
            return true;
        }
        return false;
    }

    bool CueAudioManager::stop_cue_realtime(const char* cue_id) {
        // This is called from audio thread - NO LOCKS!
        auto* cue = find_cue_lockfree(cue_id);
        if (cue) {
            cue->stop_realtime();
            return true;
        }
        return false;
    }

    // Non-realtime thread methods send messages
    bool CueAudioManager::start_cue(const std::string& cue_id) {
        AudioThreadMessage msg;
        msg.type = AudioThreadMessage::START_CUE;
        strncpy(msg.cue_id, cue_id.c_str(), sizeof(msg.cue_id) - 1);

        return parent_core_->sendAudioThreadMessage(msg);
    }

    bool CueAudioManager::stop_cue(const std::string& cue_id) {
        AudioThreadMessage msg;
        msg.type = AudioThreadMessage::STOP_CUE;
        strncpy(msg.cue_id, cue_id.c_str(), sizeof(msg.cue_id) - 1);

        return parent_core_->sendAudioThreadMessage(msg);
    }

}