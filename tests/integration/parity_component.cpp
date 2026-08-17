#include <sdk.hpp>
#include <ompcapi.h>

#include <fstream>
#include <string>

namespace
{
constexpr uint64_t ParityComponentUID = 0xD93D4096397AC9D1ULL;
constexpr uint64_t ExampleInterfaceUID = 0x4350415049544553ULL;
constexpr uint64_t GoInterfaceUID = 0x474F41504954424CULL;

struct ExampleAPI
{
	OMPComponentAPIHeader header;
	int32_t (*add)(int32_t, int32_t);
};

int32_t add(int32_t left, int32_t right) { return left + right; }
const ExampleAPI exampleAPI { { ExampleInterfaceUID, 1, sizeof(ExampleAPI) }, add };

using InteropGoQuery = int32_t (*)(OMPAPI_t*);
using InteropGoAPI = const OMPComponentAPIHeader* (*)();
void* goLibrary = nullptr;
OMPComponentWatch* selfWatch = nullptr;

OMPAPI_t api {};
OMPNetSubscription* outgoingA = nullptr;
OMPNetSubscription* outgoingB = nullptr;
OMPNetSubscription* incomingAll = nullptr;
OMPNetSubscription* incomingRPC = nullptr;
bool recursed = false;
bool disconnectedInsideCallback = false;
bool sawIncomingRPC = false;

void appendNetwork(const std::string& line)
{
	std::ofstream output("capi-parity.txt", std::ios::app);
	output << line << '\n';
}

void componentInvalidated(OMPComponentHandle*, void*)
{
	appendNetwork("component.invalidated=1");
}

OMPNetResult outgoingBCallback(void*, int32_t, OMPNetBuffer*, void*)
{
	appendNetwork("network.outgoing_b_called=1");
	return OMPNetResult_Continue;
}

OMPNetResult outgoingACallback(void*, int32_t id, OMPNetBuffer* buffer, void*)
{
	appendNetwork("network.outgoing_a.id=" + std::to_string(id));
	appendNetwork("network.outgoing_a.bits=" + std::to_string(buffer->bit_length));
	appendNetwork("network.unsubscribe_other=" + std::to_string(api.Network.Unsubscribe(outgoingB)));
	appendNetwork("network.unsubscribe_self=" + std::to_string(api.Network.Unsubscribe(outgoingA)));
	appendNetwork("network.unsubscribe_self_twice=" + std::to_string(api.Network.Unsubscribe(outgoingA)));
	return OMPNetResult_Continue;
}

OMPNetResult resizeAndDrop(void*, int32_t id, OMPNetBuffer* buffer, void*)
{
	appendNetwork("network.resize.id=" + std::to_string(id));
	buffer->data[0] = 201;
	appendNetwork("network.resize.ok=" + std::to_string(api.Network.BufferResize(buffer, 4097)));
	appendNetwork("network.resize.bits=" + std::to_string(buffer->bit_length));
	return OMPNetResult_Drop;
}

OMPNetResult incomingCallback(void* player, int32_t id, OMPNetBuffer* buffer, void*)
{
	if (!recursed)
	{
		recursed = true;
		const uint8_t payload[] { 7 };
		appendNetwork("network.incoming.id=" + std::to_string(id));
		appendNetwork("network.incoming.bits=" + std::to_string(buffer->bit_length));
		appendNetwork("network.reentrant_send=" + std::to_string(api.Network.SendRPC(player, 251, payload, 8, 0, true)));
	}
	else if (!disconnectedInsideCallback)
	{
		disconnectedInsideCallback = true;
		appendNetwork("network.disconnect_inside_callback=" + std::to_string(api.Player.Kick(player)));
	}
	return OMPNetResult_Continue;
}

OMPNetResult incomingRPCCallback(void*, int32_t id, OMPNetBuffer* buffer, void*)
{
	if (!sawIncomingRPC)
	{
		sawIncomingRPC = true;
		appendNetwork("network.incoming_rpc.id=" + std::to_string(id));
		appendNetwork("network.incoming_rpc.bits=" + std::to_string(buffer->bit_length));
	}
	return OMPNetResult_Continue;
}

void appendPlayerEvent(const char* event, EventArgs_Common* args)
{
	std::ofstream output("capi-parity.txt", std::ios::app);
	void* player = *static_cast<void**>(args->list[0]);
	output << event << ".args=" << args->size << '\n';
	const int playerID = api.Player.GetID(player);
	output << event << ".id=" << playerID << '\n';
	output << event << ".from_id_matches=" << (api.Player.FromID(playerID) == player) << '\n';
	CAPIStringView name {};
	api.Player.GetName(player, &name);
	output << event << ".name=" << std::string(name.data, name.len) << '\n';
}

bool onPlayerConnect(EventArgs_Common* args)
{
	appendPlayerEvent("player.connect", args);
	void* player = *static_cast<void**>(args->list[0]);
	const uint8_t rpc[] { 1, 2 };
	const uint8_t packet[] { 200, 2 };
	appendNetwork("network.send_rpc_first=" + std::to_string(api.Network.SendRPC(player, 250, rpc, 16, 0, true)));
	appendNetwork("network.send_rpc_second=" + std::to_string(api.Network.SendRPC(player, 250, rpc, 16, 0, true)));
	appendNetwork("network.send_packet_dropped=" + std::to_string(api.Network.SendPacket(player, packet, 16, 0, true)));
	appendNetwork("network.broadcast_rpc=" + std::to_string(api.Network.BroadcastRPC(-1, nullptr, 252, rpc, 16, 0, true)));
	appendNetwork("network.broadcast_packet=" + std::to_string(api.Network.BroadcastPacket(-1, nullptr, packet, 16, 0, false)));
	return true;
}

bool onPlayerDisconnect(EventArgs_Common* args)
{
	appendPlayerEvent("player.disconnect", args);
	std::ofstream output("capi-parity.txt", std::ios::app);
	output << "player.disconnect.reason=" << *static_cast<int*>(args->list[1]) << '\n';
	return true;
}

class CAPIParityFixture final : public IComponent
{
public:
	PROVIDE_UID(ParityComponentUID)

	StringView componentName() const override
	{
		return "CAPI parity fixture";
	}

	SemanticVersion componentVersion() const override
	{
		return { 1, 0, 0, 0 };
	}

	void onLoad(ICore*) override
	{
	}

	void onInit(IComponentList*) override
	{
	}

	void onReady() override
	{
		std::ofstream output("capi-parity.txt", std::ios::trunc);
		if (!omp_initialize_capi(&api))
		{
			output << "initialize=0\n";
			return;
		}

		output << "initialize=1\n";
		output << "core.max_players=" << api.Core.MaxPlayers() << '\n';
		output << "core.tick_available=" << (api.Core.TickCount() > 0) << '\n';

		int id = -1;
		void* actor = api.Actor.Create(1, 10.0f, 20.0f, 30.0f, 90.0f, &id);
		output << "actor.create=" << (actor != nullptr) << '\n';
		output << "actor.id=" << (actor ? api.Actor.GetID(actor) : -1) << '\n';
		output << "actor.valid=" << (actor && api.Actor.IsValid(actor)) << '\n';
		output << "actor.destroy=" << (actor && api.Actor.Destroy(actor)) << '\n';

		id = -1;
		void* vehicle = api.Vehicle.Create(400, 10.0f, 20.0f, 30.0f, 90.0f, 1, 2, 60, false, &id);
		output << "vehicle.create=" << (vehicle != nullptr) << '\n';
		output << "vehicle.id=" << (vehicle ? api.Vehicle.GetID(vehicle) : -1) << '\n';
		output << "vehicle.valid=" << (vehicle && api.Vehicle.IsValid(vehicle)) << '\n';
		output << "vehicle.destroy=" << (vehicle && api.Vehicle.Destroy(vehicle)) << '\n';

		id = -1;
		void* object = api.Object.Create(19300, 10.0f, 20.0f, 30.0f, 0.0f, 0.0f, 0.0f, 200.0f, &id);
		output << "object.create=" << (object != nullptr) << '\n';
		output << "object.id=" << (object ? api.Object.GetID(object) : -1) << '\n';
		output << "object.valid=" << (object && api.Object.IsValid(object)) << '\n';
		output << "object.destroy=" << (object && api.Object.Destroy(object)) << '\n';

		id = -1;
		void* pickup = api.Pickup.Create(1240, 1, 10.0f, 20.0f, 30.0f, 0, &id);
		output << "pickup.create=" << (pickup != nullptr) << '\n';
		output << "pickup.id=" << (pickup ? api.Pickup.GetID(pickup) : -1) << '\n';
		output << "pickup.valid=" << (pickup && api.Pickup.IsValid(pickup)) << '\n';
		output << "pickup.destroy=" << (pickup && api.Pickup.Destroy(pickup)) << '\n';

		output << "event.add_connect=" << api.Event.AddHandler("onPlayerConnect", EventPriorityType_Default, reinterpret_cast<void*>(onPlayerConnect)) << '\n';
		output << "event.add_disconnect=" << api.Event.AddHandler("onPlayerDisconnect", EventPriorityType_Default, reinterpret_cast<void*>(onPlayerDisconnect)) << '\n';
		output << "network.count=" << api.Network.Count() << '\n';
		if (api.Network.Count()) output << "network.type0=" << api.Network.Type(0) << '\n';
		outgoingA = api.Network.Subscribe(OMPNetDirection_OutgoingRPC, 250, -10, outgoingACallback, nullptr);
		outgoingB = api.Network.Subscribe(OMPNetDirection_OutgoingRPC, 250, 0, outgoingBCallback, nullptr);
		api.Network.Subscribe(OMPNetDirection_OutgoingPacket, 200, 0, resizeAndDrop, nullptr);
		incomingAll = api.Network.SubscribeAll(OMPNetDirection_IncomingPacket, 0, incomingCallback, nullptr);
		incomingRPC = api.Network.SubscribeAll(OMPNetDirection_IncomingRPC, 0, incomingRPCCallback, nullptr);
		output << "network.subscriptions=" << (outgoingA && outgoingB && incomingAll && incomingRPC) << '\n';

		auto* registration = api.ComponentInterop.RegisterAPI(ParityComponentUID, &exampleAPI.header);
		auto* self = api.ComponentInterop.Find(ParityComponentUID);
		output << "component.find=" << (self != nullptr) << '\n';
		output << "component.valid=" << api.ComponentInterop.IsValid(self) << '\n';
		output << "component.uid=" << std::hex << api.ComponentInterop.GetUID(self) << std::dec << '\n';
		ComponentVersion version {};
		output << "component.version_ok=" << api.ComponentInterop.GetVersion(self, &version) << '\n';
		output << "component.version=" << int(version.major) << '.' << int(version.minor) << '.' << int(version.patch) << '.' << version.prerel << '\n';
		auto* queried = static_cast<const ExampleAPI*>(api.ComponentInterop.QueryAPI(self, ExampleInterfaceUID, 1, sizeof(ExampleAPI)));
		output << "component.register=" << (registration != nullptr) << '\n';
		output << "component.query=" << (queried != nullptr) << '\n';
		output << "component.call=" << (queried ? queried->add(20, 22) : -1) << '\n';
		output << "component.wrong_version=" << (api.ComponentInterop.QueryAPI(self, ExampleInterfaceUID, 2, sizeof(ExampleAPI)) == nullptr) << '\n';
		selfWatch = api.ComponentInterop.Watch(self, componentInvalidated, nullptr);
		output << "component.watch=" << (selfWatch != nullptr) << '\n';

#if defined(_WIN32)
		goLibrary = LIBRARY_OPEN("./capi-go-interop.dll");
#else
		goLibrary = LIBRARY_OPEN("./capi-go-interop.so");
#endif
		if (goLibrary)
		{
			auto goQuery = reinterpret_cast<InteropGoQuery>(LIBRARY_GET_ADDR(goLibrary, "InteropGoQuery"));
			auto goAPI = reinterpret_cast<InteropGoAPI>(LIBRARY_GET_ADDR(goLibrary, "InteropGoAPI"));
			output << "component.go_query_call=" << (goQuery ? goQuery(&api) : -1) << '\n';
			const OMPComponentAPIHeader* goTable = goAPI ? goAPI() : nullptr;
			auto* goRegistration = api.ComponentInterop.RegisterAPI(ParityComponentUID, goTable);
			auto* queriedGo = static_cast<const ExampleAPI*>(api.ComponentInterop.QueryAPI(self, GoInterfaceUID, 1, sizeof(ExampleAPI)));
			output << "component.go_register=" << (goRegistration != nullptr) << '\n';
			output << "component.go_callback=" << (queriedGo ? queriedGo->add(6, 7) : -1) << '\n';
		}
	}

	void onFree(IComponent*) override
	{
	}

	void free() override
	{
		delete this;
	}

	void reset() override
	{
	}
};
}

COMPONENT_ENTRY_POINT()
{
	return new CAPIParityFixture();
}
