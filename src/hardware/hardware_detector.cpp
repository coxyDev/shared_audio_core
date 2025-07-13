#include <portaudio.h>
#include <string>
#include <algorithm>
#include <iostream>

#ifdef _WIN32
#include <windows.h>
#include <combaseapi.h>
#endif

namespace SharedAudio {

    std::vector<HardwareType> detect_professional_hardware() {
        std::vector<HardwareType> detected;

        std::cout << "Detecting professional audio hardware..." << std::endl;

        // Initialize PortAudio temporarily for device detection
        PaError err = Pa_Initialize();
        if (err != paNoError) {
            std::cout << "Error initializing PortAudio for hardware detection" << std::endl;
            return detected;
        }

        int device_count = Pa_GetDeviceCount();
        if (device_count < 0) {
            std::cout << "Error getting device count" << std::endl;
            Pa_Terminate();
            return detected;
        }

        bool found_professional = false;

        for (int i = 0; i < device_count; ++i) {
            const PaDeviceInfo* device_info = Pa_GetDeviceInfo(i);
            if (!device_info) continue;

            std::string device_name = device_info->name;
            std::string lower_name = device_name;
            std::transform(lower_name.begin(), lower_name.end(), lower_name.begin(), ::tolower);

            HardwareType detected_type = HardwareType::UNKNOWN;

            // Check for specific professional hardware
            if (lower_name.find("apollo") != std::string::npos) {
                detected_type = HardwareType::UAD_APOLLO;
                std::cout << "✅ Found UAD Apollo: " << device_name << std::endl;
                found_professional = true;
            }
            else if (lower_name.find("avantis") != std::string::npos) {
                detected_type = HardwareType::ALLEN_HEATH_AVANTIS;
                std::cout << "✅ Found Allen & Heath Avantis: " << device_name << std::endl;
                found_professional = true;
            }
            else if (lower_name.find("digico") != std::string::npos || lower_name.find("sd9") != std::string::npos) {
                detected_type = HardwareType::DIGICO_SD9;
                std::cout << "✅ Found DiGiCo SD9: " << device_name << std::endl;
                found_professional = true;
            }
            else if (lower_name.find("yamaha") != std::string::npos || lower_name.find("cl5") != std::string::npos) {
                detected_type = HardwareType::YAMAHA_CL5;
                std::cout << "✅ Found Yamaha CL5: " << device_name << std::endl;
                found_professional = true;
            }
            else if (lower_name.find("x32") != std::string::npos || lower_name.find("behringer") != std::string::npos) {
                detected_type = HardwareType::BEHRINGER_X32;
                std::cout << "✅ Found Behringer X32: " << device_name << std::endl;
                found_professional = true;
            }
            else if (lower_name.find("rme") != std::string::npos || lower_name.find("fireface") != std::string::npos) {
                detected_type = HardwareType::RME_FIREFACE;
                std::cout << "✅ Found RME Fireface: " << device_name << std::endl;
                found_professional = true;
            }
            else if (lower_name.find("focusrite") != std::string::npos || lower_name.find("scarlett") != std::string::npos) {
                detected_type = HardwareType::FOCUSRITE_SCARLETT;
                std::cout << "✅ Found Focusrite Scarlett: " << device_name << std::endl;
                found_professional = true;
            }
            else if (lower_name.find("asio") != std::string::npos) {
                detected_type = HardwareType::GENERIC_ASIO;
                std::cout << "✅ Found Generic ASIO: " << device_name << std::endl;
            }

            // Add to detected list if it's professional hardware
            if (detected_type != HardwareType::UNKNOWN) {
                // Avoid duplicates
                if (std::find(detected.begin(), detected.end(), detected_type) == detected.end()) {
                    detected.push_back(detected_type);
                }
            }
        }

        // Windows-specific ASIO detection
#ifdef _WIN32
        detect_windows_asio_hardware(detected);
#endif

        Pa_Terminate();

        if (!found_professional && detected.empty()) {
            std::cout << "⚠️  No professional audio hardware detected" << std::endl;
            std::cout << "   Using system default audio device" << std::endl;
            detected.push_back(HardwareType::UNKNOWN);
        }

        return detected;
    }

#ifdef _WIN32
    void detect_windows_asio_hardware(std::vector<HardwareType>& detected) {
        // Check Windows Registry for ASIO drivers
        HKEY hKey;
        LONG result = RegOpenKeyExA(HKEY_LOCAL_MACHINE,
            "SOFTWARE\\ASIO",
            0,
            KEY_READ,
            &hKey);

        if (result == ERROR_SUCCESS) {
            std::cout << "🔍 Checking Windows Registry for ASIO drivers..." << std::endl;

            DWORD index = 0;
            char subKeyName[256];
            DWORD subKeyLength = sizeof(subKeyName);

            while (RegEnumKeyExA(hKey, index++, subKeyName, &subKeyLength,
                NULL, NULL, NULL, NULL) == ERROR_SUCCESS) {

                std::string driverName = subKeyName;
                std::string lowerName = driverName;
                std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);

                HardwareType type = HardwareType::UNKNOWN;

                if (lowerName.find("apollo") != std::string::npos) {
                    type = HardwareType::UAD_APOLLO;
                }
                else if (lowerName.find("rme") != std::string::npos) {
                    type = HardwareType::RME_FIREFACE;
                }
                else if (lowerName.find("focusrite") != std::string::npos) {
                    type = HardwareType::FOCUSRITE_SCARLETT;
                }
                else {
                    type = HardwareType::GENERIC_ASIO;
                }

                if (type != HardwareType::UNKNOWN &&
                    std::find(detected.begin(), detected.end(), type) == detected.end()) {
                    detected.push_back(type);
                    std::cout << "✅ Found ASIO Driver: " << driverName << std::endl;
                }

                subKeyLength = sizeof(subKeyName);
            }

            RegCloseKey(hKey);
        }
    }
#endif

    bool is_hardware_asio_capable(const std::string& device_name) {
        std::string lower_name = device_name;
        std::transform(lower_name.begin(), lower_name.end(), lower_name.begin(), ::tolower);

        return lower_name.find("asio") != std::string::npos ||
            lower_name.find("apollo") != std::string::npos ||
            lower_name.find("rme") != std::string::npos ||
            lower_name.find("focusrite") != std::string::npos ||
            lower_name.find("avantis") != std::string::npos ||
            lower_name.find("digico") != std::string::npos ||
            lower_name.find("yamaha") != std::string::npos ||
            lower_name.find("behringer") != std::string::npos;
    }

    double get_hardware_minimum_latency(HardwareType type) {
        switch (type) {
        case HardwareType::UAD_APOLLO:
            return 1.8; // ms - Thunderbolt interface, excellent drivers
        case HardwareType::ALLEN_HEATH_AVANTIS:
            return 2.1; // ms - Professional console with good USB interface
        case HardwareType::DIGICO_SD9:
            return 1.9; // ms - Professional console
        case HardwareType::RME_FIREFACE:
            return 2.0; // ms - Excellent drivers, professional interface
        case HardwareType::YAMAHA_CL5:
            return 2.5; // ms - Professional console
        case HardwareType::BEHRINGER_X32:
            return 2.7; // ms - Good value, decent latency
        case HardwareType::FOCUSRITE_SCARLETT:
            return 3.2; // ms - Consumer/prosumer interface
        case HardwareType::GENERIC_ASIO:
            return 5.0; // ms - Generic ASIO, varies widely
        default:
            return 10.0; // ms - System default, higher latency
        }
    }

    // Test hardware capabilities
    struct HardwareCapabilities {
        HardwareType type;
        std::string name;
        double min_latency_ms;
        int max_sample_rate;
        int max_channels;
        bool supports_exclusive_mode;
        bool supports_low_latency;
    };

    HardwareCapabilities get_hardware_capabilities(HardwareType type) {
        HardwareCapabilities caps;
        caps.type = type;
        caps.name = hardware_type_to_string(type);
        caps.min_latency_ms = get_hardware_minimum_latency(type);

        switch (type) {
        case HardwareType::UAD_APOLLO:
            caps.max_sample_rate = 192000;
            caps.max_channels = 32;
            caps.supports_exclusive_mode = true;
            caps.supports_low_latency = true;
            break;
        case HardwareType::ALLEN_HEATH_AVANTIS:
            caps.max_sample_rate = 96000;
            caps.max_channels = 64;
            caps.supports_exclusive_mode = true;
            caps.supports_low_latency = true;
            break;
        case HardwareType::DIGICO_SD9:
            caps.max_sample_rate = 96000;
            caps.max_channels = 128;
            caps.supports_exclusive_mode = true;
            caps.supports_low_latency = true;
            break;
        case HardwareType::RME_FIREFACE:
            caps.max_sample_rate = 192000;
            caps.max_channels = 24;
            caps.supports_exclusive_mode = true;
            caps.supports_low_latency = true;
            break;
        case HardwareType::YAMAHA_CL5:
            caps.max_sample_rate = 96000;
            caps.max_channels = 72;
            caps.supports_exclusive_mode = true;
            caps.supports_low_latency = true;
            break;
        case HardwareType::BEHRINGER_X32:
            caps.max_sample_rate = 48000;
            caps.max_channels = 32;
            caps.supports_exclusive_mode = true;
            caps.supports_low_latency = true;
            break;
        case HardwareType::FOCUSRITE_SCARLETT:
            caps.max_sample_rate = 192000;
            caps.max_channels = 8;
            caps.supports_exclusive_mode = true;
            caps.supports_low_latency = true;
            break;
        default:
            caps.max_sample_rate = 48000;
            caps.max_channels = 2;
            caps.supports_exclusive_mode = false;
            caps.supports_low_latency = false;
            break;
        }

        return caps;
    }

    // Performance optimization based on detected hardware
    AudioSettings optimize_settings_for_hardware(HardwareType type) {
        AudioSettings settings;

        switch (type) {
        case HardwareType::UAD_APOLLO:
        case HardwareType::RME_FIREFACE:
            settings.sample_rate = 96000;  // High quality
            settings.buffer_size = 64;     // Ultra-low latency
            settings.target_latency_ms = 2.0;
            break;

        case HardwareType::ALLEN_HEATH_AVANTIS:
        case HardwareType::DIGICO_SD9:
            settings.sample_rate = 48000;  // Professional standard
            settings.buffer_size = 128;    // Low latency
            settings.target_latency_ms = 3.0;
            break;

        case HardwareType::BEHRINGER_X32:
        case HardwareType::FOCUSRITE_SCARLETT:
            settings.sample_rate = 48000;
            settings.buffer_size = 256;    // Balanced
            settings.target_latency_ms = 5.0;
            break;

        default:
            settings.sample_rate = 48000;
            settings.buffer_size = 512;    // Safe default
            settings.target_latency_ms = 10.0;
            break;
        }

        return settings;
    }

}