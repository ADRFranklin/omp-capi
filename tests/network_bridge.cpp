#include "Impl/Network/Network.hpp"

#include <cassert>
#include <cstdint>

namespace
{
struct State
{
	int calls = 0;
	int id = -1;
	bool resize = false;
};

OMPNetResult callback(void* player, int32_t id, OMPNetBuffer* buffer, void* userdata)
{
	auto& state = *static_cast<State*>(userdata);
	assert(player == nullptr);
	++state.calls;
	state.id = id;
	assert(buffer->bit_length == 16);
	buffer->data[0] = 0x42;
	if (state.resize)
	{
		assert(Network_BufferResize(buffer, 4097));
		assert(buffer->bit_length == 4097);
		assert(buffer->capacity_bits >= 4097);
		buffer->data[512] = 0x80;
	}
	return OMPNetResult_Drop;
}
}

int main()
{
	NetworkBitStream stream;
	const uint8_t input[] { 1, 2 };
	stream.WriteBits(input, 16, false);

	State state;
	OMPNetSubscription subscription {};
	subscription.direction = OMPNetDirection_OutgoingPacket;
	subscription.id = 207;
	subscription.callback = callback;
	subscription.userdata = &state;
	subscription.active = true;

	assert(!subscription.onSend(nullptr, stream));
	assert(state.calls == 1 && state.id == 207);
	assert(stream.GetData()[0] == 0x42);

	// A global packet subscription must not observe RPC dispatch.
	assert(subscription.onSendRPC(nullptr, 10, stream));
	assert(state.calls == 1);

	state.resize = true;
	assert(!subscription.onSendPacket(nullptr, 207, stream));
	assert(stream.GetNumberOfBitsUsed() == 4097);
	assert(stream.GetData()[512] == 0x80);

	subscription.active = false;
	assert(subscription.onSend(nullptr, stream));
	assert(state.calls == 2);
	return 0;
}
