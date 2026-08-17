#pragma once

#include <bitstream.hpp>
#include <network.hpp>
#include <ompcapi.h>

#include <memory>
#include <vector>

struct OMPNetSubscription final : NetworkInEventHandler,
	NetworkOutEventHandler, SingleNetworkInEventHandler, SingleNetworkOutEventHandler
{
	OMPNetDirection direction;
	int id;
	int8_t priority;
	OMPNetCallback callback;
	void* userdata;
	bool all;
	bool active = true;
	bool attached = false;

	bool onReceivePacket(IPlayer& peer, int eventID, NetworkBitStream& bs) override;
	bool onReceiveRPC(IPlayer& peer, int eventID, NetworkBitStream& bs) override;
	bool onReceive(IPlayer& peer, NetworkBitStream& bs) override;
	bool onSendPacket(IPlayer* peer, int eventID, NetworkBitStream& bs) override;
	bool onSendRPC(IPlayer* peer, int eventID, NetworkBitStream& bs) override;
	bool onSend(IPlayer* peer, NetworkBitStream& bs) override;
};

class CAPINetworkManager
{
public:
	static CAPINetworkManager& instance();
	void initialize(ICore* core);
	void shutdown();
	void networkUnloaded(INetwork* network);
	OMPNetSubscription* subscribe(OMPNetDirection direction, int id, int8_t priority,
		OMPNetCallback callback, void* userdata, bool all);
	bool unsubscribe(OMPNetSubscription* subscription);
	bool dispatch(OMPNetSubscription& subscription, IPlayer* player, int id, NetworkBitStream& bs);
	void leaveDispatch();
	ICore* core() const { return core_; }

private:
	void attach(OMPNetSubscription& subscription, INetwork& network);
	void detach(OMPNetSubscription& subscription, INetwork& network);

	ICore* core_ = nullptr;
	unsigned dispatchDepth_ = 0;
	std::vector<std::unique_ptr<OMPNetSubscription>> subscriptions_;
};
