#pragma once

#include "shared_audio/shared_audio_core.h"
#include <vector>
#include <string>

namespace SharedAudio {

    // Hardware capability information
    struct HardwareCapabilities {
        HardwareType type;
        std::string name;
        std::string manufacturer;
        double min_latency_ms;
        double typical_latency_ms;
        int max_sample_rate;
        int max_channels;
        int max_input_channels;
        int max_output_channels;
        int min_buffer_size;
        bool supports_exclusive_mode;
        bool supports_low_latency;
        bool supports_asio;
        bool supports_professional_routing;
        std::vector<int> supported_sample_rates;
        std::vector<int> supported_buffer_sizes;
    };

    // Core hardware detection functions
    std::vector<HardwareType> detect_professional_hardware();
    std::vector<AudioDeviceInfo> get_available_devices();
    HardwareType detect_hardware_type(const std::string& device_name);
    bool is_professional_hardware_available();
    bool is_professional_latency_capable(HardwareType type);
    std::string hardware_type_to_string(HardwareType type);
    HardwareCapabilities get_hardware_capabilities(HardwareType type);

    // Additional utility functions
    bool is_hardware_asio_capable(const std::string& device_name);
    double get_hardware_minimum_latency(HardwareType type);
    AudioSettings optimize_settings_for_hardware(HardwareType type);

    // Platform-specific detection (declarations only)
#ifdef _WIN32
    void detect_windows_asio_hardware(std::vector<HardwareType>& detected);
    std::vector<std::string> get_installed_asio_drivers();
    bool test_asio_driver_compatibility(const std::string& driver_name);
#endif

#ifdef __APPLE__
    void detect_macos_core_audio_hardware(std::vector<HardwareType>& detected);
    std::vector<std::string> get_core_audio_devices();
    bool test_core_audio_exclusive_mode(const std::string& device_name);
#endif

#ifdef __linux__
    void detect_linux_alsa_hardware(std::vector<HardwareType>& detected);
    std::vector<std::string> get_alsa_devices();
    bool test_alsa_low_latency_mode(const std::string& device_name);
#endif

    // Hardware testing and validation
    struct HardwareTestResult {
        HardwareType type;
        std::string device_name;
        bool initialization_success;
        double measured_latency_ms;
        double cpu_usage_percent;
        int buffer_underruns;
        int buffer_overruns;
        bool supports_target_latency;
        std::string error_message;
    };

    // Hardware testing functions (declarations only - implementations can be added later)
    HardwareTestResult test_hardware_performance(HardwareType type, const AudioSettings& settings);
    std::vector<HardwareTestResult> benchmark_all_hardware();
    bool validate_hardware_configuration(const AudioSettings& settings);

} // namespace SharedAudio