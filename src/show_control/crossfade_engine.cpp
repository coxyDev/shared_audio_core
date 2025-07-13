#include "show_control/crossfade_engine.h"
#include <iostream>
#include <algorithm>
#include <cmath>
#include <queue>

namespace SharedAudio {

    // CrossfadeEngine Implementation
    class CrossfadeEngine::Impl {
    public:
        Impl()
            : initialized_(false)
            , sample_rate_(48000)
            , is_crossfading_(false)
            , crossfade_curve_type_(CrossfadeCurveType::SINE_COSINE)
            , crossfade_curve_parameter_(0.0f)
            , crossfade_progress_(0.0)
            , crossfade_duration_samples_(0)
            , crossfade_position_(0)
            , default_duration_(3.0)
            , auto_start_target_cue_(true)
        {
        }

        bool initialize(int sample_rate) {
            sample_rate_ = sample_rate;
            initialized_ = true;

            std::cout << "CrossfadeEngine initialized (SR: " << sample_rate_ << " Hz)" << std::endl;
            return true;
        }

        void shutdown() {
            stop_crossfade();
            crossfade_queue_.clear();
            initialized_ = false;
        }

        bool start_crossfade(const std::string& from_cue_id, const std::string& to_cue_id, double duration_seconds) {
            if (is_crossfading_) {
                std::cout << "Warning: Crossfade already in progress, stopping current crossfade" << std::endl;
                stop_crossfade();
            }

            crossfade_info_.from_cue_id = from_cue_id;
            crossfade_info_.to_cue_id = to_cue_id;
            crossfade_info_.duration_seconds = duration_seconds;
            crossfade_info_.current_position_seconds = 0.0;
            crossfade_info_.progress_normalized = 0.0;
            crossfade_info_.curve_type = crossfade_curve_type_;
            crossfade_info_.is_active = true;

            crossfade_duration_samples_ = static_cast<int>(duration_seconds * sample_rate_);
            crossfade_position_ = 0;
            crossfade_progress_ = 0.0;
            is_crossfading_ = true;

            std::cout << "Started crossfade: " << from_cue_id << " -> " << to_cue_id
                << " (" << duration_seconds << "s)" << std::endl;
            return true;
        }

        bool stop_crossfade() {
            if (!is_crossfading_) {
                return false;
            }

            is_crossfading_ = false;
            crossfade_progress_ = 0.0;
            crossfade_info_.is_active = false;

            std::cout << "Stopped crossfade" << std::endl;
            return true;
        }

        bool pause_crossfade() {
            // TODO: Implement pause functionality
            return false;
        }

        bool resume_crossfade() {
            // TODO: Implement resume functionality
            return false;
        }

        bool is_crossfading() const {
            return is_crossfading_;
        }

        void set_crossfade_curve(CrossfadeCurveType curve_type) {
            crossfade_curve_type_ = curve_type;
        }

        void set_crossfade_curve(float curve_parameter) {
            crossfade_curve_parameter_ = std::clamp(curve_parameter, -1.0f, 1.0f);

            // Map parameter to curve type
            if (curve_parameter < -0.5f) {
                crossfade_curve_type_ = CrossfadeCurveType::LOGARITHMIC;
            }
            else if (curve_parameter > 0.5f) {
                crossfade_curve_type_ = CrossfadeCurveType::EXPONENTIAL;
            }
            else {
                crossfade_curve_type_ = CrossfadeCurveType::LINEAR;
            }
        }

        void set_custom_curve(const std::vector<float>& curve_points) {
            custom_curve_points_ = curve_points;
            crossfade_curve_type_ = CrossfadeCurveType::CUSTOM;
        }

        CrossfadeCurveType get_crossfade_curve() const {
            return crossfade_curve_type_;
        }

        void set_default_duration(double duration_seconds) {
            default_duration_ = std::max(0.1, duration_seconds); // Minimum 0.1 seconds
        }

        double get_default_duration() const {
            return default_duration_;
        }

        void set_auto_start_target_cue(bool auto_start) {
            auto_start_target_cue_ = auto_start;
        }

        bool get_auto_start_target_cue() const {
            return auto_start_target_cue_;
        }

        CrossfadeInfo get_crossfade_info() const {
            return crossfade_info_;
        }

        double get_crossfade_progress() const {
            return crossfade_progress_;
        }

        double get_remaining_time() const {
            if (!is_crossfading_) {
                return 0.0;
            }
            return crossfade_info_.duration_seconds - crossfade_info_.current_position_seconds;
        }

        double get_elapsed_time() const {
            return crossfade_info_.current_position_seconds;
        }

        bool queue_crossfade(const std::string& from_cue_id, const std::string& to_cue_id, double duration_seconds) {
            QueuedCrossfade queued;
            queued.from_cue_id = from_cue_id;
            queued.to_cue_id = to_cue_id;
            queued.duration_seconds = duration_seconds;

            crossfade_queue_.push(queued);

            std::cout << "Queued crossfade: " << from_cue_id << " -> " << to_cue_id << std::endl;
            return true;
        }

        void clear_crossfade_queue() {
            std::queue<QueuedCrossfade> empty;
            crossfade_queue_.swap(empty); // Clear the queue
        }

        int get_queued_crossfade_count() const {
            return static_cast<int>(crossfade_queue_.size());
        }

        void process_audio(AudioBuffer& outputs, int num_samples) {
            if (!is_crossfading_) {
                // Check if there's a queued crossfade to start
                if (!crossfade_queue_.empty()) {
                    auto next_crossfade = crossfade_queue_.front();
                    crossfade_queue_.pop();
                    start_crossfade(next_crossfade.from_cue_id, next_crossfade.to_cue_id, next_crossfade.duration_seconds);
                }
                return;
            }

            // Update crossfade progress
            for (int sample = 0; sample < num_samples; ++sample) {
                if (crossfade_position_ >= crossfade_duration_samples_) {
                    is_crossfading_ = false;
                    crossfade_progress_ = 1.0;
                    crossfade_info_.progress_normalized = 1.0;
                    crossfade_info_.is_active = false;

                    std::cout << "Crossfade completed" << std::endl;
                    break;
                }

                crossfade_progress_ = static_cast<double>(crossfade_position_) / crossfade_duration_samples_;
                crossfade_info_.progress_normalized = crossfade_progress_;
                crossfade_info_.current_position_seconds = crossfade_progress_ * crossfade_info_.duration_seconds;

                crossfade_position_++;
            }

            // Update performance metrics
            performance_metrics_.samples_processed += num_samples;
            performance_metrics_.cpu_usage_percent = 1.0; // Simplified
            performance_metrics_.is_realtime_safe = true;
        }

        CrossfadeMetrics get_performance_metrics() const {
            return performance_metrics_;
        }

    private:
        struct QueuedCrossfade {
            std::string from_cue_id;
            std::string to_cue_id;
            double duration_seconds;
        };

        bool initialized_;
        int sample_rate_;
        bool is_crossfading_;
        CrossfadeCurveType crossfade_curve_type_;
        float crossfade_curve_parameter_;
        std::vector<float> custom_curve_points_;
        double crossfade_progress_;
        int crossfade_duration_samples_;
        int crossfade_position_;
        double default_duration_;
        bool auto_start_target_cue_;

        CrossfadeInfo crossfade_info_;
        std::queue<QueuedCrossfade> crossfade_queue_;

        CrossfadeMetrics performance_metrics_;
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

    bool CrossfadeEngine::is_initialized() const {
        return impl_->initialized_;
    }

    bool CrossfadeEngine::start_crossfade(const std::string& from_cue_id, const std::string& to_cue_id, double duration_seconds) {
        return impl_->start_crossfade(from_cue_id, to_cue_id, duration_seconds);
    }

    bool CrossfadeEngine::stop_crossfade() {
        return impl_->stop_crossfade();
    }

    bool CrossfadeEngine::pause_crossfade() {
        return impl_->pause_crossfade();
    }

    bool CrossfadeEngine::resume_crossfade() {
        return impl_->resume_crossfade();
    }

    bool CrossfadeEngine::is_crossfading() const {
        return impl_->is_crossfading();
    }

    void CrossfadeEngine::set_crossfade_curve(CrossfadeCurveType curve_type) {
        impl_->set_crossfade_curve(curve_type);
    }

    void CrossfadeEngine::set_crossfade_curve(float curve_parameter) {
        impl_->set_crossfade_curve(curve_parameter);
    }

    void CrossfadeEngine::set_custom_curve(const std::vector<float>& curve_points) {
        impl_->set_custom_curve(curve_points);
    }

    CrossfadeCurveType CrossfadeEngine::get_crossfade_curve() const {
        return impl_->get_crossfade_curve();
    }

    void CrossfadeEngine::set_default_duration(double duration_seconds) {
        impl_->set_default_duration(duration_seconds);
    }

    double CrossfadeEngine::get_default_duration() const {
        return impl_->get_default_duration();
    }

    void CrossfadeEngine::set_auto_start_target_cue(bool auto_start) {
        impl_->set_auto_start_target_cue(auto_start);
    }

    bool CrossfadeEngine::get_auto_start_target_cue() const {
        return impl_->get_auto_start_target_cue();
    }

    CrossfadeInfo CrossfadeEngine::get_crossfade_info() const {
        return impl_->get_crossfade_info();
    }

    double CrossfadeEngine::get_crossfade_progress() const {
        return impl_->get_crossfade_progress();
    }

    double CrossfadeEngine::get_remaining_time() const {
        return impl_->get_remaining_time();
    }

    double CrossfadeEngine::get_elapsed_time() const {
        return impl_->get_elapsed_time();
    }

    bool CrossfadeEngine::queue_crossfade(const std::string& from_cue_id, const std::string& to_cue_id, double duration_seconds) {
        return impl_->queue_crossfade(from_cue_id, to_cue_id, duration_seconds);
    }

    void CrossfadeEngine::clear_crossfade_queue() {
        impl_->clear_crossfade_queue();
    }

    int CrossfadeEngine::get_queued_crossfade_count() const {
        return impl_->get_queued_crossfade_count();
    }

    void CrossfadeEngine::process_audio(AudioBuffer& outputs, int num_samples) {
        impl_->process_audio(outputs, num_samples);
    }

    CrossfadeEngine::CrossfadeMetrics CrossfadeEngine::get_performance_metrics() const {
        return impl_->get_performance_metrics();
    }

    // Utility functions for crossfade curves
    namespace CrossfadeCurves {
        float linear_curve(float position) {
            return position;
        }

        float exponential_curve(float position, float parameter) {
            if (parameter > 0.0f) {
                return std::pow(position, 1.0f + parameter);
            }
            else {
                return 1.0f - std::pow(1.0f - position, 1.0f - parameter);
            }
        }

        float logarithmic_curve(float position, float parameter) {
            if (position <= 0.0f) return 0.0f;
            if (position >= 1.0f) return 1.0f;

            float curve_factor = 1.0f + parameter;
            return std::log(1.0f + position * curve_factor) / std::log(1.0f + curve_factor);
        }

        float sine_cosine_curve(float position) {
            // Constant power crossfade using sine/cosine
            return std::sin(position * M_PI * 0.5f);
        }

        float custom_curve(float position, const std::vector<float>& curve_points) {
            if (curve_points.empty()) {
                return linear_curve(position);
            }

            // Interpolate between curve points
            float scaled_pos = position * (curve_points.size() - 1);
            int index = static_cast<int>(scaled_pos);
            float fraction = scaled_pos - index;

            if (index >= static_cast<int>(curve_points.size()) - 1) {
                return curve_points.back();
            }

            // Linear interpolation between points
            return curve_points[index] + fraction * (curve_points[index + 1] - curve_points[index]);
        }
    }

} // namespace SharedAudio