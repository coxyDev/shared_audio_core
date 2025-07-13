#pragma once

#include "shared_audio/shared_audio_core.h"
#include <vector>

namespace SharedAudio {

    // Hardware detection functions (from Syntri)
    std::vector<HardwareType> detect_professional_hardware();
    bool is_hardware_asio_capable(const std::string& device_name);
    double get_hardware_minimum_latency(HardwareType type);
    AudioSettings optimize_settings_for_hardware(HardwareType type);

#ifdef _WIN32
    void detect_windows_asio_hardware(std::vector<HardwareType>& detected);
#endif

}