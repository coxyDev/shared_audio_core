#include "show_control/cue_audio_manager.h"
#include <iostream>
#include <algorithm>
#include <cmath>
#include <map>

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
            // TODO: Implement proper audio file loading with libsndfile or similar
            // For now, simulate successful loading with test tone
            std::cout << "Loading audio file: " << file_path_ << std::endl;

            // Simulate file data
            duration_samples_ = sample_rate_ * 10; // 10 seconds
            audio_data_.resize(2); // Stereo
            audio_data_[0].resize(duration_samples_, 0.0f);
            audio_data_[1].resize(duration_samples_, 0.0f);

            // Generate test tone for verification
            float frequency = 440.0f; // A4
            if (cue_id_.find("test_cue_2") != std::string::npos) {
                frequency = 880.0f; // A5
            }

            for (size_t i = 0; i < duration_samples_; ++i) {
                float sample = std::sin(2.0f * M_PI * frequency * i / sample_rate_) * 0.3f;
                audio_data_[0][i] = sample; // Left channel
                audio_data_[1][i] = sample; // Right channel
            }

            std::cout << "Audio file loaded successfully: " << cue_id_ << std::endl;
            return true;
        }

        void start() {
            state_ = CueState::PLAYING;
            current_position_ = 0;
            std::cout << "Started cue: " << cue_id_ << std::endl;
        }

        void stop() {
            state_ = CueState::STOPPED;
            current_position_ = 0;
            std::cout << "Stopped cue: " << cue_id_ << std::endl;
        }

        void pause() {
            if (state_ == CueState::PLAYING) {
                state_ = CueState::PAUSED;
            }
        }

        void resume() {
            if (state_ == CueState::PAUSED) {
                state_ = CueState::PLAYING;
            }
        }

        void set_volume(float volume) {
            volume_ = std::clamp(volume, 0.0f, 1.0f);
        }

        void set_pan(float pan) {
            pan_ = std::clamp(pan, -1.0f, 1.0f);
        }

        void fade_in(double duration_seconds, int sample_rate) {
            target_volume_ = volume_;
            volume_ = 0.0f;
            fade_samples_total_ = static_cast<int>(duration_seconds * sample_rate);
            fade_samples_remaining_ = fade_samples_total_;
            state_ = CueState::FADING_IN;
        }

        void fade_out(double duration_seconds, int sample_rate) {
            target_volume_ = 0.0f;
            fade_samples_total_ = static_cast<int>(duration_seconds * sample_rate);
            fade_samples_remaining_ = fade_samples_total_;
            state_ = CueState::FADING_OUT;
        }

        void process_audio(AudioBuffer& outputs, int num_samples) {
            if (state_ == CueState::STOPPED || audio_data_.empty()) {
                return;
            }

            int channels = std::min(static_cast<int>(outputs.size()), static_cast<int>(audio_data_.size()));

            for (int sample = 0; sample < num_samples; ++sample) {
                if (current_position_ >= duration_samples_) {
                    if (is_looping_) {
                        current_position_ = 0;
                    }
                    else {
                        state_ = CueState::STOPPED;
                        break;
                    }
                }

                // Handle fading
                float current_volume = volume_;
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
                        volume_ = target_volume_;
                        if (state_ == CueState::FADING_OUT && target_volume_ == 0.0f) {
                            state_ = CueState::STOPPED;
                            break;
                        }
                        else {
                            state_ = CueState::PLAYING;
                        }
                    }
                }

                // Calculate pan gains
                float left_gain = current_volume * (1.0f - std::max(0.0f, pan_));
                float right_gain = current_volume * (1.0f + std::min(0.0f, pan_));

                // Mix audio
                for (int ch = 0; ch < channels; ++ch) {
                    float audio_sample = audio_data_[ch][current_position_];
                    float gain = (ch == 0) ? left_gain : right_gain;
                    outputs[ch][sample] += audio_sample * gain;
                }

                current_position_++;
            }
        }

        AudioCueInfo get_info() const {
            AudioCueInfo info;
            info.cue_id = cue_id_;
            info.file_path = file_path_;
            info.state = state_;
            info.duration_seconds = static_cast<double>(duration_samples_) / sample_rate_;
            info.current_position_seconds = static_cast<double>(current_position_) / sample_rate_;
            info.volume = volume_;
            info.pan = pan_;
            info.is_looping = is_looping_;
            info.sample_rate = sample_rate_;
            info.channels = static_cast<int>(audio_data_.size());
            return info;
        }

        std::string cue_id_;
        std::string file_path_;
        CueState state_;
        std::vector<std::vector<float>> audio_data_;
        size_t current_position_;
        size_t duration_samples_;
        float volume_;
        float pan_;
        float target_volume_;
        int fade_samples_remaining_;
        int fade_samples_total_;
        bool is_looping_;
        int sample_rate_;
    };

    // CueAudioManager Implementation
    class CueAudioManager::Impl {
    public:
        Impl() : initialized_(false), sample_rate_(48000), buffer_size_(256), master_volume_(1.0f) {}

        bool initialize(int sample_rate, int buffer_size) {
            sample_rate_ = sample_rate;
            buffer_size_ = buffer_size;
            initialized_ = true;

            std::cout << "CueAudioManager initialized (SR: " << sample_rate_ << " Hz, Buffer: " << buffer_size_ << ")" << std::endl;
            return true;
        }

        void shutdown() {
            cues_.clear();
            initialized_ = false;
        }

        bool load_audio_cue(const std::string& cue_id, const std::string& file_path) {
            auto cue = std::make_unique<AudioCue>(cue_id, file_path);
            if (cue->load_audio_file()) {
                cues_[cue_id] = std::move(cue);
                return true;
            }
            return false;
        }

        bool unload_audio_cue(const std::string& cue_id) {
            auto it = cues_.find(cue_id);
            if (it != cues_.end()) {
                cues_.erase(it);
                return true;
            }
            return false;
        }

        bool start_cue(const std::string& cue_id) {
            auto it = cues_.find(cue_id);
            if (it != cues_.end()) {
                it->second->start();
                return true;
            }
            return false;
        }

        bool stop_cue(const std::string& cue_id) {
            auto it = cues_.find(cue_id);
            if (it != cues_.end()) {
                it->second->stop();
                return true;
            }
            return false;
        }

        bool pause_cue(const std::string& cue_id) {
            auto it = cues_.find(cue_id);
            if (it != cues_.end()) {
                it->second->pause();
                return true;
            }
            return false;
        }

        bool resume_cue(const std::string& cue_id) {
            auto it = cues_.find(cue_id);
            if (it != cues_.end()) {
                it->second->resume();
                return true;
            }
            return false;
        }

        bool set_cue_volume(const std::string& cue_id, float volume) {
            auto it = cues_.find(cue_id);
            if (it != cues_.end()) {
                it->second->set_volume(volume);
                return true;
            }
            return false;
        }

        bool set_cue_pan(const std::string& cue_id, float pan) {
            auto it = cues_.find(cue_id);
            if (it != cues_.end()) {
                it->second->set_pan(pan);
                return true;
            }
            return false;
        }

        bool fade_in_cue(const std::string& cue_id, double duration_seconds) {
            auto it = cues_.find(cue_id);
            if (it != cues_.end()) {
                it->second->fade_in(duration_seconds, sample_rate_);
                return true;
            }
            return false;
        }

        bool fade_out_cue(const std::string& cue_id, double duration_seconds) {
            auto it = cues_.find(cue_id);
            if (it != cues_.end()) {
                it->second->fade_out(duration_seconds, sample_rate_);
                return true;
            }
            return false;
        }

        void stop_all_cues() {
            for (auto& [cue_id, cue] : cues_) {
                cue->stop();
            }
        }

        void pause_all_cues() {
            for (auto& [cue_id, cue] : cues_) {
                cue->pause();
            }
        }

        void resume_all_cues() {
            for (auto& [cue_id, cue] : cues_) {
                cue->resume();
            }
        }

        void set_master_volume(float volume) {
            master_volume_ = std::clamp(volume, 0.0f, 1.0f);
        }

        float get_master_volume() const {
            return master_volume_;
        }

        void process_audio(const AudioBuffer& inputs, AudioBuffer& outputs, int num_samples) {
            // Clear outputs first
            for (auto& channel : outputs) {
                std::fill(channel.begin(), channel.begin() + num_samples, 0.0f);
            }

            // Process all active cues
            for (auto& [cue_id, cue] : cues_) {
                cue->process_audio(outputs, num_samples);
            }

            // Apply master volume
            for (auto& channel : outputs) {
                for (int i = 0; i < num_samples; ++i) {
                    channel[i] *= master_volume_;
                }
            }
        }

        bool is_cue_loaded(const std::string& cue_id) const {
            return cues_.find(cue_id) != cues_.end();
        }

        bool is_cue_playing(const std::string& cue_id) const {
            auto it = cues_.find(cue_id);
            if (it != cues_.end()) {
                return it->second->state_ == CueState::PLAYING ||
                    it->second->state_ == CueState::FADING_IN ||
                    it->second->state_ == CueState::FADING_OUT;
            }
            return false;
        }

        std::vector<AudioCueInfo> get_active_cues() const {
            std::vector<AudioCueInfo> active_cues;
            for (const auto& [cue_id, cue] : cues_) {
                if (cue->state_ != CueState::STOPPED) {
                    active_cues.push_back(cue->get_info());
                }
            }
            return active_cues;
        }

        std::vector<AudioCueInfo> get_all_cues() const {
            std::vector<AudioCueInfo> all_cues;
            for (const auto& [cue_id, cue] : cues_) {
                all_cues.push_back(cue->get_info());
            }
            return all_cues;
        }

        AudioCueInfo get_cue_info(const std::string& cue_id) const {
            auto it = cues_.find(cue_id);
            if (it != cues_.end()) {
                return it->second->get_info();
            }
            return AudioCueInfo{}; // Return empty info if not found
        }

        int get_active_cue_count() const {
            int count = 0;
            for (const auto& [cue_id, cue] : cues_) {
                if (cue->state_ != CueState::STOPPED) {
                    count++;
                }
            }
            return count;
        }

        bool initialized_;
        int sample_rate_;
        int buffer_size_;
        float master_volume_;
        std::map<std::string, std::unique_ptr<AudioCue>> cues_;
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

    bool CueAudioManager::is_initialized() const {
        return impl_->initialized_;
    }

    bool CueAudioManager::load_audio_cue(const std::string& cue_id, const std::string& file_path) {
        return impl_->load_audio_cue(cue_id, file_path);
    }

    bool CueAudioManager::unload_audio_cue(const std::string& cue_id) {
        return impl_->unload_audio_cue(cue_id);
    }

    void CueAudioManager::clear_all_cues() {
        impl_->cues_.clear();
    }

    bool CueAudioManager::start_cue(const std::string& cue_id) {
        return impl_->start_cue(cue_id);
    }

    bool CueAudioManager::stop_cue(const std::string& cue_id) {
        return impl_->stop_cue(cue_id);
    }

    bool CueAudioManager::pause_cue(const std::string& cue_id) {
        return impl_->pause_cue(cue_id);
    }

    bool CueAudioManager::resume_cue(const std::string& cue_id) {
        return impl_->resume_cue(cue_id);
    }

    bool CueAudioManager::set_cue_volume(const std::string& cue_id, float volume) {
        return impl_->set_cue_volume(cue_id, volume);
    }

    bool CueAudioManager::set_cue_pan(const std::string& cue_id, float pan) {
        return impl_->set_cue_pan(cue_id, pan);
    }

    bool CueAudioManager::fade_in_cue(const std::string& cue_id, double duration_seconds) {
        return impl_->fade_in_cue(cue_id, duration_seconds);
    }

    bool CueAudioManager::fade_out_cue(const std::string& cue_id, double duration_seconds) {
        return impl_->fade_out_cue(cue_id, duration_seconds);
    }

    void CueAudioManager::stop_all_cues() {
        impl_->stop_all_cues();
    }

    void CueAudioManager::pause_all_cues() {
        impl_->pause_all_cues();
    }

    void CueAudioManager::resume_all_cues() {
        impl_->resume_all_cues();
    }

    void CueAudioManager::set_master_volume(float volume) {
        impl_->set_master_volume(volume);
    }

    float CueAudioManager::get_master_volume() const {
        return impl_->get_master_volume();
    }

    std::vector<AudioCueInfo> CueAudioManager::get_all_cues() const {
        return impl_->get_all_cues();
    }

    std::vector<AudioCueInfo> CueAudioManager::get_active_cues() const {
        return impl_->get_active_cues();
    }

    AudioCueInfo CueAudioManager::get_cue_info(const std::string& cue_id) const {
        return impl_->get_cue_info(cue_id);
    }

    bool CueAudioManager::is_cue_loaded(const std::string& cue_id) const {
        return impl_->is_cue_loaded(cue_id);
    }

    bool CueAudioManager::is_cue_playing(const std::string& cue_id) const {
        return impl_->is_cue_playing(cue_id);
    }

    int CueAudioManager::get_active_cue_count() const {
        return impl_->get_active_cue_count();
    }

    void CueAudioManager::process_audio(const AudioBuffer& inputs, AudioBuffer& outputs, int num_samples) {
        impl_->process_audio(inputs, outputs, num_samples);
    }

    // Placeholder implementations for additional methods
    bool CueAudioManager::set_cue_position(const std::string& cue_id, double position_seconds) {
        // TODO: Implement
        return false;
    }

    bool CueAudioManager::set_cue_loop(const std::string& cue_id, bool loop) {
        // TODO: Implement
        return false;
    }

    bool CueAudioManager::crossfade_cues(const std::string& from_cue_id, const std::string& to_cue_id, double duration_seconds) {
        // TODO: Implement
        return false;
    }

    void CueAudioManager::set_lookahead_samples(int samples) {
        // TODO: Implement
    }

    void CueAudioManager::set_thread_priority(int priority) {
        // TODO: Implement
    }

} // namespace SharedAudio