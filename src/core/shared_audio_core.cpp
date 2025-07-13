#include "shared_audio/shared_audio_core.h"
#include "hardware/hardware_detector.h"
#include "show_control/cue_audio_manager.h"
#include "show_control/crossfade_engine.h"

#include <portaudio.h>
#include <iostream>
#include <chrono>
#include <thread>
#include <algorithm>

namespace SharedAudio {

    // Implementation class (PIMPL pattern)
    class SharedAudioCore::Impl {
    public:
        Impl()
            : initialized_(false)
            , audio_running_(false)
            , stream_(nullptr)
            , current_device_info_(nullptr)
            , cue_manager_(std::make_unique<CueAudioManager>())
            , crossfade_engine_(std::make_unique<CrossfadeEngine>())
            , last_metrics_update_(std::chrono::steady_clock::now())
        {
            // Initialize performance metrics
            current_metrics_.current_latency_ms = 0.0;
            current_metrics_.cpu_usage_percent = 0.0;
            current_metrics_.buffer_underruns = 0;
            current_metrics_.buffer_overruns = 0;
            current_metrics_.is_stable = false;
        }

        ~Impl() {
            shutdown();
        }

        bool initialize(const AudioSettings& settings) {
            if (initialized_) {
                return true;
            }

            // Initialize PortAudio
            PaError err = Pa_Initialize();
            if (err != paNoError) {
                last_error_ = "Failed to initialize PortAudio: " + std::string(Pa_GetErrorText(err));
                return false;
            }

            settings_ = settings;

            // Detect and setup audio device
            if (!setup_audio_device()) {
                Pa_Terminate();
                return false;
            }

            // Initialize components
            cue_manager_->initialize(settings.sample_rate, settings.buffer_size);
            crossfade_engine_->initialize(settings.sample_rate);

            initialized_ = true;
            std::cout << "SharedAudioCore initialized successfully" << std::endl;
            std::cout << "Sample Rate: " << settings_.sample_rate << " Hz" << std::endl;
            std::cout << "Buffer Size: " << settings_.buffer_size << " samples" << std::endl;
            std::cout << "Target Latency: " << settings_.target_latency_ms << " ms" << std::endl;

            return true;
        }

        void shutdown() {
            if (!initialized_) {
                return;
            }

            stop_audio();

            if (stream_) {
                Pa_CloseStream(stream_);
                stream_ = nullptr;
            }

            Pa_Terminate();
            initialized_ = false;
            std::cout << "SharedAudioCore shutdown complete" << std::endl;
        }

        bool setup_audio_device() {
            PaDeviceIndex device_index = Pa_GetDefaultOutputDevice();

            if (device_index == paNoDevice) {
                last_error_ = "No default audio device found";
                return false;
            }

            current_device_info_ = Pa_GetDeviceInfo(device_index);
            if (!current_device_info_) {
                last_error_ = "Failed to get device info";
                return false;
            }

            // Setup stream parameters
            PaStreamParameters output_params;
            output_params.device = device_index;
            output_params.channelCount = settings_.output_channels;
            output_params.sampleFormat = paFloat32;
            output_params.suggestedLatency = current_device_info_->defaultLowOutputLatency;
            output_params.hostApiSpecificStreamInfo = nullptr;

            PaStreamParameters input_params;
            input_params.device = Pa_GetDefaultInputDevice();
            input_params.channelCount = settings_.input_channels;
            input_params.sampleFormat = paFloat32;
            input_params.suggestedLatency = Pa_GetDeviceInfo(input_params.device)->defaultLowInputLatency;
            input_params.hostApiSpecificStreamInfo = nullptr;

            // Create the audio stream
            PaError err = Pa_OpenStream(
                &stream_,
                &input_params,
                &output_params,
                settings_.sample_rate,
                settings_.buffer_size,
                paClipOff,
                &audio_callback_static,
                this
            );

            if (err != paNoError) {
                last_error_ = "Failed to open audio stream: " + std::string(Pa_GetErrorText(err));
                return false;
            }

            return true;
        }

        void start_audio() {
            if (!initialized_ || !stream_ || audio_running_) {
                return;
            }

            PaError err = Pa_StartStream(stream_);
            if (err != paNoError) {
                last_error_ = "Failed to start audio stream: " + std::string(Pa_GetErrorText(err));
                return;
            }

            audio_running_ = true;
            current_metrics_.is_stable = true;
            std::cout << "Audio stream started" << std::endl;
        }

        void stop_audio() {
            if (!audio_running_ || !stream_) {
                return;
            }

            PaError err = Pa_StopStream(stream_);
            if (err != paNoError) {
                last_error_ = "Failed to stop audio stream: " + std::string(Pa_GetErrorText(err));
            }

            audio_running_ = false;
            current_metrics_.is_stable = false;
            std::cout << "Audio stream stopped" << std::endl;
        }

        std::vector<AudioDeviceInfo> get_available_devices() {
            std::vector<AudioDeviceInfo> devices;

            int device_count = Pa_GetDeviceCount();
            if (device_count < 0) {
                return devices;
            }

            for (int i = 0; i < device_count; ++i) {
                const PaDeviceInfo* device_info = Pa_GetDeviceInfo(i);
                if (!device_info) continue;

                AudioDeviceInfo info;
                info.name = device_info->name;
                info.max_input_channels = device_info->maxInputChannels;
                info.max_output_channels = device_info->maxOutputChannels;
                info.is_default_input = (i == Pa_GetDefaultInputDevice());
                info.is_default_output = (i == Pa_GetDefaultOutputDevice());

                // Detect hardware type
                info.hardware_type = detect_hardware_type(info.name);
                info.supports_asio = (info.hardware_type != HardwareType::UNKNOWN);
                info.min_latency_ms = device_info->defaultLowOutputLatency * 1000.0;

                devices.push_back(info);
            }

            return devices;
        }

        PerformanceMetrics get_performance_metrics() const {
            auto now = std::chrono::steady_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_metrics_update_);

            // Update metrics if enough time has passed
            if (duration.count() > 100) { // Update every 100ms
                update_performance_metrics();
                last_metrics_update_ = now;
            }

            return current_metrics_;
        }

        void update_performance_metrics() const {
            if (stream_ && audio_running_) {
                const PaStreamInfo* info = Pa_GetStreamInfo(stream_);
                if (info) {
                    current_metrics_.current_latency_ms = (info->inputLatency + info->outputLatency) * 1000.0;
                }

                // Estimate CPU usage (simplified)
                current_metrics_.cpu_usage_percent = Pa_GetStreamCpuLoad(stream_) * 100.0;

                // Check stability
                current_metrics_.is_stable = (current_metrics_.current_latency_ms < settings_.target_latency_ms * 2.0) &&
                    (current_metrics_.cpu_usage_percent < 50.0);
            }
        }

        // Static callback wrapper
        static int audio_callback_static(
            const void* inputs,
            void* outputs,
            unsigned long frames_per_buffer,
            const PaStreamCallbackTimeInfo* time_info,
            PaStreamCallbackFlags status_flags,
            void* user_data
        ) {
            Impl* impl = static_cast<Impl*>(user_data);
            return impl->audio_callback(inputs, outputs, frames_per_buffer, time_info, status_flags);
        }

        // Main audio callback
        int audio_callback(
            const void* inputs,
            void* outputs,
            unsigned long frames_per_buffer,
            const PaStreamCallbackTimeInfo* time_info,
            PaStreamCallbackFlags status_flags
        ) {
            const float* input_buffer = static_cast<const float*>(inputs);
            float* output_buffer = static_cast<float*>(outputs);

            // Convert to our buffer format
            AudioBuffer input_channels(settings_.input_channels);
            AudioBuffer output_channels(settings_.output_channels);

            for (int ch = 0; ch < settings_.input_channels; ++ch) {
                input_channels[ch].resize(frames_per_buffer);
                if (input_buffer) {
                    for (unsigned long i = 0; i < frames_per_buffer; ++i) {
                        input_channels[ch][i] = input_buffer[i * settings_.input_channels + ch];
                    }
                }
            }

            for (int ch = 0; ch < settings_.output_channels; ++ch) {
                output_channels[ch].resize(frames_per_buffer);
                std::fill(output_channels[ch].begin(), output_channels[ch].end(), 0.0f);
            }

            // Call user callback if set
            if (user_callback_) {
                user_callback_(input_channels, output_channels, frames_per_buffer, settings_.sample_rate);
            }

            // Process through show control systems
            cue_manager_->process_audio(input_channels, output_channels, frames_per_buffer);
            crossfade_engine_->process_audio(output_channels, frames_per_buffer);

            // Convert back to interleaved format
            for (int ch = 0; ch < settings_.output_channels; ++ch) {
                for (unsigned long i = 0; i < frames_per_buffer; ++i) {
                    output_buffer[i * settings_.output_channels + ch] = output_channels[ch][i];
                }
            }

            return paContinue;
        }

        // Member variables
        bool initialized_;
        bool audio_running_;
        PaStream* stream_;
        const PaDeviceInfo* current_device_info_;
        AudioSettings settings_;
        AudioCallback user_callback_;
        std::string last_error_;

        // Components
        std::unique_ptr<CueAudioManager> cue_manager_;
        std::unique_ptr<CrossfadeEngine> crossfade_engine_;

        // Performance tracking
        mutable std::chrono::steady_clock::time_point last_metrics_update_;
        mutable PerformanceMetrics current_metrics_;
    };

    // SharedAudioCore public interface implementation
    SharedAudioCore::SharedAudioCore()
        : impl_(std::make_unique<Impl>())
    {
    }

    SharedAudioCore::~SharedAudioCore() = default;

    bool SharedAudioCore::initialize(const AudioSettings& settings) {
        return impl_->initialize(settings);
    }

    void SharedAudioCore::shutdown() {
        impl_->shutdown();
    }

    bool SharedAudioCore::is_initialized() const {
        return impl_->initialized_;
    }

    void SharedAudioCore::set_audio_callback(AudioCallback callback) {
        impl_->user_callback_ = callback;
    }

    void SharedAudioCore::start_audio() {
        impl_->start_audio();
    }

    void SharedAudioCore::stop_audio() {
        impl_->stop_audio();
    }

    bool SharedAudioCore::is_audio_running() const {
        return impl_->audio_running_;
    }

    std::vector<AudioDeviceInfo> SharedAudioCore::get_available_devices() {
        return ::SharedAudio::get_available_devices(); 
    }

    AudioDeviceInfo SharedAudioCore::get_current_device() const {
        AudioDeviceInfo info;
        if (impl_->current_device_info_) {
            info.name = impl_->current_device_info_->name;
            info.max_input_channels = impl_->current_device_info_->maxInputChannels;
            info.max_output_channels = impl_->current_device_info_->maxOutputChannels;
        }
        return info;
    }

    PerformanceMetrics SharedAudioCore::get_performance_metrics() const {
        return impl_->get_performance_metrics();
    }

    CueAudioManager* SharedAudioCore::get_cue_manager() {
        return impl_->cue_manager_.get();
    }

    CrossfadeEngine* SharedAudioCore::get_crossfade_engine() {
        return impl_->crossfade_engine_.get();
    }

    std::vector<HardwareType> SharedAudioCore::detect_professional_hardware() const {
        return ::SharedAudio::detect_professional_hardware();
    }

    bool SharedAudioCore::is_professional_hardware_available() const {
        auto hardware = detect_professional_hardware();
        return !hardware.empty() && hardware[0] != HardwareType::UNKNOWN;
    }

    std::string SharedAudioCore::get_last_error() const {
        return impl_->last_error_;
    }

    // Utility function implementations
    std::string hardware_type_to_string(HardwareType type) {
        switch (type) {
        case HardwareType::GENERIC_ASIO: return "Generic ASIO";
        case HardwareType::UAD_APOLLO: return "UAD Apollo";
        case HardwareType::ALLEN_HEATH_AVANTIS: return "Allen & Heath Avantis";
        case HardwareType::DIGICO_SD9: return "DiGiCo SD9";
        case HardwareType::YAMAHA_CL5: return "Yamaha CL5";
        case HardwareType::BEHRINGER_X32: return "Behringer X32";
        case HardwareType::RME_FIREFACE: return "RME Fireface";
        case HardwareType::FOCUSRITE_SCARLETT: return "Focusrite Scarlett";
        default: return "Unknown";
        }
    }

    HardwareType detect_hardware_type(const std::string& device_name) {
        std::string lower_name = device_name;
        std::transform(lower_name.begin(), lower_name.end(), lower_name.begin(), ::tolower);

        if (lower_name.find("apollo") != std::string::npos) return HardwareType::UAD_APOLLO;
        if (lower_name.find("avantis") != std::string::npos) return HardwareType::ALLEN_HEATH_AVANTIS;
        if (lower_name.find("digico") != std::string::npos || lower_name.find("sd9") != std::string::npos) return HardwareType::DIGICO_SD9;
        if (lower_name.find("yamaha") != std::string::npos || lower_name.find("cl5") != std::string::npos) return HardwareType::YAMAHA_CL5;
        if (lower_name.find("x32") != std::string::npos || lower_name.find("behringer") != std::string::npos) return HardwareType::BEHRINGER_X32;
        if (lower_name.find("rme") != std::string::npos || lower_name.find("fireface") != std::string::npos) return HardwareType::RME_FIREFACE;
        if (lower_name.find("focusrite") != std::string::npos || lower_name.find("scarlett") != std::string::npos) return HardwareType::FOCUSRITE_SCARLETT;
        if (lower_name.find("asio") != std::string::npos) return HardwareType::GENERIC_ASIO;

        return HardwareType::UNKNOWN;
    }

    bool is_professional_latency_capable(HardwareType type) {
        switch (type) {
        case HardwareType::UAD_APOLLO:
        case HardwareType::ALLEN_HEATH_AVANTIS:
        case HardwareType::DIGICO_SD9:
        case HardwareType::RME_FIREFACE:
            return true;
        default:
            return false;
        }
    }

    // Factory function
    std::unique_ptr<SharedAudioCore> create_audio_core() {
        return std::make_unique<SharedAudioCore>();
    }

} // namespace SharedAudio