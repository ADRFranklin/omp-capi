#include <sdk.hpp>
#include <ompcapi.h>

#include <fstream>
#include <string>

namespace
{
OMPAPI_t api {};

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
	PROVIDE_UID(0xD93D4096397AC9D1)

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
