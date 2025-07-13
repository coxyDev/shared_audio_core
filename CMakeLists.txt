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
    add_definitions(-DWIN32_LEAN_AND_MEAN)  # Reduce Windows.h bloat
elseif(APPLE)
    set(PLATFORM_MACOS TRUE)
    add_definitions(-DPLATFORM_MACOS=1)
elseif(UNIX)
    set(PLATFORM_LINUX TRUE)
    add_definitions(-DPLATFORM_LINUX=1)
endif()

# =============================================
# PORTAUDIO DETECTION (Windows/vcpkg friendly)
# =============================================

if(WIN32)
    # On Windows, prefer vcpkg's CMake integration
    message(STATUS "Windows detected - using vcpkg CMake integration for PortAudio")
    
    # Try to find PortAudio via vcpkg's CMake config
    find_package(portaudio CONFIG QUIET)
    
    if(portaudio_FOUND)
        message(STATUS "✅ PortAudio found via vcpkg CMake config")
        set(PORTAUDIO_FOUND TRUE)
        set(PORTAUDIO_LIBRARIES portaudio)
        get_target_property(PORTAUDIO_INCLUDE_DIRS portaudio INTERFACE_INCLUDE_DIRECTORIES)
        
    else()
        # Fallback: Try to find manually installed PortAudio
        message(STATUS "vcpkg PortAudio not found, trying manual detection...")
        
        find_path(PORTAUDIO_INCLUDE_DIR portaudio.h
            HINTS 
                ${CMAKE_PREFIX_PATH}/include
                C:/portaudio/include
                C:/Program\ Files/portaudio/include
                C:/Program\ Files\ \(x86\)/portaudio/include
        )
        
        find_library(PORTAUDIO_LIBRARY 
            NAMES portaudio portaudio_static portaudio_x64 portaudio_x86
            HINTS
                ${CMAKE_PREFIX_PATH}/lib
                C:/portaudio/lib
                C:/Program\ Files/portaudio/lib
                C:/Program\ Files\ \(x86\)/portaudio/lib
        )
        
        if(PORTAUDIO_INCLUDE_DIR AND PORTAUDIO_LIBRARY)
            set(PORTAUDIO_FOUND TRUE)
            set(PORTAUDIO_INCLUDE_DIRS ${PORTAUDIO_INCLUDE_DIR})
            set(PORTAUDIO_LIBRARIES ${PORTAUDIO_LIBRARY})
            message(STATUS "✅ PortAudio found manually: ${PORTAUDIO_LIBRARY}")
        else()
            message(FATAL_ERROR "❌ PortAudio not found. Please install via vcpkg: 'vcpkg install portaudio:x64-windows'")
        endif()
    endif()
    
else()
    # On Unix systems, use pkg-config
    find_package(PkgConfig)
    
    if(PkgConfig_FOUND)
        pkg_check_modules(PORTAUDIO REQUIRED portaudio-2.0)
        message(STATUS "✅ PortAudio found via pkg-config")
    else()
        message(FATAL_ERROR "❌ pkg-config not found. Please install pkg-config and portaudio development packages.")
    endif()
endif()

# =============================================
# JUCE FRAMEWORK (Optional)
# =============================================

if(BUILD_JUCE_INTEGRATION)
    find_package(JUCE CONFIG QUIET)
    if(JUCE_FOUND)
        set(HAS_JUCE TRUE)
        add_definitions(-DHAS_JUCE=1)
        message(STATUS "✅ JUCE found - enabling JUCE integration")
    else()
        message(WARNING "⚠️  JUCE not found, JUCE integration will be disabled")
        set(BUILD_JUCE_INTEGRATION OFF)
    endif()
endif()

# =============================================
# INCLUDE DIRECTORIES
# =============================================

include_directories(include)
if(PORTAUDIO_INCLUDE_DIRS)
    include_directories(${PORTAUDIO_INCLUDE_DIRS})
endif()

# =============================================
# SOURCE FILES
# =============================================

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

# =============================================
# CREATE SHARED LIBRARY
# =============================================

add_library(SharedAudioCore SHARED ${CORE_SOURCES})

# Link PortAudio
target_link_libraries(SharedAudioCore ${PORTAUDIO_LIBRARIES})

# Platform-specific linking
if(PLATFORM_WINDOWS)
    target_link_libraries(SharedAudioCore
        winmm      # Windows Multimedia API
        dsound     # DirectSound
        ole32      # COM support
        user32     # User interface
        advapi32   # Registry access
        kernel32   # Core Windows APIs
    )
    
    # Windows-specific compile definitions
    target_compile_definitions(SharedAudioCore PRIVATE
        _CRT_SECURE_NO_WARNINGS
        WIN32_LEAN_AND_MEAN
        NOMINMAX
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
        m  # Math library
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

# =============================================
# COMPILER DEFINITIONS
# =============================================

target_compile_definitions(SharedAudioCore PRIVATE
    SHARED_AUDIO_CORE_VERSION_MAJOR=1
    SHARED_AUDIO_CORE_VERSION_MINOR=0
    SHARED_AUDIO_CORE_VERSION_PATCH=0
)

# =============================================
# BUILD SUBDIRECTORIES
# =============================================

# Build tests
if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# Build examples
if(BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

# Build Electron binding (optional, requires Node.js)
if(BUILD_ELECTRON_BINDING)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bindings/electron/CMakeLists.txt")
        add_subdirectory(bindings/electron)
    else()
        message(STATUS "⚠️  Electron binding directory not found - skipping")
    endif()
endif()

# =============================================
# INSTALLATION
# =============================================

install(TARGETS SharedAudioCore
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
)

install(DIRECTORY include/ DESTINATION include)

# =============================================
# STATUS INFORMATION
# =============================================

message(STATUS "==========================================")
message(STATUS "       SharedAudioCore Build Configuration")
message(STATUS "==========================================")
message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Platform: ${CMAKE_SYSTEM_NAME}")
message(STATUS "C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "")
message(STATUS "Dependencies:")
message(STATUS "  - PortAudio: ${PORTAUDIO_FOUND}")
if(PORTAUDIO_FOUND)
    message(STATUS "    Libraries: ${PORTAUDIO_LIBRARIES}")
    if(PORTAUDIO_INCLUDE_DIRS)
        message(STATUS "    Include Dirs: ${PORTAUDIO_INCLUDE_DIRS}")
    endif()
endif()
message(STATUS "  - JUCE Integration: ${BUILD_JUCE_INTEGRATION}")
message(STATUS "")
message(STATUS "Build Targets:")
message(STATUS "  - SharedAudioCore (library): YES")
message(STATUS "  - Examples: ${BUILD_EXAMPLES}")
message(STATUS "  - Tests: ${BUILD_TESTS}")
message(STATUS "  - Electron Binding: ${BUILD_ELECTRON_BINDING}")
message(STATUS "")
if(WIN32)
    message(STATUS "Windows-specific:")
    message(STATUS "  - Using vcpkg toolchain: ${VCPKG_TOOLCHAIN}")
    message(STATUS "  - Target Architecture: ${CMAKE_GENERATOR_PLATFORM}")
endif()
message(STATUS "==========================================")

# =============================================
# TROUBLESHOOTING INFORMATION
# =============================================

if(NOT PORTAUDIO_FOUND)
    message(STATUS "")
    message(STATUS "🔧 TROUBLESHOOTING PORTAUDIO:")
    message(STATUS "If PortAudio is not found, try:")
    if(WIN32)
        message(STATUS "  1. Install via vcpkg: 'C:/vcpkg/vcpkg install portaudio:x64-windows'")
        message(STATUS "  2. Use vcpkg toolchain: '-DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake'")
        message(STATUS "  3. Check vcpkg integration: 'C:/vcpkg/vcpkg integrate install'")
    else()
        message(STATUS "  1. Install portaudio dev package: 'sudo apt install libportaudio2 libportaudio-dev'")
        message(STATUS "  2. Install pkg-config: 'sudo apt install pkg-config'")
    endif()
    message(STATUS "==========================================")
endif()