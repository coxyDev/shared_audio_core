#include "shared_audio/shared_audio_core.h"
#include <iostream>
#include <vector>
#include <chrono>
#include <thread>

using namespace SharedAudio;

void print_separator(const std::string& title) {
    std::cout << "\n==========================================\n";
    std::cout << "  " << title << "\n";
    std::cout << "==========================================\n";
}

int main() {
    std::cout << "🔧 SharedAudioCore Hardware Detection Test\n";
    std::cout << "Testing hardware detection and device enumeration...\n";

    // Test 1: Basic Hardware Detection
    print_separator("PROFESSIONAL HARDWARE DETECTION");

    auto detected_hardware = detect_professional_hardware();

    std::cout << "Detected " << detected_hardware.size() << " professional audio device(s):\n";
    for (auto hardware : detected_hardware) {
        std::cout << "  ✅ " << hardware_type_to_string(hardware) << "\n";

        auto caps = get_hardware_capabilities(hardware);
        std::cout << "     - Max Channels: " << caps.max_channels << "\n";
        std::cout << "     - Min Latency: " << caps.min_latency_ms << "ms\n";
        std::cout << "     - ASIO Support: " << (caps.supports_asio ? "Yes" : "No") << "\n";
        std::cout << "     - Low Latency: " << (caps.supports_low_latency ? "Yes" : "No") << "\n";
    }

    // Test 2: Available Device Enumeration
    print_separator("AVAILABLE AUDIO DEVICES");

    auto devices = get_available_devices();

    std::cout << "Found " << devices.size() << " audio device(s):\n\n";

    for (size_t i = 0; i < devices.size(); ++i) {
        const auto& device = devices[i];

        std::cout << "Device " << i << ": " << device.name << "\n";
        std::cout << "  Driver: " << device.driver_name << "\n";
        std::cout << "  Hardware Type: " << hardware_type_to_string(device.hardware_type) << "\n";
        std::cout << "  Input Channels: " << device.max_input_channels << "\n";
        std::cout << "  Output Channels: " << device.max_output_channels << "\n";
        std::cout << "  ASIO Support: " << (device.supports_asio ? "Yes" : "No") << "\n";
        std::cout << "  Min Latency: " << device.min_latency_ms << "ms\n";
        std::cout << "  Default Input: " << (device.is_default_input ? "Yes" : "No") << "\n";
        std::cout << "  Default Output: " << (device.is_default_output ? "Yes" : "No") << "\n";

        std::cout << "  Supported Sample Rates: ";
        for (size_t j = 0; j < device.supported_sample_rates.size(); ++j) {
            std::cout << device.supported_sample_rates[j];
            if (j < device.supported_sample_rates.size() - 1) std::cout << ", ";
        }
        std::cout << " Hz\n";

        std::cout << "  Supported Buffer Sizes: ";
        for (size_t j = 0; j < device.supported_buffer_sizes.size(); ++j) {
            std::cout << device.supported_buffer_sizes[j];
            if (j < device.supported_buffer_sizes.size() - 1) std::cout << ", ";
        }
        std::cout << " samples\n\n";
    }

    // Test 3: Professional Hardware Capability Check
    print_separator("PROFESSIONAL HARDWARE CAPABILITIES");

    bool has_professional = is_professional_hardware_available();
    std::cout << "Professional hardware available: " << (has_professional ? "✅ Yes" : "❌ No") << "\n";

    if (has_professional) {
        std::cout << "\nProfessional hardware capabilities:\n";
        for (auto hardware : detected_hardware) {
            if (is_professional_latency_capable(hardware)) {
                std::cout << "  ✅ " << hardware_type_to_string(hardware)
                    << " - Professional latency capable\n";
            }
        }
    }
    else {
        std::cout << "  ⚠️  No professional hardware detected\n";
        std::cout << "  ℹ️  Using generic audio interfaces\n";
    }

    // Test 4: Quick Audio Core Test
    print_separator("QUICK AUDIO CORE TEST");

    auto audio_core = create_audio_core();

    AudioSettings settings;
    settings.sample_rate = 48000;
    settings.buffer_size = 256;
    settings.input_channels = 2;
    settings.output_channels = 2;
    settings.enable_asio = has_professional;
    settings.target_latency_ms = has_professional ? 3.0 : 10.0;

    std::cout << "Initializing audio core...\n";
    std::cout << "  Sample Rate: " << settings.sample_rate << " Hz\n";
    std::cout << "  Buffer Size: " << settings.buffer_size << " samples\n";
    std::cout << "  ASIO Enabled: " << (settings.enable_asio ? "Yes" : "No") << "\n";
    std::cout << "  Target Latency: " << settings.target_latency_ms << " ms\n";

    bool initialized = audio_core->initialize(settings);
    std::cout << "  Result: " << (initialized ? "✅ Success" : "❌ Failed") << "\n";

    if (initialized) {
        auto current_device = audio_core->get_current_device();
        std::cout << "  Current Device: " << current_device.name << "\n";

        // Quick performance check
        std::cout << "\nTesting audio stream startup...\n";
        audio_core->start_audio();

        if (audio_core->is_audio_running()) {
            std::cout << "  ✅ Audio stream started successfully\n";

            // Let it run for a second to get performance metrics
            std::this_thread::sleep_for(std::chrono::seconds(1));

            auto metrics = audio_core->get_performance_metrics();
            std::cout << "  Current Latency: " << metrics.current_latency_ms << " ms\n";
            std::cout << "  CPU Usage: " << metrics.cpu_usage_percent << "%\n";
            std::cout << "  Stable: " << (metrics.is_stable ? "Yes" : "No") << "\n";

            audio_core->stop_audio();
            std::cout << "  ✅ Audio stream stopped cleanly\n";
        }
        else {
            std::cout << "  ❌ Failed to start audio stream\n";
        }

        audio_core->shutdown();
        std::cout << "  ✅ Audio core shutdown complete\n";
    }
    else {
        std::cout << "  Error: " << audio_core->get_last_error() << "\n";
    }

    print_separator("HARDWARE TEST COMPLETE");
    std::cout << "✅ Hardware detection test finished successfully!\n";
    std::cout << "📋 Results summary:\n";
    std::cout << "  - Professional hardware: " << (has_professional ? "Available" : "Not available") << "\n";
    std::cout << "  - Total devices found: " << devices.size() << "\n";
    std::cout << "  - Audio core initialization: " << (initialized ? "Success" : "Failed") << "\n";

    return 0;
}