# Standalone open.mp CAPI component

This repository packages open.mp's official CAPI component as an independently
buildable shared library. The implementation and C ABI are intentionally kept
in sync with the component shipped by open.mp.

## Build

Use Clang or clang-cl and CMake 3.19 or newer:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_COMPILER=clang++
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

The component is written to `build/components/$CAPI.so` on Linux or
`build/components/$CAPI.dll` on Windows. Copy it into the server's
`components` directory. Do not load it alongside the built-in CAPI component.