# Find testing framework (optional - GTest)
find_package(GTest QUIET)

if(GTEST_FOUND)
    message(STATUS "GTest found - building unit tests")
    
    # Create unit tests only if individual test files exist
    set(UNIT_TEST_SOURCES "")
    
    # Check for individual test files and add them if they exist
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_hardware_detection.cpp")
        list(APPEND UNIT_TEST_SOURCES test_hardware_detection.cpp)
    endif()
    
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_audio_core.cpp")
        list(APPEND UNIT_TEST_SOURCES test_audio_core.cpp)
    endif()
    
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_cue_manager.cpp")
        list(APPEND UNIT_TEST_SOURCES test_cue_manager.cpp)
    endif()
    
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_crossfade_engine.cpp")
        list(APPEND UNIT_TEST_SOURCES test_crossfade_engine.cpp)
    endif()
    
    # Only create unit_tests target if we have source files
    if(UNIT_TEST_SOURCES)
        add_executable(unit_tests ${UNIT_TEST_SOURCES})
        
        target_link_libraries(unit_tests 
            SharedAudioCore
            GTest::gtest_main
        )
        
        add_test(NAME unit_tests COMMAND unit_tests)
        
        message(STATUS "Built unit_tests with sources: ${UNIT_TEST_SOURCES}")
    else()
        message(STATUS "No GTest unit test source files found - skipping unit_tests target")
    endif()
    
else()
    message(STATUS "GTest not found - building manual tests instead")
endif()

# Always build manual test suite (fallback)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/manual_test_suite.cpp")
    add_executable(manual_tests manual_test_suite.cpp)
    target_link_libraries(manual_tests SharedAudioCore)
    
    # Windows-specific linking for manual tests
    if(WIN32)
        target_link_libraries(manual_tests
            winmm
            ole32
        )
        
        # Ensure it's a console app
        set_target_properties(manual_tests PROPERTIES
            WIN32_EXECUTABLE FALSE
        )
    endif()
    
    message(STATUS "Built manual_tests")
else()
    message(WARNING "manual_test_suite.cpp not found - no test targets available")
endif()

# Optional: Create a custom test target that runs available tests
if(TARGET manual_tests)
    add_custom_target(run_tests
        COMMAND manual_tests
        DEPENDS manual_tests
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        COMMENT "Running manual test suite"
    )
endif()

if(TARGET unit_tests)
    add_custom_target(run_unit_tests
        COMMAND unit_tests
        DEPENDS unit_tests
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        COMMENT "Running unit tests"
    )
endif()

# Print test configuration summary
message(STATUS "==========================================")
message(STATUS "Test Configuration Summary:")
if(TARGET unit_tests)
    message(STATUS "  - unit_tests (GTest): Available")
else()
    message(STATUS "  - unit_tests (GTest): Not available")
endif()

if(TARGET manual_tests)
    message(STATUS "  - manual_tests: Available")
else()
    message(STATUS "  - manual_tests: Not available")
endif()
message(STATUS "==========================================")