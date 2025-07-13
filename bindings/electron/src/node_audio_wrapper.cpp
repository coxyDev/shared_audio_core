#include <napi.h>
#include "shared_audio/shared_audio_core.h"
#include <memory>
#include <map>

using namespace SharedAudio;

// Global audio core instance
static std::unique_ptr<SharedAudioCore> g_audio_core = nullptr;

// Convert C++ HardwareType to JavaScript string
Napi::String HardwareTypeToJS(Napi::Env env, HardwareType type) {
    return Napi::String::New(env, hardware_type_to_string(type));
}

// Convert C++ AudioDeviceInfo to JavaScript object
Napi::Object AudioDeviceInfoToJS(Napi::Env env, const AudioDeviceInfo& info) {
    Napi::Object obj = Napi::Object::New(env);
    obj.Set("name", Napi::String::New(env, info.name));
    obj.Set("driverName", Napi::String::New(env, info.driver_name));
    obj.Set("hardwareType", HardwareTypeToJS(env, info.hardware_type));
    obj.Set("maxInputChannels", Napi::Number::New(env, info.max_input_channels));
    obj.Set("maxOutputChannels", Napi::Number::New(env, info.max_output_channels));
    obj.Set("isDefaultInput", Napi::Boolean::New(env, info.is_default_input));
    obj.Set("isDefaultOutput", Napi::Boolean::New(env, info.is_default_output));
    obj.Set("supportsAsio", Napi::Boolean::New(env, info.supports_asio));
    obj.Set("minLatencyMs", Napi::Number::New(env, info.min_latency_ms));
    return obj;
}

// Convert C++ PerformanceMetrics to JavaScript object
Napi::Object PerformanceMetricsToJS(Napi::Env env, const PerformanceMetrics& metrics) {
    Napi::Object obj = Napi::Object::New(env);
    obj.Set("currentLatencyMs", Napi::Number::New(env, metrics.current_latency_ms));
    obj.Set("cpuUsagePercent", Napi::Number::New(env, metrics.cpu_usage_percent));
    obj.Set("bufferUnderruns", Napi::Number::New(env, metrics.buffer_underruns));
    obj.Set("bufferOverruns", Napi::Number::New(env, metrics.buffer_overruns));
    obj.Set("isStable", Napi::Boolean::New(env, metrics.is_stable));
    return obj;
}

// Convert C++ AudioCueInfo to JavaScript object
Napi::Object AudioCueInfoToJS(Napi::Env env, const AudioCueInfo& info) {
    Napi::Object obj = Napi::Object::New(env);
    obj.Set("cueId", Napi::String::New(env, info.cue_id));
    obj.Set("filePath", Napi::String::New(env, info.file_path));

    std::string state_str;
    switch (info.state) {
    case CueState::STOPPED: state_str = "stopped"; break;
    case CueState::PLAYING: state_str = "playing"; break;
    case CueState::PAUSED: state_str = "paused"; break;
    case CueState::FADING_IN: state_str = "fading_in"; break;
    case CueState::FADING_OUT: state_str = "fading_out"; break;
    case CueState::CROSSFADING: state_str = "crossfading"; break;
    }
    obj.Set("state", Napi::String::New(env, state_str));

    obj.Set("durationSeconds", Napi::Number::New(env, info.duration_seconds));
    obj.Set("currentPositionSeconds", Napi::Number::New(env, info.current_position_seconds));
    obj.Set("volume", Napi::Number::New(env, info.volume));
    obj.Set("pan", Napi::Number::New(env, info.pan));
    obj.Set("isLooping", Napi::Boolean::New(env, info.is_looping));
    obj.Set("sampleRate", Napi::Number::New(env, info.sample_rate));
    obj.Set("channels", Napi::Number::New(env, info.channels));
    return obj;
}

// Initialize the audio core
Napi::Value Initialize(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();

    if (g_audio_core) {
        Napi::TypeError::New(env, "Audio core already initialized").ThrowAsJavaScriptException();
        return env.Undefined();
    }

    AudioSettings settings;

    // Parse optional settings object
    if (info.Length() > 0 && info[0].IsObject()) {
        Napi::Object settingsObj = info[0].As<Napi::Object>();

        if (settingsObj.Has("sampleRate")) {
            settings.sample_rate = settingsObj.Get("sampleRate").As<Napi::Number>().Int32Value();
        }
        if (settingsObj.Has("bufferSize")) {
            settings.buffer_size = settingsObj.Get("bufferSize").As<Napi::Number>().Int32Value();
        }
        if (settingsObj.Has("inputChannels")) {
            settings.input_channels = settingsObj.Get("inputChannels").As<Napi::Number>().Int32Value();
        }
        if (settingsObj.Has("outputChannels")) {
            settings.output_channels = settingsObj.Get("outputChannels").As<Napi::Number>().Int32Value();
        }
        if (settingsObj.Has("targetLatencyMs")) {
            settings.target_latency_ms = settingsObj.Get("targetLatencyMs").As<Napi::Number>().DoubleValue();
        }
    }

    g_audio_core = create_audio_core();

    if (!g_audio_core) {
        Napi::Error::New(env, "Failed to create audio core").ThrowAsJavaScriptException();
        return env.Undefined();
    }

    bool success = g_audio_core->initialize(settings);
    return Napi::Boolean::New(env, success);
}

// Shutdown the audio core
Napi::Value Shutdown(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();

    if (g_audio_core) {
        g_audio_core->shutdown();
        g_audio_core.reset();
    }

    return env.Undefined();
}

// Detect professional hardware
Napi::Value DetectHardware(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();

    if (!g_audio_core) {
        Napi::Error::New(env, "Audio core not initialized").ThrowAsJavaScriptException();
        return env.Undefined();
    }

    auto hardware_types = g_audio_core->detect_professional_hardware();

    Napi::Array array = Napi::Array::New(env, hardware_types.size());
    for (size_t i = 0; i < hardware_types.size(); ++i) {
        array[i] = HardwareTypeToJS(env, hardware_types[i]);
    }

    return array;
}

// Get available devices
Napi::Value GetAvailableDevices(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();

    if (!g_audio_core) {
        Napi::Error::New(env, "Audio core not initialized").ThrowAsJavaScriptException();
        return env.Undefined();
    }

    auto devices = g_audio_core->get_available_devices();

    Napi::Array array = Napi::Array::New(env, devices.size());
    for (size_t i = 0; i < devices.size(); ++i) {
        array[i] = AudioDeviceInfoToJS(env, devices[i]);
    }

    return array;
}

// Start audio
Napi::Value StartAudio(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();

    if (!g_audio_core) {
        Napi::Error::New(env, "Audio core not initialized").ThrowAsJavaScriptException();
        return env.Undefined();
    }

    g_audio_core->start_audio();
    return Napi::Boolean::New(env, g_audio_core->is_audio_running());
}

// Stop audio
Napi::Value StopAudio(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();

    if (!g_audio_core) {
        Napi::Error::New(env, "Audio core not initialized").ThrowAsJavaScriptException();
        return env.Undefined();
    }

    g_audio_core->stop_audio();
    return env.Undefined();
}

// Get performance metrics
Napi::Value GetPerformanceMetrics(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();

    if (!g_audio_core) {
        Napi::Error::New(env, "Audio core not initialized").ThrowAsJavaScriptException();
        return env.Undefined();
    }

    auto metrics = g_audio_core->get_performance_metrics();
    return PerformanceMetricsToJS(env, metrics);
}

// Load audio cue
Napi::Value LoadAudioCue(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();

    if (!g_audio_core) {
        Napi::Error::New(env, "Audio core not initialized").ThrowAsJavaScriptException();
        return env.Undefined();
    }

    if (info.Length() < 2 || !info[0].IsString() || !info[1].IsString()) {
        Napi::TypeError::New(env, "Expected (cueId: string, filePath: string)").ThrowAsJavaScriptException();
        return env.Undefined();
    }

    std::string cue_id = info[0].As<Napi::String>().Utf8Value();
    std::string file_path = info[1].As<Napi::String>().Utf8Value();

    auto* cue_manager = g_audio_core->get_cue_manager();
    bool success = cue_manager->load_audio_cue(cue_id, file_path);

    return Napi::Boolean::New(env, success);
}

// Start cue
Napi::Value StartCue(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();

    if (!g_audio_core) {
        Napi::Error::New(env, "Audio core not initialized").ThrowAsJavaScriptException();
        return env.Undefined();
    }

    if (info.Length() < 1 || !info[0].IsString()) {
        Napi::TypeError::New(env, "Expected (cueId: string)").ThrowAsJavaScriptException();
        return env.Undefined();
    }

    std::string cue_id = info[0].As<Napi::String>().Utf8Value();

    auto* cue_manager = g_audio_core->get_cue_manager();
    bool success = cue_manager->start_cue(cue_id);

    return Napi::Boolean::New(env, success);
}

// Stop cue
Napi::Value StopCue(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();

    if (!g_audio_core) {
        Napi::Error::New(env, "Audio core not initialized").ThrowAsJavaScriptException();
        return env.Undefined();
    }

    if (info.Length() < 1 || !info[0].IsString()) {
        Napi::TypeError::New(env, "Expected (cueId: string)").ThrowAsJavaScriptException();
        return env.Undefined();
    }

    std::string cue_id = info[0].As<Napi::String>().Utf8Value();

    auto* cue_manager = g_audio_core->get_cue_manager();
    bool success = cue_manager->stop_cue(cue_id);

    return Napi::Boolean::New(env, success);
}

// Set cue volume
Napi::Value SetCueVolume(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();

    if (!g_audio_core) {
        Napi::Error::New(env, "Audio core not initialized").ThrowAsJavaScriptException();
        return env.Undefined();
    }

    if (info.Length() < 2 || !info[0].IsString() || !info[1].IsNumber()) {
        Napi::TypeError::New(env, "Expected (cueId: string, volume: number)").ThrowAsJavaScriptException();
        return env.Undefined();
    }

    std::string cue_id = info[0].As<Napi::String>().Utf8Value();
    float volume = info[1].As<Napi::Number>().FloatValue();

    auto* cue_manager = g_audio_core->get_cue_manager();
    bool success = cue_manager->set_cue_volume(cue_id, volume);

    return Napi::Boolean::New(env, success);
}

// Fade in cue
Napi::Value FadeInCue(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();

    if (!g_audio_core) {
        Napi::Error::New(env, "Audio core not initialized").ThrowAsJavaScriptException();
        return env.Undefined();
    }

    if (info.Length() < 2 || !info[0].IsString() || !info[1].IsNumber()) {
        Napi::TypeError::New(env, "Expected (cueId: string, durationSeconds: number)").ThrowAsJavaScriptException();
        return env.Undefined();
    }

    std::string cue_id = info[0].As<Napi::String>().Utf8Value();
    double duration = info[1].As<Napi::Number>().DoubleValue();

    auto* cue_manager = g_audio_core->get_cue_manager();
    bool success = cue_manager->fade_in_cue(cue_id, duration);

    return Napi::Boolean::New(env, success);
}

// Fade out cue
Napi::Value FadeOutCue(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();

    if (!g_audio_core) {
        Napi::Error::New(env, "Audio core not initialized").ThrowAsJavaScriptException();
        return env.Undefined();
    }

    if (info.Length() < 2 || !info[0].IsString() || !info[1].IsNumber()) {
        Napi::TypeError::New(env, "Expected (cueId: string, durationSeconds: number)").ThrowAsJavaScriptException();
        return env.Undefined();
    }

    std::string cue_id = info[0].As<Napi::String>().Utf8Value();
    double duration = info[1].As<Napi::Number>().DoubleValue();

    auto* cue_manager = g_audio_core->get_cue_manager();
    bool success = cue_manager->fade_out_cue(cue_id, duration);

    return Napi::Boolean::New(env, success);
}

// Get active cues
Napi::Value GetActiveCues(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();

    if (!g_audio_core) {
        Napi::Error::New(env, "Audio core not initialized").ThrowAsJavaScriptException();
        return env.Undefined();
    }

    auto* cue_manager = g_audio_core->get_cue_manager();
    auto active_cues = cue_manager->get_active_cues();

    Napi::Array array = Napi::Array::New(env, active_cues.size());
    for (size_t i = 0; i < active_cues.size(); ++i) {
        array[i] = AudioCueInfoToJS(env, active_cues[i]);
    }

    return array;
}

// Start crossfade
Napi::Value StartCrossfade(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();

    if (!g_audio_core) {
        Napi::Error::New(env, "Audio core not initialized").ThrowAsJavaScriptException();
        return env.Undefined();
    }

    if (info.Length() < 3 || !info[0].IsString() || !info[1].IsString() || !info[2].IsNumber()) {
        Napi::TypeError::New(env, "Expected (fromCueId: string, toCueId: string, durationSeconds: number)").ThrowAsJavaScriptException();
        return env.Undefined();
    }

    std::string from_cue_id = info[0].As<Napi::String>().Utf8Value();
    std::string to_cue_id = info[1].As<Napi::String>().Utf8Value();
    double duration = info[2].As<Napi::Number>().DoubleValue();

    auto* crossfade_engine = g_audio_core->get_crossfade_engine();
    bool success = crossfade_engine->start_crossfade(from_cue_id, to_cue_id, duration);

    return Napi::Boolean::New(env, success);
}

// Get crossfade progress
Napi::Value GetCrossfadeProgress(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();

    if (!g_audio_core) {
        Napi::Error::New(env, "Audio core not initialized").ThrowAsJavaScriptException();
        return env.Undefined();
    }

    auto* crossfade_engine = g_audio_core->get_crossfade_engine();
    double progress = crossfade_engine->get_crossfade_progress();

    return Napi::Number::New(env, progress);
}

// Is crossfading
Napi::Value IsCrossfading(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();

    if (!g_audio_core) {
        Napi::Error::New(env, "Audio core not initialized").ThrowAsJavaScriptException();
        return env.Undefined();
    }

    auto* crossfade_engine = g_audio_core->get_crossfade_engine();
    bool is_crossfading = crossfade_engine->is_crossfading();

    return Napi::Boolean::New(env, is_crossfading);
}

// Get last error
Napi::Value GetLastError(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();

    if (!g_audio_core) {
        return Napi::String::New(env, "Audio core not initialized");
    }

    std::string error = g_audio_core->get_last_error();
    return Napi::String::New(env, error);
}

// Module initialization
Napi::Object Init(Napi::Env env, Napi::Object exports) {
    // Core functions
    exports.Set("initialize", Napi::Function::New(env, Initialize));
    exports.Set("shutdown", Napi::Function::New(env, Shutdown));
    exports.Set("detectHardware", Napi::Function::New(env, DetectHardware));
    exports.Set("getAvailableDevices", Napi::Function::New(env, GetAvailableDevices));
    exports.Set("startAudio", Napi::Function::New(env, StartAudio));
    exports.Set("stopAudio", Napi::Function::New(env, StopAudio));
    exports.Set("getPerformanceMetrics", Napi::Function::New(env, GetPerformanceMetrics));
    exports.Set("getLastError", Napi::Function::New(env, GetLastError));

    // Cue management functions
    exports.Set("loadAudioCue", Napi::Function::New(env, LoadAudioCue));
    exports.Set("startCue", Napi::Function::New(env, StartCue));
    exports.Set("stopCue", Napi::Function::New(env, StopCue));
    exports.Set("setCueVolume", Napi::Function::New(env, SetCueVolume));
    exports.Set("fadeInCue", Napi::Function::New(env, FadeInCue));
    exports.Set("fadeOutCue", Napi::Function::New(env, FadeOutCue));
    exports.Set("getActiveCues", Napi::Function::New(env, GetActiveCues));

    // Crossfade functions
    exports.Set("startCrossfade", Napi::Function::New(env, StartCrossfade));
    exports.Set("getCrossfadeProgress", Napi::Function::New(env, GetCrossfadeProgress));
    exports.Set("isCrossfading", Napi::Function::New(env, IsCrossfading));

    return exports;
}

NODE_API_MODULE(shared_audio_node, Init)