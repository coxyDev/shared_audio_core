# Test program
add_executable(test_shared_audio test_shared_audio.cpp)
target_link_libraries(test_shared_audio SharedAudioCore)

# Hardware detection test
add_executable(hardware_test hardware_test.cpp)
target_link_libraries(hardware_test SharedAudioCore)

# Performance test
add_executable(performance_test performance_test.cpp)
target_link_libraries(performance_test SharedAudioCore)