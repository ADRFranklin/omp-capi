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

add_executable(network-bridge-test network_bridge.cpp
    ${PROJECT_SOURCE_DIR}/src/Impl/Network/Network.cpp)
target_include_directories(network-bridge-test PRIVATE ${PROJECT_SOURCE_DIR}/src)
target_compile_definitions(network-bridge-test PRIVATE CAPI_COMPONENT_BUILD)
target_link_libraries(network-bridge-test PRIVATE OMP-Network OMP-SDK OMP-CAPI)
add_test(NAME network-bridge COMMAND network-bridge-test)

add_library(c-header-smoke OBJECT c_header_smoke.c)
target_link_libraries(c-header-smoke PRIVATE OMP-CAPI)

add_executable(component-interop-test component_interop.cpp
    ${PROJECT_SOURCE_DIR}/src/Impl/ComponentInterop/ComponentInterop.cpp)
target_include_directories(component-interop-test PRIVATE ${PROJECT_SOURCE_DIR}/src)
target_compile_definitions(component-interop-test PRIVATE CAPI_COMPONENT_BUILD)
target_link_libraries(component-interop-test PRIVATE OMP-SDK OMP-CAPI)
add_test(NAME component-interop COMMAND component-interop-test)
