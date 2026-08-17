set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR AMD64)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

if(NOT DEFINED ENV{XWIN_ROOT})
    message(FATAL_ERROR "XWIN_ROOT must point to an xwin splat directory")
endif()

set(XWIN_ROOT "$ENV{XWIN_ROOT}")
find_program(CLANG_CL NAMES clang-cl clang-cl-19 REQUIRED)
find_program(LLD_LINK NAMES lld-link lld-link-19 REQUIRED)
find_program(LLVM_LIB NAMES llvm-lib llvm-lib-19 REQUIRED)
find_program(LLVM_MT NAMES llvm-mt llvm-mt-19 REQUIRED)
find_program(LLVM_RC NAMES llvm-rc llvm-rc-19 REQUIRED)
set(CMAKE_C_COMPILER "${CLANG_CL}")
set(CMAKE_CXX_COMPILER "${CLANG_CL}")
set(CMAKE_LINKER "${LLD_LINK}")
set(CMAKE_AR "${LLVM_LIB}")
set(CMAKE_MT "${LLVM_MT}")
set(CMAKE_RC_COMPILER "${LLVM_RC}")

set(XWIN_INCLUDE_FLAGS
    "/imsvc${XWIN_ROOT}/crt/include /imsvc${XWIN_ROOT}/sdk/include/ucrt /imsvc${XWIN_ROOT}/sdk/include/shared /imsvc${XWIN_ROOT}/sdk/include/um")
set(XWIN_LIBRARY_FLAGS
    "/libpath:${XWIN_ROOT}/crt/lib/x86_64 /libpath:${XWIN_ROOT}/sdk/lib/ucrt/x86_64 /libpath:${XWIN_ROOT}/sdk/lib/um/x86_64")

set(CMAKE_C_FLAGS_INIT "${XWIN_INCLUDE_FLAGS}")
set(CMAKE_CXX_FLAGS_INIT "${XWIN_INCLUDE_FLAGS} /EHsc")
set(CMAKE_EXE_LINKER_FLAGS_INIT "${XWIN_LIBRARY_FLAGS}")
set(CMAKE_SHARED_LINKER_FLAGS_INIT "${XWIN_LIBRARY_FLAGS}")
set(CMAKE_MODULE_LINKER_FLAGS_INIT "${XWIN_LIBRARY_FLAGS}")
