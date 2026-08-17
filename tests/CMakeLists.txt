if(UNIX AND NOT APPLE)
    add_test(NAME component-abi-smoke
        COMMAND ${CMAKE_COMMAND} -DCOMPONENT_FILE=$<TARGET_FILE:openmp-capi>
            -P ${CMAKE_CURRENT_SOURCE_DIR}/check_abi.cmake)
endif()
