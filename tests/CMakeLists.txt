if(UNIX AND NOT APPLE)
    add_test(NAME component-abi-smoke
        COMMAND ${CMAKE_COMMAND} -DCOMPONENT_FILE=$<TARGET_FILE:openmp-capi>
            -P ${CMAKE_CURRENT_SOURCE_DIR}/check_abi.cmake)
endif()

add_library(capi-parity-fixture SHARED integration/parity_component.cpp)
target_link_libraries(capi-parity-fixture PRIVATE OMP-CAPI OMP-SDK)
target_compile_definitions(capi-parity-fixture PRIVATE
    OMP_VERSION_MAJOR=${PROJECT_VERSION_MAJOR}
    OMP_VERSION_MINOR=${PROJECT_VERSION_MINOR}
    OMP_VERSION_PATCH=${PROJECT_VERSION_PATCH}
    BUILD_NUMBER=${OMP_BUILD_VERSION})
set_target_properties(capi-parity-fixture PROPERTIES
    PREFIX "$"
    OUTPUT_NAME "CAPIParityFixture"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tests/components"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tests/components")
