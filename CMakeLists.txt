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
option(BUILD_JUCE_INTEGRATION "Build JUCE integration" OFF)

# Platform detection
if(WIN32)
    set(PLATFORM_WINDOWS TRUE)
    add_definitions(-DPLATFORM_WINDOWS=1)
    add_definitions(-D_WIN32_WINNT=0x0601)  # Windows 7+
    add_definitions(-DWINVER=0x0601)
    add_definitions(-DNOMINMAX)  # Prevent Windows.h min/max conflicts
elseif(APPLE)
    set(PLATFORM_MACOS TRUE)
    add_definitions(-DPLATFORM_MACOS=1)
elseif(UNIX)
    set(PLATFORM_LINUX TRUE)
    add_definitions(-DPLATFORM_LINUX=1)
endif()

# Find required packages
find_package(PkgConfig REQUIRED)

# PortAudio for cross-platform audio I/O
if(PLATFORM_WINDOWS)
    # Use vcpkg or manually installed PortAudio on Windows
    find_path(PORTAUDIO_INCLUDE_DIR portaudio.h)
    find_library(PORTAUDIO_LIBRARY portaudio)
    
    if(NOT PORTAUDIO_INCLUDE_DIR OR NOT PORTAUDIO_LIBRARY)
        message(WARNING "PortAudio not found via vcpkg. Trying pkg-config...")
        pkg_check_modules(PORTAUDIO portaudio-2.0)
    else()
        set(PORTAUDIO_INCLUDE_DIRS ${PORTAUDIO_INCLUDE_DIR})
        set(PORTAUDIO_LIBRARIES ${PORTAUDIO_LIBRARY})
    endif()
else()
    pkg_check_modules(PORTAUDIO REQUIRED portaudio-2.0)
endif()

# Optional: JUCE Framework (for MainStageSampler integration)
if(BUILD_JUCE_INTEGRATION)
    find_package(JUCE CONFIG QUIET)
    if(JUCE_FOUND)
        set(HAS_JUCE TRUE)
        add_definitions(-DHAS_JUCE=1)
        message(STATUS "JUCE found - enabling JUCE integration")
    else()
        message(WARNING "JUCE not found, JUCE integration will be disabled")
        set(BUILD_JUCE_INTEGRATION OFF)
    endif()
endif()

# Include directories
include_directories(include)
include_directories(${PORTAUDIO_INCLUDE_DIRS})

# Core source files (only include files that actually exist)
set(CORE_SOURCES
    src/core/shared_audio_core.cpp
    src/hardware/hardware_detector.cpp
    src/show_control/cue_audio_manager.cpp
    src/show_control/crossfade_engine.cpp
)

# Platform-specific sources (commented out until implemented)
# TODO: Add these files as they are implemented
#if(PLATFORM_WINDOWS)
#    list(APPEND CORE_SOURCES
#        src/platform/windows/asio_driver_manager.cpp
#    )
#elseif(PLATFORM_MACOS)
#    list(APPEND CORE_SOURCES
#        src/platform/macos/core_audio_interface.cpp
#    )
#elseif(PLATFORM_LINUX)
#    list(APPEND CORE_SOURCES
#        src/platform/linux/alsa_interface.cpp
#        src/platform/linux/pulse_interface.cpp
#    )
#endif()

# JUCE integration sources (when available)
if(BUILD_JUCE_INTEGRATION AND HAS_JUCE)
    list(APPEND CORE_SOURCES
        src/juce_integration/juce_audio_bridge.cpp
        src/juce_integration/juce_effects_processor.cpp
    )
endif()

# Create shared library
add_library(SharedAudioCore SHARED ${CORE_SOURCES})

# Link libraries
target_link_libraries(SharedAudioCore 
    ${PORTAUDIO_LIBRARIES}
)

# Platform-specific linking
if(PLATFORM_WINDOWS)
    target_link_libraries(SharedAudioCore
        winmm
        dsound
        ole32
        user32
    )
elseif(PLATFORM_MACOS)
    target_link_libraries(SharedAudioCore
        "-framework CoreAudio"
        "-framework AudioUnit"
        "-framework CoreServices"
    )
elseif(PLATFORM_LINUX)
    target_link_libraries(SharedAudioCore
        ${PORTAUDIO_LIBRARIES}
        pthread
    )
endif()

# JUCE linking
if(BUILD_JUCE_INTEGRATION AND HAS_JUCE)
    target_link_libraries(SharedAudioCore
        juce::juce_audio_utils
        juce::juce_audio_devices
        juce::juce_audio_formats
        juce::juce_audio_processors
    )
endif()

# Compiler definitions
target_compile_definitions(SharedAudioCore PRIVATE
    SHARED_AUDIO_CORE_VERSION_MAJOR=1
    SHARED_AUDIO_CORE_VERSION_MINOR=0
    SHARED_AUDIO_CORE_VERSION_PATCH=0
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

# Status information
message(STATUS "==========================================")
message(STATUS "SharedAudioCore Build Configuration")
message(STATUS "==========================================")
message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Platform: ${CMAKE_SYSTEM_NAME}")
message(STATUS "PortAudio Found: ${PORTAUDIO_FOUND}")
message(STATUS "JUCE Integration: ${BUILD_JUCE_INTEGRATION}")
message(STATUS "Build Examples: ${BUILD_EXAMPLES}")
message(STATUS "Build Tests: ${BUILD_TESTS}")
message(STATUS "Build Electron Binding: ${BUILD_ELECTRON_BINDING}")
message(STATUS "==========================================")

# Warning about missing optional files
message(STATUS "Note: Some optional platform-specific files are not yet implemented")
message(STATUS "The library will build with core functionality only")
message(STATUS "==========================================")