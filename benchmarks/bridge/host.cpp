#include "bridge.h"

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <dlfcn.h>
#include <string>
#include <sys/resource.h>
#include <vector>

__attribute__((noinline)) int32_t nativeCallback(BenchBuffer* buffer, BenchContext* context)
{
	switch (context->mode)
	{
	case 1: context->accumulator += buffer->data[0] + buffer->bits; break;
	case 2: buffer->data[0] ^= 1; break;
	case 3: return 1;
	default: break;
	}
	return 0;
}

int main(int argc, char** argv)
{
	if (argc != 3)
	{
		std::fprintf(stderr, "usage: %s <runtime> <native|shared-library>\n", argv[0]);
		return 2;
	}
	void* library = nullptr;
	BenchCallback callback = nativeCallback;
	using Begin = void (*)(); using Allocations = uint64_t (*)();
	Begin begin = nullptr; Allocations allocations = nullptr;
	if (std::strcmp(argv[2], "native") != 0)
	{
		library = dlopen(argv[2], RTLD_NOW | RTLD_LOCAL);
		if (!library) { std::fprintf(stderr, "%s\n", dlerror()); return 2; }
		callback = reinterpret_cast<BenchCallback>(dlsym(library, "bench_callback"));
		if (!callback) { std::fprintf(stderr, "%s\n", dlerror()); return 2; }
		begin = reinterpret_cast<Begin>(dlsym(library, "bench_begin"));
		allocations = reinterpret_cast<Allocations>(dlsym(library, "bench_allocations"));
	}

	constexpr size_t batches = 20000;
	constexpr size_t callsPerBatch = 100;
	const char* scenarios[] { "empty", "inspect", "modify", "block" };
	std::puts("runtime,scenario,calls,avg_ns,p50_ns,p95_ns,p99_ns,cpu_ms,max_rss_kb,allocations");
	for (int mode = 0; mode != 4; ++mode)
	{
		uint8_t bytes[64] { 207 };
		BenchBuffer buffer { bytes, 512, 512 };
		BenchContext context { mode, 0 };
		std::vector<double> latency; latency.reserve(batches);
		rusage before {}, after {}; getrusage(RUSAGE_SELF, &before);
		if (begin) begin();
		auto totalStart = std::chrono::steady_clock::now();
		for (size_t batch = 0; batch != batches; ++batch)
		{
			auto start = std::chrono::steady_clock::now();
			for (size_t call = 0; call != callsPerBatch; ++call) context.accumulator += callback(&buffer, &context);
			auto end = std::chrono::steady_clock::now();
			latency.push_back(std::chrono::duration<double, std::nano>(end - start).count() / callsPerBatch);
		}
		auto totalEnd = std::chrono::steady_clock::now(); getrusage(RUSAGE_SELF, &after);
		std::sort(latency.begin(), latency.end());
		auto percentile = [&](double p) { return latency[static_cast<size_t>(p * (latency.size() - 1))]; };
		double average = std::chrono::duration<double, std::nano>(totalEnd - totalStart).count() / (batches * callsPerBatch);
		double cpu = (after.ru_utime.tv_sec - before.ru_utime.tv_sec) * 1000.0 + (after.ru_utime.tv_usec - before.ru_utime.tv_usec) / 1000.0;
		const uint64_t allocationCount = allocations ? allocations() : 0;
		std::printf("%s,%s,%zu,%.3f,%.3f,%.3f,%.3f,%.3f,%ld,%llu\n", argv[1], scenarios[mode], batches * callsPerBatch,
			average, percentile(.50), percentile(.95), percentile(.99), cpu, after.ru_maxrss,
			static_cast<unsigned long long>(allocationCount));
		if (context.accumulator == UINT64_MAX) std::abort();
	}
	if (library) dlclose(library);
}
