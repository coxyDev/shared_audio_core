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
        int max_input_channels;
        int max_output_channels;
        bool supports_exclusive_mode;
        bool supports_low_latency;
        bool supports_asio;
        bool supports_professional_routing;
        std::vector<int> supported_sample_rates;
        std::vector<int> supported_buffer_sizes;
    };

    // Hardware detection functions
    std::vector<HardwareType> detect_professional_hardware();
    HardwareCapabilities get_hardware_capabilities(HardwareType type);
    bool is_hardware_asio_capable(const std::string& device_name);
    double get_hardware_minimum_latency(HardwareType type);
    AudioSettings optimize_settings_for_hardware(HardwareType type);

    // Platform-specific detection
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
        int max_stable_channels;
        bool supports_target_latency;
        std::string error_message;
    };

    HardwareTestResult test_hardware_performance(HardwareType type, const AudioSettings& settings);
    std::vector<HardwareTestResult> benchmark_all_hardware();

    // Hardware-specific optimizations
    AudioSettings get_recommended_settings(HardwareType type, bool prioritize_latency = true);
    void apply_hardware_specific_optimizations(HardwareType type);

    // Professional hardware profiles
    namespace HardwareProfiles {
        AudioSettings get_broadcast_profile(HardwareType type);     // For radio/TV broadcast
        AudioSettings get_live_sound_profile(HardwareType type);    // For live performance
        AudioSettings get_recording_profile(HardwareType type);     // For studio recording
        AudioSettings get_post_production_profile(HardwareType type); // For post-production
        AudioSettings get_gaming_profile(HardwareType type);        // For low-latency gaming
    }

} // namespace SharedAudio