#include "ComponentInterop.hpp"

#include <algorithm>
#include <cstring>

ComponentInteropManager& ComponentInteropManager::instance()
{
	static ComponentInteropManager manager;
	return manager;
}

void ComponentInteropManager::initialize(IComponentList* components)
{
	components_ = components;
}

OMPComponentHandle* ComponentInteropManager::knownHandle(OMPComponentHandle* handle) const
{
	if (!handle) return nullptr;
	auto found = std::find_if(handles_.begin(), handles_.end(),
		[handle](const auto& item) { return item.get() == handle; });
	return found == handles_.end() ? nullptr : found->get();
}

OMPComponentHandle* ComponentInteropManager::find(uint64_t uid)
{
	if (!components_ || notifying_) return nullptr;
	IComponent* component = components_->queryComponent(uid);
	if (!component) return nullptr;
	for (auto& handle : handles_)
		if (handle->active && handle->component == component) return handle.get();
	auto handle = std::make_unique<OMPComponentHandle>();
	handle->component = component; handle->uid = uid;
	handle->generation = nextGeneration_++; handle->active = true;
	auto* result = handle.get(); handles_.push_back(std::move(handle));
	return result;
}

bool ComponentInteropManager::isValid(OMPComponentHandle* handle) const
{
	auto* known = knownHandle(handle);
	return known && known->active && components_ && components_->queryComponent(known->uid) == known->component;
}

OMPComponentAPIRegistration* ComponentInteropManager::registerAPI(uint64_t ownerUID,
	const OMPComponentAPIHeader* table)
{
	if (!components_ || notifying_ || !table || table->interface_uid == 0 || table->abi_version == 0 ||
		table->struct_size < sizeof(OMPComponentAPIHeader)) return nullptr;
	IComponent* owner = components_->queryComponent(ownerUID);
	if (!owner) return nullptr;
	for (const auto& item : registrations_)
		if (item->active && item->owner == owner && item->interfaceUID == table->interface_uid &&
			item->version == table->abi_version) return nullptr;
	auto registration = std::make_unique<OMPComponentAPIRegistration>();
	registration->owner = owner; registration->table = table;
	registration->interfaceUID = table->interface_uid;
	registration->version = table->abi_version; registration->size = table->struct_size;
	registration->active = true;
	auto* result = registration.get(); registrations_.push_back(std::move(registration));
	return result;
}

bool ComponentInteropManager::unregisterAPI(OMPComponentAPIRegistration* registration)
{
	if (!registration) return false;
	auto found = std::find_if(registrations_.begin(), registrations_.end(),
		[registration](const auto& item) { return item.get() == registration; });
	if (found == registrations_.end() || !registration->active) return false;
	registration->active = false;
	return true;
}

const void* ComponentInteropManager::queryAPI(OMPComponentHandle* handle, uint64_t interfaceUID,
	uint32_t version, uint32_t minimumSize) const
{
	if (!isValid(handle) || interfaceUID == 0 || version == 0 ||
		minimumSize < sizeof(OMPComponentAPIHeader)) return nullptr;
	for (const auto& item : registrations_)
		if (item->active && item->owner == handle->component && item->interfaceUID == interfaceUID &&
			item->version == version && item->size >= minimumSize) return item->table;
	return nullptr;
}

bool ComponentInteropManager::apiIsValid(OMPComponentHandle* handle, const void* table) const
{
	if (!isValid(handle) || !table) return false;
	for (const auto& item : registrations_)
		if (item->active && item->owner == handle->component && item->table == table) return true;
	return false;
}

OMPComponentWatch* ComponentInteropManager::watch(OMPComponentHandle* handle,
	OMPComponentInvalidatedCallback callback, void* userdata)
{
	if (notifying_ || !isValid(handle) || !callback) return nullptr;
	auto watch = std::make_unique<OMPComponentWatch>();
	watch->handle = handle; watch->callback = callback; watch->userdata = userdata; watch->active = true;
	auto* result = watch.get(); watches_.push_back(std::move(watch));
	return result;
}

bool ComponentInteropManager::unwatch(OMPComponentWatch* watch)
{
	if (!watch) return false;
	auto found = std::find_if(watches_.begin(), watches_.end(),
		[watch](const auto& item) { return item.get() == watch; });
	if (found == watches_.end() || !watch->active) return false;
	watch->active = false;
	return true;
}

void ComponentInteropManager::componentUnloading(IComponent* component)
{
	if (!component || notifying_) return;
	notifying_ = true;
	for (auto& registration : registrations_)
		if (registration->owner == component) registration->active = false;
	for (auto& handle : handles_)
	{
		if (!handle->active || handle->component != component) continue;
		handle->active = false;
		for (auto& watch : watches_)
		{
			if (!watch->active || watch->handle != handle.get()) continue;
			try { watch->callback(handle.get(), watch->userdata); } catch (...) { }
			watch->active = false;
		}
	}
	notifying_ = false;
}

void ComponentInteropManager::shutdown()
{
	for (auto& handle : handles_) handle->active = false;
	for (auto& registration : registrations_) registration->active = false;
	for (auto& watch : watches_) watch->active = false;
	components_ = nullptr; notifying_ = false;
	handles_.clear(); registrations_.clear(); watches_.clear(); nextGeneration_ = 1;
}

extern "C" OMP_API_EXPORT OMPComponentHandle* Component_Find(uint64_t uid) { return ComponentInteropManager::instance().find(uid); }
extern "C" OMP_API_EXPORT bool Component_IsValid(OMPComponentHandle* handle) { return ComponentInteropManager::instance().isValid(handle); }
extern "C" OMP_API_EXPORT uint64_t Component_GetUID(OMPComponentHandle* handle) { return ComponentInteropManager::instance().isValid(handle) ? handle->uid : 0; }
extern "C" OMP_API_EXPORT int Component_GetName(OMPComponentHandle* handle, CAPIStringBuffer* output)
{
	if (!ComponentInteropManager::instance().isValid(handle)) return 0;
	auto name = handle->component->componentName(); const auto length = static_cast<unsigned>(name.length());
	if (output) { output->len = length; if (output->data && output->capacity >= length) {
		if (length) std::memcpy(output->data, name.data(), length); if (output->capacity > length) output->data[length] = '\0'; } }
	return static_cast<int>(length);
}
extern "C" OMP_API_EXPORT bool Component_GetVersion(OMPComponentHandle* handle, ComponentVersion* output)
{
	if (!output || !ComponentInteropManager::instance().isValid(handle)) return false;
	auto version = handle->component->componentVersion();
	output->major = version.major; output->minor = version.minor; output->patch = version.patch; output->prerel = version.prerel;
	return true;
}
extern "C" OMP_API_EXPORT int32_t Component_GetType(OMPComponentHandle* handle)
{
	return ComponentInteropManager::instance().isValid(handle) ? handle->component->componentType() : -1;
}
extern "C" OMP_API_EXPORT OMPComponentAPIRegistration* Component_RegisterAPI(uint64_t ownerUID, const OMPComponentAPIHeader* table)
{ return ComponentInteropManager::instance().registerAPI(ownerUID, table); }
extern "C" OMP_API_EXPORT bool Component_UnregisterAPI(OMPComponentAPIRegistration* registration)
{ return ComponentInteropManager::instance().unregisterAPI(registration); }
extern "C" OMP_API_EXPORT const void* Component_QueryAPI(OMPComponentHandle* handle, uint64_t uid, uint32_t version, uint32_t size)
{ return ComponentInteropManager::instance().queryAPI(handle, uid, version, size); }
extern "C" OMP_API_EXPORT bool Component_APIIsValid(OMPComponentHandle* handle, const void* table)
{ return ComponentInteropManager::instance().apiIsValid(handle, table); }
extern "C" OMP_API_EXPORT OMPComponentWatch* Component_Watch(OMPComponentHandle* handle, OMPComponentInvalidatedCallback callback, void* userdata)
{ return ComponentInteropManager::instance().watch(handle, callback, userdata); }
extern "C" OMP_API_EXPORT bool Component_Unwatch(OMPComponentWatch* watch)
{ return ComponentInteropManager::instance().unwatch(watch); }
