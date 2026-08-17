package main

/*
#include "../bridge.h"
*/
import "C"
import "unsafe"
import "runtime"

var startingMallocs uint64

//export bench_begin
func bench_begin() {
	var stats runtime.MemStats
	runtime.ReadMemStats(&stats)
	startingMallocs = stats.Mallocs
}

//export bench_allocations
func bench_allocations() C.uint64_t {
	var stats runtime.MemStats
	runtime.ReadMemStats(&stats)
	return C.uint64_t(stats.Mallocs - startingMallocs)
}

//export bench_callback
func bench_callback(buffer *C.struct_BenchBuffer, context *C.struct_BenchContext) C.int32_t {
	data := unsafe.Slice((*byte)(unsafe.Pointer(buffer.data)), 1)
	switch context.mode {
	case 1:
		context.accumulator += C.uint64_t(data[0]) + C.uint64_t(buffer.bits)
	case 2:
		data[0] ^= 1
	case 3:
		return 1
	}
	return 0
}

func main() {}
