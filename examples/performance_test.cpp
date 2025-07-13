#include "shared_audio/shared_audio_core.h"
#include <iostream>
#include <vector>
#include <chrono>
#include <thread>
#include <iomanip>

using namespace SharedAudio;

void print_separator(const std::string& title) {
    std::cout << "\n==========================================\n";
    std::cout << "  " << title << "\n";
    std::cout << "==========================================\n";
}

void test_buffer_size_performance(int buffer_size) {
    std::cout << "\n🔧 Testing buffer size: " << buffer_size << " samples\n";

    auto audio_core = create_audio_core();

    AudioSettings settings;
    settings.sample_rate = 48000;
    settings.buffer_size = buffer_size;
    settings.input_channels = 2;
    settings.output_channels = 2;
    settings.enable_asio = true;
    settings.target_latency_ms = 5.0;

    if (audio_core->initialize(settings)) {
        std::cout << "  ✅ Initialized successfully\n";

        // Test callback performance
        int callback_count = 0;
        auto start_time = std::chrono::high_resolution_clock::now();

        audio_core->set_audio_callback([&](const AudioBuffer& inputs, AudioBuffer& outputs, int num_samples, double sample_rate) {
            callback_count++;

            // Simple processing test - copy input to output with slight processing
            for (size_t ch = 0; ch < outputs.size() && ch < inputs.size(); ++ch) {
                for (int i = 0; i < num_samples; ++i) {
                    outputs[ch][i] = inputs[ch][i] * 0.7f; // Simple volume reduction
                }
            }
            });

        audio_core->start_audio();

        if (audio_core->is_audio_running()) {
            std::cout << "  ✅ Audio stream started\n";

            // Run for 3 seconds
            std::this_thread::sleep_for(std::chrono::seconds(3));

            auto end_time = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

            auto metrics = audio_core->get_performance_metrics();

            std::cout << "  📊 Performance Results:\n";
            std::cout << "    - Buffer Size: " << buffer_size << " samples\n";
            std::cout << "    - Theoretical Latency: " << (double(buffer_size) / 48000.0 * 1000.0) << " ms\n";
            std::cout << "    - Measured Latency: " << metrics.current_latency_ms << " ms\n";
            std::cout << "    - CPU Usage: " << std::fixed << std::setprecision(1) << metrics.cpu_usage_percent << "%\n";
            std::cout << "    - Callback Count: " << callback_count << "\n";
            std::cout << "    - Callbacks/sec: " << (callback_count * 1000 / duration.count()) << "\n";
            std::cout << "    - Buffer Underruns: " << metrics.buffer_underruns << "\n";
            std::cout << "    - Buffer Overruns: " << metrics.buffer_overruns << "\n";
            std::cout << "    - System Stable: " << (metrics.is_stable ? "✅ Yes" : "❌ No") << "\n";

            audio_core->stop_audio();
            std::cout << "  ✅ Audio stream stopped\n";
        }
        else {
            std::cout << "  ❌ Failed to start audio stream\n";
        }

        audio_core->shutdown();
    }
    else {
        std::cout << "  ❌ Failed to initialize with buffer size " << buffer_size << "\n";
        std::cout << "  Error: " << audio_core->get_last_error() << "\n";
    }
}

void test_cue_performance() {
    print_separator("CUE MANAGER PERFORMANCE TEST");

    auto audio_core = create_audio_core();

    AudioSettings settings;
    settings.sample_rate = 48000;
    settings.buffer_size = 256;

    if (!audio_core->initialize(settings)) {
        std::cout << "❌ Failed to initialize audio core for cue test\n";
        return;
    }

    auto* cue_manager = audio_core->get_cue_manager();

    // Load multiple cues
    std::cout << "📁 Loading test cues...\n";
    for (int i = 1; i <= 5; ++i) {
        std::string cue_id = "test_cue_" + std::to_string(i);
        std::string file_path = "test_tone_" + std::to_string(440 * i) + ".wav";

        bool loaded = cue_manager->load_audio_cue(cue_id, file_path);
        std::cout << "  " << cue_id << ": " << (loaded ? "✅" : "❌") << "\n";
    }

    audio_core->start_audio();

    // Test simultaneous playback
    std::cout << "\n🎵 Testing simultaneous cue playback...\n";

    auto start_time = std::chrono::high_resolution_clock::now();

    // Start all cues
    for (int i = 1; i <= 5; ++i) {
        std::string cue_id = "test_cue_" + std::to_string(i);
        cue_manager->start_cue(cue_id);
        std::this_thread::sleep_for(std::chrono::milliseconds(200)); // Stagger starts
    }

    // Let them play for 3 seconds
    std::this_thread::sleep_for(std::chrono::seconds(3));

    auto metrics = audio_core->get_performance_metrics();
    std::cout << "  📊 Multi-cue Performance:\n";
    std::cout << "    - CPU Usage: " << std::fixed << std::setprecision(1) << metrics.cpu_usage_percent << "%\n";
    std::cout << "    - Latency: " << metrics.current_latency_ms << " ms\n";
    std::cout << "    - System Stable: " << (metrics.is_stable ? "✅ Yes" : "❌ No") << "\n";

    // Stop all cues
    std::cout << "\n⏹️  Stopping all cues...\n";
    for (int i = 1; i <= 5; ++i) {
        std::string cue_id = "test_cue_" + std::to_string(i);
        cue_manager->stop_cue(cue_id);
    }

    audio_core->stop_audio();
    audio_core->shutdown();

    std::cout << "✅ Cue performance test complete\n";
}

void test_crossfade_performance() {
    print_separator("CROSSFADE ENGINE PERFORMANCE TEST");

    auto audio_core = create_audio_core();

    AudioSettings settings;
    settings.sample_rate = 48000;
    settings.buffer_size = 256;

    if (!audio_core->initialize(settings)) {
        std::cout << "❌ Failed to initialize audio core for crossfade test\n";
        return;
    }

    auto* cue_manager = audio_core->get_cue_manager();
    auto* crossfade_engine = audio_core->get_crossfade_engine();

    // Load test cues
    cue_manager->load_audio_cue("cue_a", "test_tone_440.wav");
    cue_manager->load_audio_cue("cue_b", "test_tone_880.wav");

    audio_core->start_audio();

    // Start first cue
    cue_manager->start_cue("cue_a");
    std::this_thread::sleep_for(std::chrono::seconds(1));

    std::cout << "🔄 Starting crossfade test...\n";

    // Test different crossfade durations
    std::vector<double> fade_durations = { 0.5, 1.0, 2.0, 3.0 };

    for (double duration : fade_durations) {
        std::cout << "\n  Testing " << duration << "s crossfade...\n";

        auto start_time = std::chrono::high_resolution_clock::now();

        // Start crossfade
        crossfade_engine->start_crossfade("cue_a", "cue_b", duration);

        // Monitor crossfade progress
        while (crossfade_engine->is_crossfading()) {
            auto status = crossfade_engine->get_status();
            auto metrics = audio_core->get_performance_metrics();

            std::cout << "    Progress: " << std::fixed << std::setprecision(1)
                << (status.progress * 100.0) << "% | CPU: "
                << metrics.cpu_usage_percent << "%\r" << std::flush;

            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

        auto end_time = std::chrono::high_resolution_clock::now();
        auto actual_duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

        std::cout << "\n    ✅ Crossfade completed in " << actual_duration.count() << "ms\n";
        std::cout << "    Expected: " << (duration * 1000) << "ms\n";

        // Swap cues for next test
        std::swap(std::string("cue_a"), std::string("cue_b"));
    }

    audio_core->stop_audio();
    audio_core->shutdown();

    std::cout << "\n✅ Crossfade performance test complete\n";
}

int main() {
    std::cout << "⚡ SharedAudioCore Performance Test Suite\n";
    std::cout << "Testing performance characteristics and benchmarks...\n";

    // Test 1: Buffer Size Performance
    print_separator("BUFFER SIZE PERFORMANCE TEST");

    std::vector<int> buffer_sizes = { 64, 128, 256, 512, 1024 };

    std::cout << "Testing different buffer sizes for latency vs stability...\n";

    for (int buffer_size : buffer_sizes) {
        test_buffer_size_performance(buffer_size);
        std::this_thread::sleep_for(std::chrono::milliseconds(500)); // Brief pause between tests
    }

    // Test 2: Cue Manager Performance
    test_cue_performance();

    // Test 3: Crossfade Engine Performance
    test_crossfade_performance();

    // Test 4: System Stress Test
    print_separator("SYSTEM STRESS TEST");

    std::cout << "🔥 Running system stress test...\n";
    std::cout << "This test pushes the audio system to its limits.\n";

    auto audio_core = create_audio_core();

    AudioSettings stress_settings;
    stress_settings.sample_rate = 96000;  // High sample rate
    stress_settings.buffer_size = 64;     // Low latency
    stress_settings.input_channels = 8;   // Multi-channel
    stress_settings.output_channels = 8;
    stress_settings.enable_asio = true;
    stress_settings.target_latency_ms = 2.0;  // Aggressive latency target

    std::cout << "  Using aggressive settings:\n";
    std::cout << "    - Sample Rate: " << stress_settings.sample_rate << " Hz\n";
    std::cout << "    - Buffer Size: " << stress_settings.buffer_size << " samples\n";
    std::cout << "    - Channels: " << stress_settings.input_channels << " in, " << stress_settings.output_channels << " out\n";
    std::cout << "    - Target Latency: " << stress_settings.target_latency_ms << " ms\n";

    if (audio_core->initialize(stress_settings)) {
        std::cout << "  ✅ Stress test initialization successful\n";

        // Heavy processing callback
        audio_core->set_audio_callback([](const AudioBuffer& inputs, AudioBuffer& outputs, int num_samples, double sample_rate) {
            // Simulate heavy processing
            for (size_t ch = 0; ch < outputs.size(); ++ch) {
                for (int i = 0; i < num_samples; ++i) {
                    // Some CPU-intensive operations
                    float sample = 0.0f;
                    if (ch < inputs.size()) {
                        sample = inputs[ch][i];
                    }

                    // Simulate effects processing
                    sample = sample * 0.8f + (sample * sample * sample) * 0.2f; // Mild distortion
                    outputs[ch][i] = sample;
                }
            }
            });

        audio_core->start_audio();

        if (audio_core->is_audio_running()) {
            std::cout << "  ✅ Stress test audio stream started\n";

            // Run stress test for 5 seconds
            for (int i = 0; i < 50; ++i) {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));

                auto metrics = audio_core->get_performance_metrics();
                std::cout << "  Stress " << (i + 1) << "/50 | CPU: "
                    << std::fixed << std::setprecision(1) << metrics.cpu_usage_percent
                    << "% | Latency: " << metrics.current_latency_ms
                    << "ms | Stable: " << (metrics.is_stable ? "✅" : "❌") << "\r" << std::flush;
            }

            std::cout << "\n";

            auto final_metrics = audio_core->get_performance_metrics();
            std::cout << "  📊 Final Stress Test Results:\n";
            std::cout << "    - Peak CPU Usage: " << final_metrics.cpu_usage_percent << "%\n";
            std::cout << "    - Final Latency: " << final_metrics.current_latency_ms << " ms\n";
            std::cout << "    - Buffer Underruns: " << final_metrics.buffer_underruns << "\n";
            std::cout << "    - Buffer Overruns: " << final_metrics.buffer_overruns << "\n";
            std::cout << "    - System Remained Stable: " << (final_metrics.is_stable ? "✅ Yes" : "❌ No") << "\n";

            audio_core->stop_audio();
            std::cout << "  ✅ Stress test completed\n";
        }
        else {
            std::cout << "  ❌ Failed to start stress test audio stream\n";
        }

        audio_core->shutdown();
    }
    else {
        std::cout << "  ❌ Stress test initialization failed\n";
        std::cout << "  This is normal - the settings were intentionally aggressive\n";
        std::cout << "  Error: " << audio_core->get_last_error() << "\n";
    }

    print_separator("PERFORMANCE TEST COMPLETE");
    std::cout << "✅ All performance tests completed!\n";
    std::cout << "📋 Performance testing finished - check results above for system capabilities.\n";

    return 0;
}