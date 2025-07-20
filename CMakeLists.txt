cmake_minimum_required(VERSION 3.15)
project(SharedAudioCore VERSION 1.0.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Build configuration
set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type")
option(BUILD_TESTS "Build tests" ON)
option(BUILD_EXAMPLES "Build examples" ON)
option(BUILD_ELECTRON_BINDING "Build Electron/Node.js binding" ON)

# Platform detection
if(WIN32)
    set(PLATFORM_WINDOWS TRUE)
    add_definitions(-DPLATFORM_WINDOWS=1)
    # Enable Windows-specific optimizations
    add_definitions(-D_WIN32_WINNT=0x0A00) # Windows 10
    add_definitions(-DNOMINMAX) # Prevent Windows.h min/max macros
elseif(APPLE)
    set(PLATFORM_MACOS TRUE)
    add_definitions(-DPLATFORM_MACOS=1)
elseif(UNIX)
    set(PLATFORM_LINUX TRUE)
    add_definitions(-DPLATFORM_LINUX=1)
endif()

# Find vcpkg if available
if(DEFINED ENV{VCPKG_ROOT})
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE STRING "")
endif()

# Find JUCE (REQUIRED for professional audio)
find_package(JUCE CONFIG REQUIRED)
if(NOT JUCE_FOUND)
    message(FATAL_ERROR "JUCE is required for SharedAudioCore. Please install JUCE or set JUCE_DIR")
endif()

# Set JUCE modules we need
set(JUCE_MODULES_ONLY ON)

# Find PortAudio as fallback
find_package(PkgConfig)
if(PkgConfig_FOUND)
    pkg_check_modules(PORTAUDIO portaudio-2.0)
endif()

# Include directories
include_directories(include)

# Source files
set(CORE_SOURCES
    src/core/shared_audio_core.cpp
    src/core/audio_device_manager.cpp
    src/core/audio_buffer.cpp
    src/core/audio_callback.cpp
    src/core/lock_free_fifo.cpp
    src/hardware/hardware_detector.cpp
    src/hardware/asio_header_interface.cpp
    src/processing/audio_processor.cpp
    src/processing/multi_channel_mixer.cpp
    src/processing/audio_file_loader.cpp
    src/processing/effects_processor.cpp
    src/show_control/cue_audio_manager.cpp
    src/show_control/crossfade_engine.cpp
)

# Platform-specific sources
if(PLATFORM_WINDOWS)
    list(APPEND CORE_SOURCES
        src/platform/windows/windows_audio_driver.cpp
        src/platform/windows/asio_registry_scanner.cpp
        src/platform/windows/wasapi_interface.cpp
    )
elseif(PLATFORM_MACOS)
    list(APPEND CORE_SOURCES
        src/platform/macos/core_audio_interface.cpp
    )
elseif(PLATFORM_LINUX)
    list(APPEND CORE_SOURCES
        src/platform/linux/alsa_interface.cpp
        src/platform/linux/jack_interface.cpp
    )
endif()

# Create shared library
add_library(SharedAudioCore SHARED ${CORE_SOURCES})

# Set up JUCE
target_compile_definitions(SharedAudioCore
    PRIVATE
        JUCE_GLOBAL_MODULE_SETTINGS_INCLUDED=1
        JUCE_STANDALONE_APPLICATION=0
        JUCE_USE_CURL=0
        JUCE_WEB_BROWSER=0
)

# Link JUCE modules
target_link_libraries(SharedAudioCore
    PRIVATE
        juce::juce_audio_basics
        juce::juce_audio_devices
        juce::juce_audio_formats
        juce::juce_audio_processors
        juce::juce_audio_utils
        juce::juce_core
        juce::juce_data_structures
        juce::juce_events
)

# Platform-specific linking
if(PLATFORM_WINDOWS)
    target_link_libraries(SharedAudioCore
        PRIVATE
            winmm
            ole32
            oleaut32
            uuid
            user32
            advapi32
            kernel32
    )
    
    # Set Windows subsystem
    set_target_properties(SharedAudioCore PROPERTIES
        LINK_FLAGS "/SUBSYSTEM:WINDOWS"
    )
elseif(PLATFORM_MACOS)
    target_link_libraries(SharedAudioCore
        PRIVATE
            "-framework CoreAudio"
            "-framework AudioUnit"
            "-framework CoreServices"
            "-framework CoreMIDI"
            "-framework Accelerate"
    )
elseif(PLATFORM_LINUX)
    target_link_libraries(SharedAudioCore
        PRIVATE
            ${CMAKE_DL_LIBS}
            pthread
            rt
    )
    
    if(PORTAUDIO_FOUND)
        target_link_libraries(SharedAudioCore PRIVATE ${PORTAUDIO_LIBRARIES})
        target_include_directories(SharedAudioCore PRIVATE ${PORTAUDIO_INCLUDE_DIRS})
    endif()
endif()

# Export symbols for Node.js binding
if(PLATFORM_WINDOWS)
    set_target_properties(SharedAudioCore PROPERTIES
        WINDOWS_EXPORT_ALL_SYMBOLS ON
    )
endif()

# Compiler optimizations for real-time audio
target_compile_options(SharedAudioCore PRIVATE
    $<$<CONFIG:Release>:
        $<$<CXX_COMPILER_ID:MSVC>:/O2 /fp:fast /arch:AVX2>
        $<$<CXX_COMPILER_ID:GNU>:-O3 -march=native -ffast-math>
        $<$<CXX_COMPILER_ID:Clang>:-O3 -march=native -ffast-math>
    >
)

# Build tests
if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# Build examples
if(BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

# Build Electron binding
if(BUILD_ELECTRON_BINDING)
    add_subdirectory(bindings/electron)
endif()

# Install configuration
install(TARGETS SharedAudioCore
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
)

install(DIRECTORY include/ DESTINATION include)

# Generate pkg-config file
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/SharedAudioCore.pc.in"
    "${CMAKE_CURRENT_BINARY_DIR}/SharedAudioCore.pc"
    @ONLY
)

install(FILES "${CMAKE_CURRENT_BINARY_DIR}/SharedAudioCore.pc"
    DESTINATION lib/pkgconfig
)