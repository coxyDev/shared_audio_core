#include "shared_audio/shared_audio_core.h"
#include <iostream>
#include <string>
#include <chrono>
#include <thread>

using namespace SharedAudio;

class ManualTestSuite {
public:
    ManualTestSuite() : test_count_(0), passed_tests_(0) {}

    void run_all_tests() {
        std::cout << "🧪 SharedAudioCore Manual Test Suite\n";
        std::cout << "=====================================\n\n";

        test_basic_initialization();
        test_hardware_detection();
        test_device_enumeration();
        test_cue_management();
        test_crossfade_engine();
        test_audio_streaming();
        test_performance_metrics();
        test_error_handling();

        print_final_results();
    }

private:
    void test_basic_initialization() {
        std::cout << "Test 1: Basic Initialization\n";
        std::cout << "----------------------------\n";

        auto audio_core = create_audio_core();
        assert_test("Audio core creation", audio_core != nullptr);

        AudioSettings settings;
        bool initialized = audio_core->initialize(settings);
        assert_test("Audio core initialization", initialized);

        assert_test("Is initialized check", audio_core->is_initialized());

        audio_core->shutdown();
        assert_test("Is shutdown check", !audio_core->is_initialized());

        std::cout << "\n";
    }

    void test_hardware_detection() {
        std::cout << "Test 2: Hardware Detection\n";
        std::cout << "---------------------------\n";

        auto hardware = detect_professional_hardware();
        assert_test("Hardware detection returns results", !hardware.empty());

        bool has_professional = is_professional_hardware_available();
        std::cout << "Professional hardware available: " << (has_professional ? "Yes" : "No") << "\n";

        for (auto hw : hardware) {
            std::cout << "Found: " << hardware_type_to_string(hw) << "\n";
            bool is_pro = is_professional_latency_capable(hw);
            std::cout << "  Professional latency capable: " << (is_pro ? "Yes" : "No") << "\n";
        }

        assert_test("Hardware type string conversion works",
            hardware_type_to_string(HardwareType::UNKNOWN) == "Unknown");

        std::cout << "\n";
    }

    void test_device_enumeration() {
        std::cout << "Test 3: Device Enumeration\n";
        std::cout << "---------------------------\n";

        auto devices = get_available_devices();
        assert_test("Device enumeration returns results", !devices.empty());

        std::cout << "Found " << devices.size() << " audio devices:\n";

        bool has_default_input = false;
        bool has_default_output = false;

        for (const auto& device : devices) {
            std::cout << "  - " << device.name << " (" << device.driver_name << ")\n";
            if (device.is_default_input) has_default_input = true;
            if (device.is_default_output) has_default_output = true;
        }

        assert_test("Has default input device", has_default_input);
        assert_test("Has default output device", has_default_output);

        std::cout << "\n";
    }

    void test_cue_management() {
        std::cout << "Test 4: Cue Management\n";
        std::cout << "-----------------------\n";

        auto audio_core = create_audio_core();
        assert_test("Audio core creation for cue test", audio_core->initialize());

        auto* cue_manager = audio_core->get_cue_manager();
        assert_test("Cue manager retrieval", cue_manager != nullptr);

        // Load test cue
        bool loaded = cue_manager->load_audio_cue("test1", "test_tone.wav");
        assert_test("Cue loading", loaded);

        assert_test("Cue loaded check", cue_manager->is_cue_loaded("test1"));
        assert_test("Non-existent cue check", !cue_manager->is_cue_loaded("nonexistent"));

        // Test playback control
        bool started = cue_manager->start_cue("test1");
        assert_test("Cue start", started);

        bool stopped = cue_manager->stop_cue("test1");
        assert_test("Cue stop", stopped);

        audio_core->shutdown();
        std::cout << "\n";
    }

    void test_crossfade_engine() {
        std::cout << "Test 5: Crossfade Engine\n";
        std::cout << "-------------------------\n";

        auto audio_core = create_audio_core();
        assert_test("Audio core initialization for crossfade", audio_core->initialize());

        auto* crossfade_engine = audio_core->get_crossfade_engine();
        assert_test("Crossfade engine retrieval", crossfade_engine != nullptr);

        auto* cue_manager = audio_core->get_cue_manager();
        cue_manager->load_audio_cue("cue_a", "test1.wav");
        cue_manager->load_audio_cue("cue_b", "test2.wav");

        assert_test("Not crossfading initially", !crossfade_engine->is_crossfading());

        bool crossfade_started = crossfade_engine->start_crossfade("cue_a", "cue_b", 1.0);
        assert_test("Crossfade start", crossfade_started);

        assert_test("Is crossfading check", crossfade_engine->is_crossfading());

        auto status = crossfade_engine->get_status();
        assert_test("Crossfade status valid", status.is_active);
        assert_test("Crossfade from cue correct", status.from_cue == "cue_a");
        assert_test("Crossfade to cue correct", status.to_cue == "cue_b");

        crossfade_engine->stop_crossfade();
        assert_test("Crossfade stop", !crossfade_engine->is_crossfading());

        audio_core->shutdown();
        std::cout << "\n";
    }

    void test_audio_streaming() {
        std::cout << "Test 6: Audio Streaming\n";
        std::cout << "------------------------\n";

        auto audio_core = create_audio_core();
        assert_test("Audio core initialization for streaming", audio_core->initialize());

        assert_test("Not running initially", !audio_core->is_audio_running());

        // Set up a simple callback
        bool callback_called = false;
        audio_core->set_audio_callback([&](const AudioBuffer& inputs, AudioBuffer& outputs, int num_samples, double sample_rate) {
            callback_called = true;
            // Simple passthrough
            for (size_t ch = 0; ch < outputs.size() && ch < inputs.size(); ++ch) {
                for (int i = 0; i < num_samples; ++i) {
                    outputs[ch][i] = inputs[ch][i] * 0.5f;
                }
            }
            });

        audio_core->start_audio();
        assert_test("Audio stream start", audio_core->is_audio_running());

        // Let it run briefly to ensure callback is called
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        assert_test("Audio callback called", callback_called);

        audio_core->stop_audio();
        assert_test("Audio stream stop", !audio_core->is_audio_running());

        audio_core->shutdown();
        std::cout << "\n";
    }

    void test_performance_metrics() {
        std::cout << "Test 7: Performance Metrics\n";
        std::cout << "----------------------------\n";

        auto audio_core = create_audio_core();
        assert_test("Audio core initialization for metrics", audio_core->initialize());

        auto metrics = audio_core->get_performance_metrics();

        assert_test("Latency metric valid", metrics.current_latency_ms >= 0.0);
        assert_test("CPU usage metric valid", metrics.cpu_usage_percent >= 0.0 && metrics.cpu_usage_percent <= 100.0);
        assert_test("Underrun count valid", metrics.buffer_underruns >= 0);
        assert_test("Overrun count valid", metrics.buffer_overruns >= 0);

        std::cout << "Current metrics:\n";
        std::cout << "  Latency: " << metrics.current_latency_ms << " ms\n";
        std::cout << "  CPU Usage: " << metrics.cpu_usage_percent << "%\n";
        std::cout << "  Buffer Underruns: " << metrics.buffer_underruns << "\n";
        std::cout << "  Buffer Overruns: " << metrics.buffer_overruns << "\n";
        std::cout << "  System Stable: " << (metrics.is_stable ? "Yes" : "No") << "\n";

        audio_core->shutdown();
        std::cout << "\n";
    }

    void test_error_handling() {
        std::cout << "Test 8: Error Handling\n";
        std::cout << "-----------------------\n";

        auto audio_core = create_audio_core();

        // Test invalid settings
        AudioSettings bad_settings;
        bad_settings.sample_rate = -1;  // Invalid
        bad_settings.buffer_size = 0;   // Invalid

        bool should_fail = audio_core->initialize(bad_settings);
        // Note: This might not fail depending on implementation
        std::cout << "Invalid settings initialization: " << (should_fail ? "Unexpectedly succeeded" : "Failed as expected") << "\n";

        if (!should_fail) {
            std::string error = audio_core->get_last_error();
            assert_test("Error message available", !error.empty());
            std::cout << "Error message: " << error << "\n";
        }

        // Test operations on uninitialized core
        auto uninitialized_core = create_audio_core();
        assert_test("Uninitialized core not running", !uninitialized_core->is_audio_running());

        auto* null_cue_manager = uninitialized_core->get_cue_manager();
        auto* null_crossfade = uninitialized_core->get_crossfade_engine();

        // These should return valid pointers even if not initialized
        assert_test("Cue manager available on uninitialized core", null_cue_manager != nullptr);
        assert_test("Crossfade engine available on uninitialized core", null_crossfade != nullptr);

        std::cout << "\n";
    }

    void assert_test(const std::string& test_name, bool condition) {
        test_count_++;
        if (condition) {
            passed_tests_++;
            std::cout << "  ✅ " << test_name << "\n";
        }
        else {
            std::cout << "  ❌ " << test_name << "\n";
        }
    }

    void print_final_results() {
        std::cout << "=====================================\n";
        std::cout << "📊 Test Results Summary\n";
        std::cout << "=====================================\n";
        std::cout << "Total Tests: " << test_count_ << "\n";
        std::cout << "Passed: " << passed_tests_ << "\n";
        std::cout << "Failed: " << (test_count_ - passed_tests_) << "\n";
        std::cout << "Success Rate: " << (100.0 * passed_tests_ / test_count_) << "%\n";

        if (passed_tests_ == test_count_) {
            std::cout << "\n🎉 ALL TESTS PASSED! SharedAudioCore is working correctly.\n";
        }
        else {
            std::cout << "\n⚠️  Some tests failed. Check the output above for details.\n";
        }

        std::cout << "=====================================\n";
    }

    int test_count_;
    int passed_tests_;
};

int main() {
    ManualTestSuite test_suite;
    test_suite.run_all_tests();
    return 0;
}