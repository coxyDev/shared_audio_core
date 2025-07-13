# Find testing framework (optional)
find_package(GTest QUIET)

if(GTEST_FOUND)
    # Google Test based tests
    add_executable(unit_tests
        test_hardware_detection.cpp
        test_audio_core.cpp
        test_cue_manager.cpp
        test_crossfade_engine.cpp
    )
    
    target_link_libraries(unit_tests 
        SharedAudioCore
        GTest::gtest_main
    )
    
    add_test(NAME unit_tests COMMAND unit_tests)
else()
    # Simple manual tests
    add_executable(manual_tests
        manual_test_suite.cpp
    )
    
    target_link_libraries(manual_tests SharedAudioCore)
endif()