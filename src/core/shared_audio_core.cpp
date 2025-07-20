#include "shared_audio/shared_audio_core.h"
#include "hardware/hardware_detector.h"
#include "processing/audio_processor.h"
#include "show_control/cue_audio_manager.h"
#include "show_control/crossfade_engine.h"
#include "core/lock_free_fifo.h"

#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_audio_basics/juce_audio_basics.h>

#include <iostream>
#include <chrono>
#include <thread>

namespace SharedAudio {

    // Implementation class using JUCE for professional audio
    class SharedAudioCore::Impl : public juce::AudioIODeviceCallback {
    public:
        Impl()
            : initialized_(false)
            , audio_running_(false)
            , current_sample_rate_(48000)
            , current_buffer_size_(256)
            , device_manager_(std::make_unique<juce::AudioDeviceManager>())
            , cue_manager_(std::make_unique<CueAudioManager>())
            , crossfade_engine_(std::make_unique<CrossfadeEngine>())
        {
            // Set thread priority for audio callback
#ifdef PLATFORM_WINDOWS
            SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);
#endif
        }

        ~Impl() {
            shutdown();
        }

        bool initialize(const AudioSettings& settings) {
            if (initialized_) {
                return true;
            }

            settings_ = settings;

            // Initialize JUCE audio device manager
            auto result = device_manager_->initialiseWithDefaultDevices(
                settings.input_channels,
                settings.output_channels
            );

            if (!result.isEmpty()) {
                last_error_ = "Failed to initialize audio device: " + result.toStdString();
                return false;
            }

            // Try to select ASIO device if requested and available
            if (settings.enable_asio && !setup_asio_device(settings.device_name)) {
                std::cout << "ASIO not available, using default audio device" << std::endl;
            }

            // Setup audio device
            juce::AudioDeviceManager::AudioDeviceSetup setup;
            device_manager_->getAudioDeviceSetup(setup);

            setup.sampleRate = settings.sample_rate;
            setup.bufferSize = settings.buffer_size;

            result = device_manager_->setAudioDeviceSetup(setup, true);
            if (!result.isEmpty()) {
                last_error_ = "Failed to configure audio device: " + result.toStdString();
                return false;
            }

            // Get actual settings
            device_manager_->getAudioDeviceSetup(setup);
            current_sample_rate_ = setup.sampleRate;
            current_buffer_size_ = setup.bufferSize;

            // Initialize components with actual sample rate
            cue_manager_->initialize(static_cast<int>(current_sample_rate_),
                current_buffer_size_);
            crossfade_engine_->initialize(static_cast<int>(current_sample_rate_));

            // Set this as the audio callback
            device_manager_->addAudioCallback(this);

            initialized_ = true;

            std::cout << "SharedAudioCore initialized successfully" << std::endl;
            std::cout << "Audio Device: " << getCurrentDeviceName() << std::endl;
            std::cout << "Sample Rate: " << current_sample_rate_ << " Hz" << std::endl;
            std::cout << "Buffer Size: " << current_buffer_size_ << " samples" << std::endl;
            std::cout << "Latency: " << getLatencyMs() << " ms" << std::endl;

            return true;
        }

        void shutdown() {
            if (!initialized_) {
                return;
            }

            stop_audio();

            device_manager_->removeAudioCallback(this);
            device_manager_->closeAudioDevice();

            initialized_ = false;

            std::cout << "SharedAudioCore shutdown complete" << std::endl;
        }

        bool setup_asio_device(const std::string& preferred_device) {
#ifdef PLATFORM_WINDOWS
            // Use our header-only ASIO detection
            auto asio_drivers = ASIOHeaderInterface::detectASIODrivers();

            if (asio_drivers.empty()) {
                return false;
            }

            // Find ASIO device type in JUCE
            auto* asio_type = device_manager_->getAvailableDeviceTypes().getFirst();
            while (asio_type != nullptr) {
                if (asio_type->getTypeName() == "ASIO") {
                    device_manager_->setCurrentAudioDeviceType("ASIO", true);

                    // Try to find preferred device
                    if (!preferred_device.empty()) {
                        auto device_names = asio_type->getDeviceNames();
                        for (const auto& name : device_names) {
                            if (name.toStdString().find(preferred_device) != std::string::npos) {
                                juce::AudioDeviceManager::AudioDeviceSetup setup;
                                device_manager_->getAudioDeviceSetup(setup);
                                setup.outputDeviceName = name;
                                setup.inputDeviceName = name;
                                device_manager_->setAudioDeviceSetup(setup, true);
                                return true;
                            }
                        }
                    }

                    return true; // ASIO available, will use default ASIO device
                }
                asio_type = device_manager_->getAvailableDeviceTypes().getNext(asio_type);
            }
#endif
            return false;
        }

        void start_audio() {
            if (!initialized_ || audio_running_) {
                return;
            }

            // Audio is automatically started by JUCE when callback is added
            audio_running_ = true;
            std::cout << "Audio started successfully" << std::endl;
        }

        void stop_audio() {
            if (!audio_running_) {
                return;
            }

            audio_running_ = false;
            std::cout << "Audio stopped" << std::endl;
        }

        // JUCE Audio Callback - REAL-TIME THREAD
        void audioDeviceIOCallback(const float** inputChannelData,
            int numInputChannels,
            float** outputChannelData,
            int numOutputChannels,
            int numSamples) override {
            // Process messages from non-realtime thread
            AudioThreadMessage msg;
            while (message_queue_.pop(msg)) {
                processAudioThreadMessage(msg);
            }

            // Convert to our buffer format
            AudioBuffer input_channels(numInputChannels);
            AudioBuffer output_channels(numOutputChannels);

            for (int ch = 0; ch < numInputChannels; ++ch) {
                input_channels[ch].resize(numSamples);
                if (inputChannelData && inputChannelData[ch]) {
                    std::copy(inputChannelData[ch],
                        inputChannelData[ch] + numSamples,
                        input_channels[ch].begin());
                }
            }

            for (int ch = 0; ch < numOutputChannels; ++ch) {
                output_channels[ch].resize(numSamples);
                std::fill(output_channels[ch].begin(), output_channels[ch].end(), 0.0f);
            }

            // Call user callback if set (NO LOCKS!)
            if (user_callback_) {
                user_callback_(input_channels, output_channels, numSamples, current_sample_rate_);
            }

            // Process through show control systems (lock-free)
            cue_manager_->process_audio(input_channels, output_channels, numSamples);
            crossfade_engine_->process_audio(output_channels, numSamples);

            // Copy back to output
            for (int ch = 0; ch < numOutputChannels; ++ch) {
                if (outputChannelData && outputChannelData[ch]) {
                    std::copy(output_channels[ch].begin(),
                        output_channels[ch].begin() + numSamples,
                        outputChannelData[ch]);
                }
            }

            // Update performance metrics (lock-free)
            updatePerformanceMetrics(numSamples);
        }

        void audioDeviceAboutToStart(juce::AudioIODevice* device) override {
            // Called before audio starts
            current_sample_rate_ = device->getCurrentSampleRate();
            current_buffer_size_ = device->getCurrentBufferSizeSamples();
        }

        void audioDeviceStopped() override {
            // Called when audio stops
        }

        // Process messages in audio thread (lock-free)
        void processAudioThreadMessage(const AudioThreadMessage& msg) {
            switch (msg.type) {
            case AudioThreadMessage::START_CUE:
                cue_manager_->start_cue_realtime(msg.cue_id);
                break;
            case AudioThreadMessage::STOP_CUE:
                cue_manager_->stop_cue_realtime(msg.cue_id);
                break;
            case AudioThreadMessage::SET_VOLUME:
                cue_manager_->set_cue_volume_realtime(msg.cue_id, msg.param1.float_value);
                break;
            case AudioThreadMessage::CROSSFADE:
                crossfade_engine_->start_crossfade_realtime(
                    msg.cue_id,
                    reinterpret_cast<const char*>(&msg.param2), // to_cue_id stored here
                    msg.param1.double_value
                );
                break;
            default:
                break;
            }
        }

        // Send message to audio thread (lock-free)
        bool sendAudioThreadMessage(const AudioThreadMessage& msg) {
            return message_queue_.push(msg);
        }

        void updatePerformanceMetrics(int samples_processed) {
            // Update metrics without locks
            samples_processed_total_.fetch_add(samples_processed, std::memory_order_relaxed);

            // Calculate CPU usage periodically
            auto now = std::chrono::steady_clock::now();
            if (now - last_metrics_update_ > std::chrono::milliseconds(100)) {
                // Update metrics (this is approximate, no locks needed)
                double latency = getLatencyMs();
                current_metrics_.current_latency_ms = latency;
                current_metrics_.is_stable = true;

                last_metrics_update_ = now;
            }
        }

        double getLatencyMs() const {
            if (!device_manager_->getCurrentAudioDevice()) {
                return 0.0;
            }

            int total_latency = device_manager_->getCurrentAudioDevice()->getOutputLatencyInSamples() +
                device_manager_->getCurrentAudioDevice()->getInputLatencyInSamples();

            return (total_latency * 1000.0) / current_sample_rate_;
        }

        std::string getCurrentDeviceName() const {
            auto* device = device_manager_->getCurrentAudioDevice();
            if (device) {
                return device->getName().toStdString();
            }
            return "No Device";
        }

        // Member variables
        bool initialized_;
        bool audio_running_;
        AudioSettings settings_;
        AudioCallback user_callback_;
        std::string last_error_;

        // JUCE components
        std::unique_ptr<juce::AudioDeviceManager> device_manager_;

        // Audio parameters
        double current_sample_rate_;
        int current_buffer_size_;

        // Components
        std::unique_ptr<CueAudioManager> cue_manager_;
        std::unique_ptr<CrossfadeEngine> crossfade_engine_;

        // Lock-free message queue for real-time thread communication
        AudioMessageQueue message_queue_;

        // Performance tracking (lock-free)
        std::atomic<int64_t> samples_processed_total_{ 0 };
        std::chrono::steady_clock::time_point last_metrics_update_;
        PerformanceMetrics current_metrics_;
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

    CueAudioManager* SharedAudioCore::get_cue_manager() {
        return impl_->cue_manager_.get();
    }

    CrossfadeEngine* SharedAudioCore::get_crossfade_engine() {
        return impl_->crossfade_engine_.get();
    }

    PerformanceMetrics SharedAudioCore::get_performance_metrics() const {
        return impl_->current_metrics_;
    }

    std::string SharedAudioCore::get_last_error() const {
        return impl_->last_error_;
    }

    // Hardware detection
    std::vector<HardwareType> SharedAudioCore::detect_professional_hardware() {
#ifdef PLATFORM_WINDOWS
        // Use our header-only ASIO detection
        auto asio_drivers = ASIOHeaderInterface::detectASIODrivers();
        std::vector<HardwareType> detected;

        for (const auto& driver : asio_drivers) {
            if (driver.is_available) {
                HardwareType type = detect_hardware_type(driver.name);
                if (type != HardwareType::UNKNOWN) {
                    detected.push_back(type);
                }
            }
        }

        if (!asio_drivers.empty()) {
            detected.push_back(HardwareType::GENERIC_ASIO);
        }

        return detected;
#else
        return {}; // Non-Windows platforms
#endif
    }

    bool SharedAudioCore::is_professional_hardware_available() const {
        auto hardware = detect_professional_hardware();
        return !hardware.empty();
    }

    // Factory function
    std::unique_ptr<SharedAudioCore> create_audio_core() {
        // Initialize JUCE
        static juce::ScopedJuceInitialiser_GUI juce_initializer;
        return std::make_unique<SharedAudioCore>();
    }

} // namespace SharedAudio