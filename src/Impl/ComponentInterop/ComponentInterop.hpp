#pragma once

#include <component.hpp>
#include <ompcapi.h>

#include <memory>
#include <string>
#include <thread>
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

struct OMPCallableRegistration
{
	IComponent* owner = nullptr;
	uint64_t ownerUID = 0;
	OMPCallableCallback callback = nullptr;
	void* userdata = nullptr;
	bool active = false;
	bool invoking = false;
	std::string name;
	std::string documentation;
	std::vector<OMPCallableParameter> parameters;
	std::vector<std::string> parameterNames;
	std::vector<std::vector<uint8_t>> defaultStorage;
	OMPCallableDescriptor descriptor {};
};

struct OMPCallableWatch
{
	OMPCallableRegistration* callable = nullptr;
	OMPCallableInvalidatedCallback callback = nullptr;
	void* userdata = nullptr;
	bool active = false;
	bool notifying = false;
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
	OMPCallableRegistration* registerCallable(uint64_t ownerUID,
		const OMPCallableDescriptor* descriptor, OMPCallableCallback callback, void* userdata);
	bool unregisterCallable(OMPCallableRegistration* callable);
	OMPCallableRegistration* findCallable(OMPComponentHandle* component, CAPIStringView name) const;
	bool callableIsValid(OMPCallableRegistration* callable) const;
	uint32_t callableCount(OMPComponentHandle* component) const;
	OMPCallableRegistration* callableAt(OMPComponentHandle* component, uint32_t index) const;
	const OMPCallableDescriptor* callableDescriptor(OMPCallableRegistration* callable) const;
	bool invokeCallable(OMPCallableRegistration* callable, const OMPCallableValue* args,
		uint32_t count, OMPCallableValue* result, OMPCallableOutputBuffer* output,
		OMPCallableError* error, uint32_t flags);
	OMPCallableWatch* watchCallable(OMPCallableRegistration* callable,
		OMPCallableInvalidatedCallback callback, void* userdata);
	bool unwatchCallable(OMPCallableWatch* watch);

private:
	OMPComponentHandle* knownHandle(OMPComponentHandle* handle) const;
	OMPCallableRegistration* knownCallable(OMPCallableRegistration* callable) const;
	void invalidateCallable(OMPCallableRegistration& callable);
	bool onMainThread() const;
	IComponentList* components_ = nullptr;
	uint64_t nextGeneration_ = 1;
	bool notifying_ = false;
	unsigned invocationDepth_ = 0;
	std::thread::id mainThread_;
	std::vector<std::unique_ptr<OMPComponentHandle>> handles_;
	std::vector<std::unique_ptr<OMPComponentAPIRegistration>> registrations_;
	std::vector<std::unique_ptr<OMPComponentWatch>> watches_;
	std::vector<std::unique_ptr<OMPCallableRegistration>> callables_;
	std::vector<std::unique_ptr<OMPCallableWatch>> callableWatches_;
};
