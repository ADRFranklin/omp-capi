#ifndef OMP_CAPI_BRIDGE_BENCHMARK_H
#define OMP_CAPI_BRIDGE_BENCHMARK_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

struct BenchBuffer { uint8_t* data; uint32_t bits; uint32_t capacity; };
struct BenchContext { int32_t mode; uint64_t accumulator; };
typedef int32_t (*BenchCallback)(struct BenchBuffer*, struct BenchContext*);

#ifdef __cplusplus
}
#endif
#endif
