set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR AMD64)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

if(NOT DEFINED ENV{XWIN_ROOT})
    message(FATAL_ERROR "XWIN_ROOT must point to an xwin splat directory")
endif()

set(XWIN_ROOT "$ENV{XWIN_ROOT}")
set(CMAKE_C_COMPILER clang-cl)
set(CMAKE_CXX_COMPILER clang-cl)
set(CMAKE_LINKER lld-link)
set(CMAKE_AR llvm-lib)
set(CMAKE_MT llvm-mt)
set(CMAKE_RC_COMPILER llvm-rc)

set(XWIN_INCLUDE_FLAGS
    "/imsvc${XWIN_ROOT}/crt/include /imsvc${XWIN_ROOT}/sdk/include/ucrt /imsvc${XWIN_ROOT}/sdk/include/shared /imsvc${XWIN_ROOT}/sdk/include/um")
set(XWIN_LIBRARY_FLAGS
    "/libpath:${XWIN_ROOT}/crt/lib/x86_64 /libpath:${XWIN_ROOT}/sdk/lib/ucrt/x86_64 /libpath:${XWIN_ROOT}/sdk/lib/um/x86_64")

set(CMAKE_C_FLAGS_INIT "${XWIN_INCLUDE_FLAGS}")
set(CMAKE_CXX_FLAGS_INIT "${XWIN_INCLUDE_FLAGS} /EHsc")
set(CMAKE_EXE_LINKER_FLAGS_INIT "${XWIN_LIBRARY_FLAGS}")
set(CMAKE_SHARED_LINKER_FLAGS_INIT "${XWIN_LIBRARY_FLAGS}")
set(CMAKE_MODULE_LINKER_FLAGS_INIT "${XWIN_LIBRARY_FLAGS}")
