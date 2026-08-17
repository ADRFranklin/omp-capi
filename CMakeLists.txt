cmake_minimum_required(VERSION 3.19)

project(openmp-capi-component VERSION 1.5.8 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)

option(OMP_CAPI_BUILD_TESTS "Build standalone component tests" ON)
set(OMP_BUILD_VERSION 0 CACHE STRING "open.mp build number reported by the component")

add_subdirectory(lib/open.mp-capi)
add_subdirectory(lib/open.mp-sdk)
add_subdirectory(lib/open.mp-network)

file(GLOB_RECURSE OMP_CAPI_COMPONENT_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.hpp")

add_library(openmp-capi SHARED ${OMP_CAPI_COMPONENT_SOURCES})
target_link_libraries(openmp-capi PRIVATE OMP-CAPI OMP-SDK OMP-Network)
target_compile_definitions(openmp-capi PRIVATE
    CAPI_COMPONENT_BUILD
    OMP_VERSION_MAJOR=${PROJECT_VERSION_MAJOR}
    OMP_VERSION_MINOR=${PROJECT_VERSION_MINOR}
    OMP_VERSION_PATCH=${PROJECT_VERSION_PATCH}
    BUILD_NUMBER=${OMP_BUILD_VERSION})

if(MSVC)
    target_compile_options(openmp-capi PRIVATE /W4 /wd4101)
else()
    target_compile_options(openmp-capi PRIVATE
        -Wall -Wextra -Wno-unused-variable -fno-eliminate-unused-debug-types)
endif()

# open.mp component filenames conventionally begin with '$'.
set_target_properties(openmp-capi PROPERTIES
    PREFIX "$"
    OUTPUT_NAME "CAPI"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/components"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/components")

include(CTest)
if(BUILD_TESTING AND OMP_CAPI_BUILD_TESTS)
    add_subdirectory(tests)
endif()
