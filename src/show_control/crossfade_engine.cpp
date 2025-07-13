class CrossfadeEngine::Impl {
public:
    Impl()
        : initialized_(false)
        , sample_rate_(48000)
        , is_crossfading_(false)
        , crossfade_curve_(0.0f)
        , crossfade_progress_(0.0)
        , crossfade_duration_samples_(0)
        , crossfade_position_(0)
    {
    }

    bool initialize(int sample_rate) {
        sample_rate_ = sample_rate;
        initialized_ = true;

        std::cout << "CrossfadeEngine initialized (SR: " << sample_rate_ << " Hz)" << std::endl;
        return true;
    }

    void process_audio(AudioBuffer& outputs, int num_samples) {
        if (!is_crossfading_) {
            return;
        }

        // Update crossfade progress
        for (int sample = 0; sample < num_samples; ++sample) {
            if (crossfade_position_ >= crossfade_duration_samples_) {
                is_crossfading_ = false;
                crossfade_progress_ = 1.0;
                break;
            }

            crossfade_progress_ = static_cast<double>(crossfade_position_) / crossfade_duration_samples_;
            crossfade_position_++;
        }
    }

    bool start_crossfade(const std::string& from_cue_id, const std::string& to_cue_id, double duration_seconds) {
        crossfade_duration_samples_ = static_cast<int>(duration_seconds * sample_rate_);
        crossfade_position_ = 0;
        crossfade_progress_ = 0.0;
        is_crossfading_ = true;

        std::cout << "Started crossfade: " << from_cue_id << " -> " << to_cue_id << " (" << duration_seconds << "s)" << std::endl;
        return true;
    }

    bool initialized_;
    int sample_rate_;
    bool is_crossfading_;
    float crossfade_curve_;
    double crossfade_progress_;
    int crossfade_duration_samples_;
    int crossfade_position_;
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

bool CueAudioManager::set_cue_volume(const std::string& cue_id, float volume) {
    return impl_->set_cue_volume(cue_id, volume);
}

bool CueAudioManager::fade_in_cue(const std::string& cue_id, double duration_seconds) {
    return impl_->fade_in_cue(cue_id, duration_seconds);
}

void CueAudioManager::process_audio(const AudioBuffer& inputs, AudioBuffer& outputs, int num_samples) {
    impl_->process_audio(inputs, outputs, num_samples);
}

bool CueAudioManager::is_cue_loaded(const std::string& cue_id) const {
    return impl_->is_cue_loaded(cue_id);
}

bool CueAudioManager::is_cue_playing(const std::string& cue_id) const {
    return impl_->is_cue_playing(cue_id);
}

std::vector<AudioCueInfo> CueAudioManager::get_active_cues() const {
    return impl_->get_active_cues();
}

// CrossfadeEngine public interface
CrossfadeEngine::CrossfadeEngine() : impl_(std::make_unique<Impl>()) {}
CrossfadeEngine::~CrossfadeEngine() = default;

bool CrossfadeEngine::initialize(int sample_rate) {
    return impl_->initialize(sample_rate);
}

bool CrossfadeEngine::start_crossfade(const std::string& from_cue_id, const std::string& to_cue_id, double duration_seconds) {
    return impl_->start_crossfade(from_cue_id, to_cue_id, duration_seconds);
}

bool CrossfadeEngine::is_crossfading() const {
    return impl_->is_crossfading_;
}

double CrossfadeEngine::get_crossfade_progress() const {
    return impl_->crossfade_progress_;
}

void CrossfadeEngine::process_audio(AudioBuffer& outputs, int num_samples) {
    impl_->process_audio(outputs, num_samples);
}

}