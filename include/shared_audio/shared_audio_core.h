#pragma once

#include <memory>
#include <vector>
#include <string>
#include <functional>

namespace SharedAudio {

    // Forward declarations
    class CueAudioManager;
    class CrossfadeEngine;

    // Audio sample type
    using AudioSample = float;
    using AudioBuffer = std::vector<std::vector<AudioSample>>;

    // Audio callback function type
    using AudioCallback = std::function<void(
        const AudioBuffer& inputs,
        AudioBuffer& outputs,
        int num_samples,
        double sample_rate
        )>;

    // Hardware types (from Syntri)
    enum class HardwareType {
        GENERIC_ASIO,
        UAD_APOLLO,
        ALLEN_HEATH_AVANTIS,
        DIGICO_SD9,
        YAMAHA_CL5,
        BEHRINGER_X32,
        RME_FIREFACE,
        FOCUSRITE_SCARLETT,
        UNKNOWN
    };

    // Audio device info
    struct AudioDeviceInfo {
        std::string name;
        std::string driver_name;
        HardwareType hardware_type;
        int max_input_channels;
        int max_output_channels;
        std::vector<int> supported_sample_rates;
        std::vector<int> supported_buffer_sizes;
        bool is_default_input;
        bool is_default_output;
        bool supports_asio;
        double min_latency_ms;
    };

    // Audio settings
    struct AudioSettings {
        std::string device_name;
        int sample_rate = 48000;
        int buffer_size = 256;
        int input_channels = 2;
        int output_channels = 2;
        bool enable_asio = true;
        double target_latency_ms = 5.0;
    };

    // Performance metrics
    struct PerformanceMetrics {
        double current_latency_ms;
        double cpu_usage_percent;
        int buffer_underruns;
        int buffer_overruns;
        bool is_stable;
    };

    // Forward declaration of HardwareCapabilities (defined in hardware_detector.h)
    struct HardwareCapabilities;

    // Main shared audio core class
    class SharedAudioCore {
    public:
        SharedAudioCore();
        ~SharedAudioCore();

        // Initialization
        bool initialize(const AudioSettings& settings = AudioSettings{});
        void shutdown();
        bool is_initialized() const;

        // Device management
        std::vector<AudioDeviceInfo> get_available_devices();
        bool set_audio_device(const std::string& device_name);
        AudioDeviceInfo get_current_device() const;

        // Audio processing
        void set_audio_callback(AudioCallback callback);
        void start_audio();
        void stop_audio();
        bool is_audio_running() const;

        // Performance monitoring
        PerformanceMetrics get_performance_metrics() const;

        // Show control features (for CueForge)
        CueAudioManager* get_cue_manager();
        CrossfadeEngine* get_crossfade_engine();

        // Hardware detection (from Syntri)
        std::vector<HardwareType> detect_professional_hardware() const;
        bool is_professional_hardware_available() const;
        HardwareCapabilities get_hardware_capabilities(HardwareType type) const;

        // Error handling
        std::string get_last_error() const;

    private:
        class Impl;
        std::unique_ptr<Impl> impl_;
    };

    // Utility functions
    std::string hardware_type_to_string(HardwareType type);
    HardwareType detect_hardware_type(const std::string& device_name);
    bool is_professional_latency_capable(HardwareType type);

    // Factory functions
    std::unique_ptr<SharedAudioCore> create_audio_core();

} // namespace SharedAudio