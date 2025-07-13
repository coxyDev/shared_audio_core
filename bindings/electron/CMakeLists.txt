if(BUILD_ELECTRON_BINDING)
    # Find Node.js
    find_program(NODE_EXECUTABLE NAMES node nodejs)
    find_program(NPM_EXECUTABLE NAMES npm)
    
    if(NODE_EXECUTABLE AND NPM_EXECUTABLE)
        message(STATUS "Found Node.js: ${NODE_EXECUTABLE}")
        message(STATUS "Found NPM: ${NPM_EXECUTABLE}")
        
        # Custom target to build Node.js addon
        add_custom_target(electron_binding
            COMMAND ${NPM_EXECUTABLE} install
            COMMAND ${NPM_EXECUTABLE} run build
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bindings/electron
            COMMENT "Building Node.js addon for Electron integration"
        )
        
        # Make it depend on the main library
        add_dependencies(electron_binding SharedAudioCore)
    else()
        message(WARNING "Node.js or NPM not found - Electron binding will not be built")
    endif()
endif()