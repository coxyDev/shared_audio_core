#include "hardware/hardware_detector.h"
#include <portaudio.h>
#include <string>
#include <algorithm>
#include <iostream>
#include <regex>

namespace SharedAudio {

    std::vector<HardwareType> detect_professional_hardware() {
        std::vector<HardwareType> detected;

        std::cout << "Detecting professional audio hardware..." << std::endl;

        // Initialize PortAudio if not already done
        static bool pa_initialized = false;
        if (!pa_initialized) {
            if (Pa_Initialize() != paNoError) {
                std::cout << "Error initializing PortAudio for hardware detection" << std::endl;
                return detected;
            }
            pa_initialized = true;
        }

        int device_count = Pa_GetDeviceCount();
        if (device_count < 0) {
            std::cout << "Error getting device count" << std::endl;
            return detected;
        }

        for (int i = 0; i < device_count; ++i) {
            const PaDeviceInfo* device_info = Pa_GetDeviceInfo(i);
            if (!device_info) continue;

            std::string device_name = device_info->name;
            HardwareType type = detect_hardware_type(device_name);

            if (type != HardwareType::UNKNOWN) {
                detected.push_back(type);
                std::cout << "Found: " << hardware_type_to_string(type)
                    << " (" << device_name << ")" << std::endl;
            }
        }

        if (detected.empty()) {
            std::cout << "No professional hardware detected - using generic audio" << std::endl;
            detected.push_back(HardwareType::GENERIC_ASIO);
        }

        return detected;
    }

    std::vector<AudioDeviceInfo> get_available_devices() {
        std::vector<AudioDeviceInfo> devices;

        // Initialize PortAudio if needed
        static bool pa_initialized = false;
        if (!pa_initialized) {
            if (Pa_Initialize() != paNoError) {
                return devices;
            }
            pa_initialized = true;
        }

        int device_count = Pa_GetDeviceCount();
        for (int i = 0; i < device_count; ++i) {
            const PaDeviceInfo* pa_info = Pa_GetDeviceInfo(i);
            if (!pa_info) continue;

            AudioDeviceInfo device;
            device.name = pa_info->name;
            device.driver_name = Pa_GetHostApiInfo(pa_info->hostApi)->name;
            device.hardware_type = detect_hardware_type(device.name);
            device.max_input_channels = pa_info->maxInputChannels;
            device.max_output_channels = pa_info->maxOutputChannels;
            device.is_default_input = (i == Pa_GetDefaultInputDevice());
            device.is_default_output = (i == Pa_GetDefaultOutputDevice());
            device.supports_asio = (std::string(device.driver_name).find("ASIO") != std::string::npos);
            device.min_latency_ms = pa_info->defaultLowOutputLatency * 1000.0;

            // Add common sample rates
            device.supported_sample_rates = { 44100, 48000, 88200, 96000, 176400, 192000 };
            device.supported_buffer_sizes = { 64, 128, 256, 512, 1024, 2048 };

            devices.push_back(device);
        }

        return devices;
    }

    HardwareType detect_hardware_type(const std::string& device_name) {
        std::string name_lower = device_name;
        std::transform(name_lower.begin(), name_lower.end(), name_lower.begin(), ::tolower);

        // UAD Apollo series
        if (name_lower.find("apollo") != std::string::npos ||
            name_lower.find("uad") != std::string::npos) {
            return HardwareType::UAD_APOLLO;
        }

        // Allen & Heath Avantis
        if (name_lower.find("avantis") != std::string::npos ||
            name_lower.find("allen") != std::string::npos) {
            return HardwareType::ALLEN_HEATH_AVANTIS;
        }

        // DiGiCo SD9
        if (name_lower.find("digico") != std::string::npos ||
            name_lower.find("sd9") != std::string::npos) {
            return HardwareType::DIGICO_SD9;
        }

        // Yamaha CL5
        if (name_lower.find("yamaha") != std::string::npos ||
            name_lower.find("cl5") != std::string::npos) {
            return HardwareType::YAMAHA_CL5;
        }

        // Behringer X32
        if (name_lower.find("x32") != std::string::npos ||
            name_lower.find("behringer") != std::string::npos) {
            return HardwareType::BEHRINGER_X32;
        }

        // RME Fireface
        if (name_lower.find("fireface") != std::string::npos ||
            name_lower.find("rme") != std::string::npos) {
            return HardwareType::RME_FIREFACE;
        }

        // Focusrite Scarlett
        if (name_lower.find("scarlett") != std::string::npos ||
            name_lower.find("focusrite") != std::string::npos) {
            return HardwareType::FOCUSRITE_SCARLETT;
        }

        // Generic ASIO
        if (name_lower.find("asio") != std::string::npos) {
            return HardwareType::GENERIC_ASIO;
        }

        return HardwareType::UNKNOWN;
    }

    bool is_professional_hardware_available() {
        auto hardware = detect_professional_hardware();
        for (auto type : hardware) {
            if (is_professional_latency_capable(type)) {
                return true;
            }
        }
        return false;
    }

    bool is_professional_latency_capable(HardwareType type) {
        switch (type) {
        case HardwareType::UAD_APOLLO:
        case HardwareType::ALLEN_HEATH_AVANTIS:
        case HardwareType::DIGICO_SD9:
        case HardwareType::YAMAHA_CL5:
        case HardwareType::RME_FIREFACE:
            return true;
        case HardwareType::BEHRINGER_X32:
        case HardwareType::FOCUSRITE_SCARLETT:
        case HardwareType::GENERIC_ASIO:
            return true;  // Can achieve low latency but not as professional
        default:
            return false;
        }
    }

    std::string hardware_type_to_string(HardwareType type) {
        switch (type) {
        case HardwareType::UAD_APOLLO: return "UAD Apollo";
        case HardwareType::ALLEN_HEATH_AVANTIS: return "Allen & Heath Avantis";
        case HardwareType::DIGICO_SD9: return "DiGiCo SD9";
        case HardwareType::YAMAHA_CL5: return "Yamaha CL5";
        case HardwareType::BEHRINGER_X32: return "Behringer X32";
        case HardwareType::RME_FIREFACE: return "RME Fireface";
        case HardwareType::FOCUSRITE_SCARLETT: return "Focusrite Scarlett";
        case HardwareType::GENERIC_ASIO: return "Generic ASIO";
        default: return "Unknown";
        }
    }

    HardwareCapabilities get_hardware_capabilities(HardwareType type) {
        HardwareCapabilities caps;

        switch (type) {
        case HardwareType::UAD_APOLLO:
            caps.supports_asio = true;
            caps.supports_low_latency = true;
            caps.max_channels = 18;
            caps.min_buffer_size = 32;
            caps.supported_sample_rates = { 44100, 48000, 88200, 96000, 176400, 192000 };
            caps.min_latency_ms = 1.5;
            break;

        case HardwareType::ALLEN_HEATH_AVANTIS:
            caps.supports_asio = true;
            caps.supports_low_latency = true;
            caps.max_channels = 64;
            caps.min_buffer_size = 32;
            caps.supported_sample_rates = { 48000, 96000 };
            caps.min_latency_ms = 2.0;
            break;

        case HardwareType::RME_FIREFACE:
            caps.supports_asio = true;
            caps.supports_low_latency = true;
            caps.max_channels = 30;
            caps.min_buffer_size = 32;
            caps.supported_sample_rates = { 44100, 48000, 88200, 96000, 176400, 192000 };
            caps.min_latency_ms = 1.0;
            break;

        default:
            caps.supports_asio = true;
            caps.supports_low_latency = false;
            caps.max_channels = 8;
            caps.min_buffer_size = 128;
            caps.supported_sample_rates = { 44100, 48000, 96000 };
            caps.min_latency_ms = 5.0;
            break;
        }

        return caps;
    }

}