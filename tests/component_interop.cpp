#include "Impl/ComponentInterop/ComponentInterop.hpp"

#include <cassert>
#include <unordered_map>

namespace
{
constexpr uint64_t ComponentUID = 0x1122334455667788ULL;
constexpr uint64_t InterfaceUID = 0x8877665544332211ULL;

struct ExampleAPI
{
	OMPComponentAPIHeader header;
	int32_t (*add)(int32_t, int32_t);
};

int32_t add(int32_t left, int32_t right) { return left + right; }

struct FakeComponent final : IComponent
{
	UID getUID() override { return ComponentUID; }
	StringView componentName() const override { return "example"; }
	SemanticVersion componentVersion() const override { return { 2, 3, 4, 5 }; }
	void onLoad(ICore*) override {}
	void free() override {}
	void reset() override {}
};

struct FakeList final : IComponentList
{
	IComponent* component = nullptr;
	IComponent* queryComponent(UID uid) override
	{
		return component && component->getUID() == uid ? component : nullptr;
	}
};

void invalidated(OMPComponentHandle* handle, void* userdata)
{
	++*static_cast<int*>(userdata);
	assert(!ComponentInteropManager::instance().isValid(handle));
	assert(ComponentInteropManager::instance().find(ComponentUID) == nullptr);
}

void throwingInvalidation(OMPComponentHandle*, void*) { throw 1; }
}

int main()
{
	FakeComponent component;
	FakeList list; list.component = &component;
	auto& manager = ComponentInteropManager::instance();
	manager.initialize(&list);

	assert(manager.find(0) == nullptr);
	auto* handle = manager.find(ComponentUID);
	assert(handle && manager.find(ComponentUID) == handle && manager.isValid(handle));
	assert(!manager.isValid(reinterpret_cast<OMPComponentHandle*>(uintptr_t(1))));

	ExampleAPI api { { InterfaceUID, 1, sizeof(ExampleAPI) }, add };
	assert(manager.registerAPI(ComponentUID, nullptr) == nullptr);
	ExampleAPI invalidUID { { 0, 1, sizeof(ExampleAPI) }, add };
	ExampleAPI invalidVersion { { InterfaceUID, 0, sizeof(ExampleAPI) }, add };
	ExampleAPI invalidSize { { InterfaceUID, 1, sizeof(OMPComponentAPIHeader) - 1 }, add };
	assert(manager.registerAPI(ComponentUID, &invalidUID.header) == nullptr);
	assert(manager.registerAPI(ComponentUID, &invalidVersion.header) == nullptr);
	assert(manager.registerAPI(ComponentUID, &invalidSize.header) == nullptr);
	auto* registration = manager.registerAPI(ComponentUID, &api.header);
	assert(registration);
	assert(manager.registerAPI(ComponentUID, &api.header) == nullptr);
	assert(manager.queryAPI(handle, InterfaceUID, 2, sizeof(ExampleAPI)) == nullptr);
	assert(manager.queryAPI(handle, InterfaceUID, 1, sizeof(ExampleAPI) + 1) == nullptr);
	auto* queried = static_cast<const ExampleAPI*>(manager.queryAPI(handle, InterfaceUID, 1, sizeof(ExampleAPI)));
	assert(queried && queried->add(20, 22) == 42);
	assert(manager.apiIsValid(handle, queried));

	int invalidations = 0;
	assert(manager.watch(handle, throwingInvalidation, nullptr));
	auto* watch = manager.watch(handle, invalidated, &invalidations);
	assert(watch);
	assert(manager.watch(handle, nullptr, nullptr) == nullptr);
	assert(!manager.unwatch(reinterpret_cast<OMPComponentWatch*>(uintptr_t(1))));
	manager.componentUnloading(&component);
	assert(invalidations == 1 && !manager.isValid(handle));
	assert(!manager.apiIsValid(handle, queried));
	assert(manager.queryAPI(handle, InterfaceUID, 1, sizeof(ExampleAPI)) == nullptr);
	assert(!manager.unwatch(watch));
	assert(!manager.unregisterAPI(registration));

	list.component = nullptr;
	assert(manager.find(ComponentUID) == nullptr);
	manager.shutdown();
	return 0;
}
