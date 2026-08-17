#pragma once

#include <component.hpp>
#include <ompcapi.h>

#include <memory>
#include <vector>

struct OMPComponentHandle
{
	IComponent* component = nullptr;
	uint64_t uid = 0;
	uint64_t generation = 0;
	bool active = false;
};

struct OMPComponentAPIRegistration
{
	IComponent* owner = nullptr;
	const OMPComponentAPIHeader* table = nullptr;
	uint64_t interfaceUID = 0;
	uint32_t version = 0;
	uint32_t size = 0;
	bool active = false;
};

struct OMPComponentWatch
{
	OMPComponentHandle* handle = nullptr;
	OMPComponentInvalidatedCallback callback = nullptr;
	void* userdata = nullptr;
	bool active = false;
};

class ComponentInteropManager
{
public:
	static ComponentInteropManager& instance();
	void initialize(IComponentList* components);
	void componentUnloading(IComponent* component);
	void shutdown();

	OMPComponentHandle* find(uint64_t uid);
	bool isValid(OMPComponentHandle* handle) const;
	OMPComponentAPIRegistration* registerAPI(uint64_t ownerUID, const OMPComponentAPIHeader* table);
	bool unregisterAPI(OMPComponentAPIRegistration* registration);
	const void* queryAPI(OMPComponentHandle* handle, uint64_t interfaceUID,
		uint32_t version, uint32_t minimumSize) const;
	bool apiIsValid(OMPComponentHandle* handle, const void* table) const;
	OMPComponentWatch* watch(OMPComponentHandle* handle,
		OMPComponentInvalidatedCallback callback, void* userdata);
	bool unwatch(OMPComponentWatch* watch);

private:
	OMPComponentHandle* knownHandle(OMPComponentHandle* handle) const;
	IComponentList* components_ = nullptr;
	uint64_t nextGeneration_ = 1;
	bool notifying_ = false;
	std::vector<std::unique_ptr<OMPComponentHandle>> handles_;
	std::vector<std::unique_ptr<OMPComponentAPIRegistration>> registrations_;
	std::vector<std::unique_ptr<OMPComponentWatch>> watches_;
};
