#include "bridge.h"

int32_t bench_callback(struct BenchBuffer* buffer, struct BenchContext* context)
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

void bench_begin(void) {}
uint64_t bench_allocations(void) { return 0; }
