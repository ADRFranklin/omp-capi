#include "Network.hpp"

#include "../ComponentManager.hpp"

#include <algorithm>
#include <climits>

namespace
{
bool validDirection(OMPNetDirection direction)
{
	return direction >= OMPNetDirection_IncomingPacket && direction <= OMPNetDirection_OutgoingRPC;
}

bool invoke(OMPNetSubscription& subscription, IPlayer* player, int id, NetworkBitStream& bs)
{
	return CAPINetworkManager::instance().dispatch(subscription, player, id, bs);
}
}

bool OMPNetSubscription::onReceivePacket(IPlayer& peer, int eventID, NetworkBitStream& bs) { return direction == OMPNetDirection_IncomingPacket ? invoke(*this, &peer, eventID, bs) : true; }
bool OMPNetSubscription::onReceiveRPC(IPlayer& peer, int eventID, NetworkBitStream& bs) { return direction == OMPNetDirection_IncomingRPC ? invoke(*this, &peer, eventID, bs) : true; }
bool OMPNetSubscription::onReceive(IPlayer& peer, NetworkBitStream& bs) { return invoke(*this, &peer, id, bs); }
bool OMPNetSubscription::onSendPacket(IPlayer* peer, int eventID, NetworkBitStream& bs) { return direction == OMPNetDirection_OutgoingPacket ? invoke(*this, peer, eventID, bs) : true; }
bool OMPNetSubscription::onSendRPC(IPlayer* peer, int eventID, NetworkBitStream& bs) { return direction == OMPNetDirection_OutgoingRPC ? invoke(*this, peer, eventID, bs) : true; }
bool OMPNetSubscription::onSend(IPlayer* peer, NetworkBitStream& bs) { return invoke(*this, peer, id, bs); }

CAPINetworkManager& CAPINetworkManager::instance()
{
	static CAPINetworkManager manager;
	return manager;
}

void CAPINetworkManager::initialize(ICore* core) { core_ = core; }

void CAPINetworkManager::attach(OMPNetSubscription& s, INetwork& n)
{
	if (s.all)
	{
		if (s.direction == OMPNetDirection_IncomingPacket || s.direction == OMPNetDirection_IncomingRPC)
			n.getInEventDispatcher().addEventHandler(&s, s.priority);
		else
			n.getOutEventDispatcher().addEventHandler(&s, s.priority);
	}
	else
	{
		switch (s.direction)
		{
		case OMPNetDirection_IncomingPacket: n.getPerPacketInEventDispatcher().addEventHandler(&s, s.id, s.priority); break;
		case OMPNetDirection_OutgoingPacket: n.getPerPacketOutEventDispatcher().addEventHandler(&s, s.id, s.priority); break;
		case OMPNetDirection_IncomingRPC: n.getPerRPCInEventDispatcher().addEventHandler(&s, s.id, s.priority); break;
		case OMPNetDirection_OutgoingRPC: n.getPerRPCOutEventDispatcher().addEventHandler(&s, s.id, s.priority); break;
		}
	}
}

void CAPINetworkManager::detach(OMPNetSubscription& s, INetwork& n)
{
	if (s.all)
	{
		if (s.direction == OMPNetDirection_IncomingPacket || s.direction == OMPNetDirection_IncomingRPC)
			n.getInEventDispatcher().removeEventHandler(&s);
		else
			n.getOutEventDispatcher().removeEventHandler(&s);
	}
	else
	{
		switch (s.direction)
		{
		case OMPNetDirection_IncomingPacket: n.getPerPacketInEventDispatcher().removeEventHandler(&s, s.id); break;
		case OMPNetDirection_OutgoingPacket: n.getPerPacketOutEventDispatcher().removeEventHandler(&s, s.id); break;
		case OMPNetDirection_IncomingRPC: n.getPerRPCInEventDispatcher().removeEventHandler(&s, s.id); break;
		case OMPNetDirection_OutgoingRPC: n.getPerRPCOutEventDispatcher().removeEventHandler(&s, s.id); break;
		}
	}
}

OMPNetSubscription* CAPINetworkManager::subscribe(OMPNetDirection direction, int id, int8_t priority,
	OMPNetCallback callback, void* userdata, bool all)
{
	if (!core_ || !callback || !validDirection(direction) || (!all && id < 0)) return nullptr;
	auto item = std::make_unique<OMPNetSubscription>();
	item->direction = direction; item->id = id; item->priority = priority;
	item->callback = callback; item->userdata = userdata; item->all = all;
	for (INetwork* network : core_->getNetworks()) attach(*item, *network);
	item->attached = true;
	auto* result = item.get();
	subscriptions_.push_back(std::move(item));
	return result;
}

bool CAPINetworkManager::unsubscribe(OMPNetSubscription* subscription)
{
	if (!subscription) return false;
	auto found = std::find_if(subscriptions_.begin(), subscriptions_.end(),
		[subscription](const auto& item) { return item.get() == subscription; });
	if (found == subscriptions_.end() || !subscription->active) return false;
	subscription->active = false;
	/* Logical removal is required here: open.mp's dispatcher is vector-backed,
	   so erasing from any callback would invalidate native iteration. */
	return true;
}

bool CAPINetworkManager::dispatch(OMPNetSubscription& s, IPlayer* player, int id, NetworkBitStream& bs)
{
	if (!s.active) return true;
	++dispatchDepth_;
	OMPNetBuffer buffer { bs.GetData(), static_cast<uint32_t>(bs.GetNumberOfBitsUsed()),
		static_cast<uint32_t>(bs.GetNumberOfBitsAllocated()), static_cast<uint32_t>(bs.GetReadOffset()), &bs };
	OMPNetResult result = OMPNetResult_Continue;
	try { result = s.callback(player, id, &buffer, s.userdata); } catch (...) { result = OMPNetResult_Continue; }
	leaveDispatch();
	return result != OMPNetResult_Drop;
}

void CAPINetworkManager::leaveDispatch() { --dispatchDepth_; }

void CAPINetworkManager::networkUnloaded(INetwork* network)
{
	if (!network) return;
	for (auto& subscription : subscriptions_)
		if (subscription->attached) detach(*subscription, *network);
}

void CAPINetworkManager::shutdown()
{
	if (core_)
		for (auto& subscription : subscriptions_)
			if (subscription->attached)
				for (INetwork* network : core_->getNetworks()) detach(*subscription, *network);
	subscriptions_.clear(); dispatchDepth_ = 0; core_ = nullptr;
}

extern "C" SDK_EXPORT OMPNetSubscription* Network_Subscribe(OMPNetDirection direction, int32_t id,
	int8_t priority, OMPNetCallback callback, void* userdata)
{
	return CAPINetworkManager::instance().subscribe(direction, id, priority, callback, userdata, false);
}

extern "C" SDK_EXPORT OMPNetSubscription* Network_SubscribeAll(OMPNetDirection direction,
	int8_t priority, OMPNetCallback callback, void* userdata)
{
	return CAPINetworkManager::instance().subscribe(direction, -1, priority, callback, userdata, true);
}

extern "C" SDK_EXPORT bool Network_Unsubscribe(OMPNetSubscription* subscription)
{
	return CAPINetworkManager::instance().unsubscribe(subscription);
}

extern "C" SDK_EXPORT bool Network_BufferResize(OMPNetBuffer* buffer, uint32_t bitLength)
{
	if (!buffer || !buffer->internal || bitLength > static_cast<uint32_t>(INT_MAX)) return false;
	auto& bs = *static_cast<NetworkBitStream*>(buffer->internal);
	const int oldLength = bs.GetNumberOfBitsUsed();
	if (bitLength > static_cast<uint32_t>(oldLength)) bs.EnsureBitCapacity(static_cast<int>(bitLength));
	bs.SetWriteOffset(static_cast<int>(bitLength));
	if (bs.GetReadOffset() > static_cast<int>(bitLength)) bs.SetReadOffset(static_cast<int>(bitLength));
	buffer->data = bs.GetData(); buffer->bit_length = bitLength;
	buffer->capacity_bits = static_cast<uint32_t>(bs.GetNumberOfBitsAllocated());
	buffer->read_offset_bits = static_cast<uint32_t>(bs.GetReadOffset());
	return true;
}

static bool validData(const uint8_t* data, uint32_t bits) { return bits == 0 || data != nullptr; }

extern "C" SDK_EXPORT bool Network_SendPacket(void* player, const uint8_t* data, uint32_t bits,
	int32_t channel, bool dispatchEvents)
{
	if (!player || !validData(data, bits)) return false;
	auto& peer = *static_cast<IPlayer*>(player);
	return peer.getNetworkData().network->sendPacket(peer, Span<uint8_t>(const_cast<uint8_t*>(data), bits), channel, dispatchEvents);
}

extern "C" SDK_EXPORT bool Network_SendRPC(void* player, int32_t id, const uint8_t* data,
	uint32_t bits, int32_t channel, bool dispatchEvents)
{
	if (!player || id < 0 || !validData(data, bits)) return false;
	auto& peer = *static_cast<IPlayer*>(player);
	return peer.getNetworkData().network->sendRPC(peer, id, Span<uint8_t>(const_cast<uint8_t*>(data), bits), channel, dispatchEvents);
}

extern "C" SDK_EXPORT uint32_t Network_BroadcastPacket(int32_t type, void* exceptPlayer,
	const uint8_t* data, uint32_t bits, int32_t channel, bool dispatchEvents)
{
	auto* core = CAPINetworkManager::instance().core(); if (!core || !validData(data, bits)) return 0;
	uint32_t sent = 0; auto* except = static_cast<IPlayer*>(exceptPlayer);
	for (INetwork* network : core->getNetworks())
		if (type < 0 || network->getNetworkType() == type)
			sent += network->broadcastPacket(Span<uint8_t>(const_cast<uint8_t*>(data), bits), channel, except, dispatchEvents);
	return sent;
}

extern "C" SDK_EXPORT uint32_t Network_BroadcastRPC(int32_t type, void* exceptPlayer, int32_t id,
	const uint8_t* data, uint32_t bits, int32_t channel, bool dispatchEvents)
{
	auto* core = CAPINetworkManager::instance().core(); if (!core || id < 0 || !validData(data, bits)) return 0;
	uint32_t sent = 0; auto* except = static_cast<IPlayer*>(exceptPlayer);
	for (INetwork* network : core->getNetworks())
		if (type < 0 || network->getNetworkType() == type)
			sent += network->broadcastRPC(id, Span<uint8_t>(const_cast<uint8_t*>(data), bits), channel, except, dispatchEvents);
	return sent;
}

extern "C" SDK_EXPORT uint32_t Network_Count()
{
	auto* core = CAPINetworkManager::instance().core(); return core ? static_cast<uint32_t>(core->getNetworks().size()) : 0;
}

extern "C" SDK_EXPORT int32_t Network_Type(uint32_t index)
{
	auto* core = CAPINetworkManager::instance().core(); if (!core) return -1;
	uint32_t current = 0; for (INetwork* network : core->getNetworks()) if (current++ == index) return network->getNetworkType();
	return -1;
}
