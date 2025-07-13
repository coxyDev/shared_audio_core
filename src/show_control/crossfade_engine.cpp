#include "show_control/crossfade_engine.h"
#include <iostream>
#include <cmath>

namespace SharedAudio {

    class CrossfadeEngine::Impl {
    public:
        Impl()
            : sample_rate_(48000)
            , is_crossfading_(false)
            , crossfade_samples_remaining_(0)
            , crossfade_samples_total_(0)
            , default_curve_(CrossfadeCurve::EQUAL_POWER)
            , current_curve_(CrossfadeCurve::EQUAL_POWER)
        {
        }

        bool initialize(int sample_rate) {
            sample_rate_ = sample_rate;
            std::cout << "🔄 CrossfadeEngine initialized (SR: " << sample_rate << " Hz)" << std::endl;
            return true;
        }

        void shutdown() {
            is_crossfading_ = false;
            std::cout << "🔄 CrossfadeEngine shutdown" << std::endl;
        }

        bool start_crossfade(const std::string& from_cue, const std::string& to_cue,
            double duration_seconds, CrossfadeCurve curve) {
            from_cue_ = from_cue;
            to_cue_ = to_cue;
            current_curve_ = curve;
            crossfade_samples_total_ = static_cast<int>(duration_seconds * sample_rate_);
            crossfade_samples_remaining_ = crossfade_samples_total_;
            is_crossfading_ = true;

            std::cout << "🔄 Starting crossfade: " << from_cue << " → " << to_cue
                << " (" << duration_seconds << "s)" << std::endl;
            return true;
        }

        bool stop_crossfade() {
            if (is_crossfading_) {
                is_crossfading_ = false;
                std::cout << "🔄 Crossfade stopped" << std::endl;
                return true;
            }
            return false;
        }

        bool is_crossfading() const {
            return is_crossfading_;
        }

        void process_audio(AudioBuffer& outputs, int num_samples) {
            if (!is_crossfading_) {
                return;
            }

            for (int sample = 0; sample < num_samples; ++sample) {
                if (crossfade_samples_remaining_ <= 0) {
                    is_crossfading_ = false;
                    break;
                }

                float progress = 1.0f - (static_cast<float>(crossfade_samples_remaining_) / crossfade_samples_total_);

                // Apply crossfade curve
                float fade_out_gain = calculate_fade_gain(1.0f - progress, current_curve_);
                float fade_in_gain = calculate_fade_gain(progress, current_curve_);

                // Note: Actual cue volume control would be handled by the CueAudioManager
                // This is just for demonstration
                crossfade_samples_remaining_--;
            }
        }

        CrossfadeStatus get_status() const {
            CrossfadeStatus status;
            status.is_active = is_crossfading_;
            status.from_cue = from_cue_;
            status.to_cue = to_cue_;
            status.duration_seconds = static_cast<double>(crossfade_samples_total_) / sample_rate_;
            status.elapsed_seconds = status.duration_seconds - (static_cast<double>(crossfade_samples_remaining_) / sample_rate_);
            status.progress = is_crossfading_ ? (1.0 - static_cast<double>(crossfade_samples_remaining_) / crossfade_samples_total_) : 0.0;
            status.curve = current_curve_;
            return status;
        }

    private:
        float calculate_fade_gain(float progress, CrossfadeCurve curve) const {
            progress = std::max(0.0f, std::min(1.0f, progress));

            switch (curve) {
            case CrossfadeCurve::LINEAR:
                return progress;

            case CrossfadeCurve::LOGARITHMIC:
                return progress * progress;

            case CrossfadeCurve::EQUAL_POWER:
                return std::sin(progress * M_PI * 0.5f);

            case CrossfadeCurve::SINE_COSINE:
                return 0.5f * (1.0f - std::cos(progress * M_PI));

            case CrossfadeCurve::EXPONENTIAL:
                return std::pow(progress, 3.0f);

            default:
                return progress;
            }
        }

        int sample_rate_;
        bool is_crossfading_;
        int crossfade_samples_remaining_;
        int crossfade_samples_total_;
        CrossfadeCurve default_curve_;
        CrossfadeCurve current_curve_;
        std::string from_cue_;
        std::string to_cue_;
    };

    // CrossfadeEngine public interface
    CrossfadeEngine::CrossfadeEngine() : impl_(std::make_unique<Impl>()) {}
    CrossfadeEngine::~CrossfadeEngine() = default;

    bool CrossfadeEngine::initialize(int sample_rate) {
        return impl_->initialize(sample_rate);
    }

    void CrossfadeEngine::shutdown() {
        impl_->shutdown();
    }

    bool CrossfadeEngine::start_crossfade(const std::string& from_cue, const std::string& to_cue,
        double duration_seconds, CrossfadeCurve curve) {
        return impl_->start_crossfade(from_cue, to_cue, duration_seconds, curve);
    }

    bool CrossfadeEngine::stop_crossfade() {
        return impl_->stop_crossfade();
    }

    bool CrossfadeEngine::is_crossfading() const {
        return impl_->is_crossfading();
    }

    void CrossfadeEngine::process_audio(AudioBuffer& outputs, int num_samples) {
        impl_->process_audio(outputs, num_samples);
    }

    CrossfadeStatus CrossfadeEngine::get_status() const {
        return impl_->get_status();
    }

}