# Main test program (this already exists)
add_executable(test_shared_audio test_shared_audio.cpp)
target_link_libraries(test_shared_audio SharedAudioCore)

# Hardware detection test
add_executable(hardware_test hardware_test.cpp)
target_link_libraries(hardware_test SharedAudioCore)

# Performance test
add_executable(performance_test performance_test.cpp)
target_link_libraries(performance_test SharedAudioCore)

# Windows-specific linking
if(WIN32)
    target_link_libraries(hardware_test
        winmm
        ole32
    )
    target_link_libraries(performance_test
        winmm
        ole32
    )
    target_link_libraries(test_shared_audio
        winmm
        ole32
    )
endif()

# Set Windows console properties for proper output
if(WIN32)
    set_target_properties(hardware_test PROPERTIES
        WIN32_EXECUTABLE FALSE
    )
    set_target_properties(performance_test PROPERTIES
        WIN32_EXECUTABLE FALSE
    )
    set_target_properties(test_shared_audio PROPERTIES
        WIN32_EXECUTABLE FALSE
    )
endif()