#include "ComponentInterop.hpp"

#include <algorithm>
#include <cstring>
#include <iterator>
#include <limits>

namespace
{
bool validString(OMPCallableStringView value)
{
	return value.abi_version == OMP_CALLABLE_ABI_VERSION &&
		value.struct_size >= sizeof(OMPCallableStringView) && (value.length == 0 || value.data);
}

bool validBytes(OMPCallableBytesView value)
{
	return value.abi_version == OMP_CALLABLE_ABI_VERSION &&
		value.struct_size >= sizeof(OMPCallableBytesView) && (value.length == 0 || value.data);
}

bool validType(uint32_t type)
{
	return type >= OMPCallableValueType_Null && type <= OMPCallableValueType_Entity;
}

bool validValue(const OMPCallableValue& value)
{
	if (value.abi_version != OMP_CALLABLE_ABI_VERSION ||
		value.struct_size < sizeof(OMPCallableValue) || value.flags != 0 || !validType(value.type)) return false;
	switch (value.type)
	{
	case OMPCallableValueType_Bool: return value.value.boolean <= 1;
	case OMPCallableValueType_String: return validString(value.value.string_value);
	case OMPCallableValueType_Bytes: return validBytes(value.value.bytes_value);
	case OMPCallableValueType_Entity:
		return value.value.entity_value.abi_version == OMP_CALLABLE_ABI_VERSION &&
			value.value.entity_value.struct_size >= sizeof(OMPCallableEntityValue) &&
			value.value.entity_value.entity_type != 0 && value.value.entity_value.reserved == 0;
	default: return true;
	}
}

void setError(OMPCallableError* error, OMPCallableErrorCode code, const char* message)
{
	if (!error || error->abi_version != OMP_CALLABLE_ABI_VERSION ||
		error->struct_size < sizeof(OMPCallableError)) return;
	error->code = code;
	const auto length = static_cast<unsigned>(std::strlen(message));
	error->message.len = length;
	if (!error->message.data || error->message.capacity == 0) return;
	const auto copied = std::min(error->message.capacity, length);
	if (copied) std::memcpy(error->message.data, message, copied);
	if (copied < error->message.capacity) error->message.data[copied] = '\0';
}

bool validError(OMPCallableError* error)
{
	return error && error->abi_version == OMP_CALLABLE_ABI_VERSION &&
		error->struct_size >= sizeof(OMPCallableError) &&
		error->reserved == 0 &&
		(error->message.capacity == 0 || error->message.data);
}

bool validOutput(OMPCallableOutputBuffer* output)
{
	return output && output->abi_version == OMP_CALLABLE_ABI_VERSION &&
		output->struct_size >= sizeof(OMPCallableOutputBuffer) &&
		(output->capacity == 0 || output->data);
}
}

ComponentInteropManager& ComponentInteropManager::instance()
{
	static ComponentInteropManager manager;
	return manager;
}

void ComponentInteropManager::initialize(IComponentList* components)
{
	components_ = components;
	mainThread_ = std::this_thread::get_id();
}

bool ComponentInteropManager::onMainThread() const
{
	return components_ && std::this_thread::get_id() == mainThread_;
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

OMPCallableRegistration* ComponentInteropManager::knownCallable(OMPCallableRegistration* callable) const
{
	if (!callable) return nullptr;
	auto found = std::find_if(callables_.begin(), callables_.end(),
		[callable](const auto& item) { return item.get() == callable; });
	return found == callables_.end() ? nullptr : found->get();
}

OMPCallableRegistration* ComponentInteropManager::registerCallable(uint64_t ownerUID,
	const OMPCallableDescriptor* input, OMPCallableCallback callback, void* userdata)
{
	constexpr uint32_t AllowedCallableFlags = OMPCallableFlag_Deprecated |
		OMPCallableFlag_MainThreadOnly | OMPCallableFlag_MayCallback;
	constexpr uint32_t AllowedParameterFlags = OMPCallableParameterFlag_Optional |
		OMPCallableParameterFlag_HasDefault;
	constexpr uint32_t MaximumParameterCount = 1024;
	if (!onMainThread() || notifying_ || invocationDepth_ || !input || !callback ||
		input->abi_version != OMP_CALLABLE_ABI_VERSION ||
		input->struct_size < sizeof(OMPCallableDescriptor) || !validString(input->name) ||
		input->name.length == 0 || !validString(input->documentation) ||
		!validType(input->return_type) || (input->flags & ~AllowedCallableFlags) ||
		input->parameter_count > MaximumParameterCount ||
		(input->parameter_count && !input->parameters)) return nullptr;
	auto* owner = components_->queryComponent(ownerUID);
	if (!owner) return nullptr;
	std::string name(input->name.data, input->name.length);
	for (const auto& item : callables_)
		if (item->active && item->owner == owner && item->name == name) return nullptr;

	bool optionalSeen = false;
	for (uint32_t i = 0; i < input->parameter_count; ++i)
	{
		const auto& parameter = input->parameters[i];
		if (parameter.abi_version != OMP_CALLABLE_ABI_VERSION ||
			parameter.struct_size < sizeof(OMPCallableParameter) || !validString(parameter.name) ||
			parameter.name.length == 0 || !validType(parameter.type) ||
			(parameter.flags & ~AllowedParameterFlags)) return nullptr;
		const bool optional = parameter.flags & OMPCallableParameterFlag_Optional;
		const bool hasDefault = parameter.flags & OMPCallableParameterFlag_HasDefault;
		if (optionalSeen && !optional) return nullptr;
		if (hasDefault && (!optional || !validValue(parameter.default_value) ||
			parameter.default_value.type != parameter.type)) return nullptr;
		optionalSeen = optionalSeen || optional;
	}

	auto registration = std::make_unique<OMPCallableRegistration>();
	registration->owner = owner; registration->ownerUID = ownerUID; registration->callback = callback;
	registration->userdata = userdata; registration->active = true;
	registration->name = std::move(name);
	if (input->documentation.length)
		registration->documentation.assign(input->documentation.data, input->documentation.length);
	if (input->parameter_count)
		registration->parameters.assign(input->parameters, input->parameters + input->parameter_count);
	registration->parameterNames.resize(input->parameter_count);
	registration->defaultStorage.resize(input->parameter_count);
	for (uint32_t i = 0; i < input->parameter_count; ++i)
	{
		auto& stored = registration->parameters[i];
		const auto& source = input->parameters[i];
		registration->parameterNames[i].assign(source.name.data, source.name.length);
		stored.name.data = registration->parameterNames[i].data();
		if (!(source.flags & OMPCallableParameterFlag_HasDefault)) continue;
		if (source.type == OMPCallableValueType_String)
		{
			auto& bytes = registration->defaultStorage[i];
			if (source.default_value.value.string_value.length)
				bytes.assign(source.default_value.value.string_value.data,
					source.default_value.value.string_value.data + source.default_value.value.string_value.length);
			stored.default_value.value.string_value.data = reinterpret_cast<const char*>(bytes.data());
		}
		else if (source.type == OMPCallableValueType_Bytes)
		{
			auto& bytes = registration->defaultStorage[i];
			if (source.default_value.value.bytes_value.length)
				bytes.assign(source.default_value.value.bytes_value.data,
					source.default_value.value.bytes_value.data + source.default_value.value.bytes_value.length);
			stored.default_value.value.bytes_value.data = bytes.data();
		}
	}
	registration->descriptor = *input;
	registration->descriptor.name.data = registration->name.data();
	registration->descriptor.documentation.data = registration->documentation.data();
	registration->descriptor.parameters = registration->parameters.data();
	auto* result = registration.get();
	callables_.push_back(std::move(registration));
	return result;
}

void ComponentInteropManager::invalidateCallable(OMPCallableRegistration& callable)
{
	if (!callable.active) return;
	callable.active = false;
	const bool wasNotifying = notifying_;
	notifying_ = true;
	for (auto& watch : callableWatches_)
		if (watch->active && watch->callable == &callable) { watch->active = false; watch->notifying = true; }
	for (auto& watch : callableWatches_)
	{
		if (!watch->notifying) continue;
		try { watch->callback(&callable, watch->userdata); } catch (...) { }
		watch->callback = nullptr; watch->userdata = nullptr; watch->callable = nullptr; watch->notifying = false;
	}
	callable.owner = nullptr; callable.callback = nullptr; callable.userdata = nullptr;
	callable.descriptor = {};
	std::string().swap(callable.name); std::string().swap(callable.documentation);
	std::vector<OMPCallableParameter>().swap(callable.parameters);
	std::vector<std::string>().swap(callable.parameterNames);
	std::vector<std::vector<uint8_t>>().swap(callable.defaultStorage);
	notifying_ = wasNotifying;
}

bool ComponentInteropManager::unregisterCallable(OMPCallableRegistration* callable)
{
	if (!onMainThread() || notifying_ || invocationDepth_) return false;
	auto* known = knownCallable(callable);
	if (!known || !known->active) return false;
	invalidateCallable(*known);
	return true;
}

bool ComponentInteropManager::callableIsValid(OMPCallableRegistration* callable) const
{
	if (!onMainThread()) return false;
	auto* known = knownCallable(callable);
	return known && known->active && components_->queryComponent(known->ownerUID) == known->owner;
}

OMPCallableRegistration* ComponentInteropManager::findCallable(OMPComponentHandle* component, CAPIStringView name) const
{
	if (!onMainThread() || !isValid(component) || (name.len && !name.data)) return nullptr;
	for (const auto& item : callables_)
		if (item->active && item->owner == component->component && item->name.size() == name.len &&
			(name.len == 0 || std::memcmp(item->name.data(), name.data, name.len) == 0)) return item.get();
	return nullptr;
}

uint32_t ComponentInteropManager::callableCount(OMPComponentHandle* component) const
{
	if (!onMainThread() || !isValid(component)) return 0;
	uint32_t count = 0;
	for (const auto& item : callables_)
		if (item->active && item->owner == component->component && count != std::numeric_limits<uint32_t>::max()) ++count;
	return count;
}

OMPCallableRegistration* ComponentInteropManager::callableAt(OMPComponentHandle* component, uint32_t index) const
{
	if (!onMainThread() || !isValid(component)) return nullptr;
	uint32_t current = 0;
	for (const auto& item : callables_)
		if (item->active && item->owner == component->component && current++ == index) return item.get();
	return nullptr;
}

const OMPCallableDescriptor* ComponentInteropManager::callableDescriptor(OMPCallableRegistration* callable) const
{
	if (!onMainThread()) return nullptr;
	auto* known = knownCallable(callable);
	return callableIsValid(known) ? &known->descriptor : nullptr;
}

bool ComponentInteropManager::invokeCallable(OMPCallableRegistration* callable,
	const OMPCallableValue* args, uint32_t count, OMPCallableValue* result,
	OMPCallableOutputBuffer* output, OMPCallableError* error, uint32_t flags)
{
	if (!validError(error)) return false;
	error->code = OMPCallableError_None; error->message.len = 0;
	if (!onMainThread()) { setError(error, OMPCallableError_InternalFailure, "callable APIs require the main thread"); return false; }
	if (invocationDepth_) { setError(error, OMPCallableError_Busy, "recursive invocation is not supported"); return false; }
	if (flags != 0) { setError(error, OMPCallableError_InvalidArgument, "unsupported invocation flags"); return false; }
	if (!callable) { setError(error, OMPCallableError_NotFound, "callable not found"); return false; }
	auto* known = knownCallable(callable);
	if (!known || !known->active) { setError(error, OMPCallableError_InvalidHandle, "invalid or stale callable handle"); return false; }
	if (!components_ || components_->queryComponent(known->ownerUID) != known->owner)
	{
		setError(error, OMPCallableError_ComponentUnavailable, "owning component is unavailable"); return false;
	}
	if (!result || result->abi_version != OMP_CALLABLE_ABI_VERSION ||
		result->struct_size < sizeof(OMPCallableValue) || result->flags != 0)
	{
		setError(error, OMPCallableError_InvalidArgument, "invalid result value"); return false;
	}
	uint32_t required = known->descriptor.parameter_count;
	while (required && (known->parameters[required - 1].flags & OMPCallableParameterFlag_Optional)) --required;
	if (count < required || count > known->descriptor.parameter_count || (count && !args))
	{
		setError(error, OMPCallableError_ArgumentCount, "argument count mismatch"); return false;
	}
	for (uint32_t i = 0; i < count; ++i)
	{
		if (!validValue(args[i])) { setError(error, OMPCallableError_InvalidArgument, "invalid argument value"); return false; }
		if (args[i].type != known->parameters[i].type)
		{
			setError(error, OMPCallableError_ArgumentType, "argument type mismatch"); return false;
		}
	}
	if (output && !validOutput(output))
	{
		setError(error, OMPCallableError_InvalidArgument, "invalid output buffer"); return false;
	}
	if ((known->descriptor.return_type == OMPCallableValueType_String ||
		known->descriptor.return_type == OMPCallableValueType_Bytes) && !output)
	{
		setError(error, OMPCallableError_InvalidArgument, "string and byte results require an output buffer"); return false;
	}

	OMPCallableValue callbackResult {};
	callbackResult.abi_version = OMP_CALLABLE_ABI_VERSION;
	callbackResult.struct_size = sizeof(OMPCallableValue);
	OMPCallableOutputBuffer callbackOutput {};
	if (output) { callbackOutput = *output; callbackOutput.length = 0; output->length = 0; }
	OMPCallableError callbackError = *error;
	callbackError.code = OMPCallableError_None; callbackError.message.len = 0;
	OMPCallableContext context { OMP_CALLABLE_ABI_VERSION, sizeof(OMPCallableContext),
		args, count, &callbackResult, output ? &callbackOutput : nullptr, &callbackError,
		flags, { 0, 0, 0, 0 } };
	bool accepted = false;
	bool threw = false;
	known->invoking = true; ++invocationDepth_;
	try { accepted = known->callback(&context, known->userdata); }
	catch (...) { threw = true; }
	--invocationDepth_; known->invoking = false;
	if (threw)
	{
		setError(error, OMPCallableError_InternalFailure, "callable threw an exception"); return false;
	}
	const bool contextValid = context.abi_version == OMP_CALLABLE_ABI_VERSION &&
		context.struct_size >= sizeof(OMPCallableContext) && context.arguments == args &&
		context.argument_count == count && context.result == &callbackResult &&
		context.output == (output ? &callbackOutput : nullptr) && context.error == &callbackError &&
		context.flags == flags && std::all_of(std::begin(context.reserved), std::end(context.reserved),
			[](uint32_t item) { return item == 0; });
	const bool errorValid = callbackError.abi_version == OMP_CALLABLE_ABI_VERSION &&
		callbackError.struct_size >= sizeof(OMPCallableError) && callbackError.reserved == 0 &&
		callbackError.message.data == error->message.data &&
		callbackError.message.capacity == error->message.capacity;
	const bool outputValid = !output || (callbackOutput.abi_version == OMP_CALLABLE_ABI_VERSION &&
		callbackOutput.struct_size >= sizeof(OMPCallableOutputBuffer) &&
		callbackOutput.data == output->data && callbackOutput.capacity == output->capacity);
	if (!contextValid || !errorValid || !outputValid)
	{
		setError(error, OMPCallableError_InternalFailure, "callable modified protected invocation state"); return false;
	}
	if (output) output->length = callbackOutput.length;
	if (!accepted)
	{
		if (output && callbackOutput.length > callbackOutput.capacity)
			setError(error, OMPCallableError_OutputTooSmall, "output buffer is too small");
		else if (callbackError.code == OMPCallableError_None)
			setError(error, OMPCallableError_Rejected, "callable rejected the request");
		else if (callbackError.code > OMPCallableError_OutputTooSmall)
			setError(error, OMPCallableError_InternalFailure, "callable returned an invalid error code");
		else
		{
			error->code = callbackError.code;
			error->message.len = callbackError.message.len;
		}
		return false;
	}
	if (callbackError.code != OMPCallableError_None)
	{
		setError(error, OMPCallableError_InternalFailure, "successful callable returned an error"); return false;
	}
	if (callbackResult.type != known->descriptor.return_type || !validValue(callbackResult))
	{
		setError(error, OMPCallableError_InternalFailure, "callable returned an invalid value"); return false;
	}
	if (callbackResult.type == OMPCallableValueType_String)
	{
		if (!output || callbackResult.value.string_value.data != reinterpret_cast<const char*>(output->data) ||
			callbackResult.value.string_value.length > output->capacity ||
			callbackResult.value.string_value.length != output->length)
		{
			setError(error, OMPCallableError_InternalFailure, "callable returned invalid string storage"); return false;
		}
	}
	else if (callbackResult.type == OMPCallableValueType_Bytes)
	{
		if (!output || callbackResult.value.bytes_value.data != output->data ||
			callbackResult.value.bytes_value.length > output->capacity ||
			callbackResult.value.bytes_value.length != output->length)
		{
			setError(error, OMPCallableError_InternalFailure, "callable returned invalid byte storage"); return false;
		}
	}
	*result = callbackResult;
	return true;
}

OMPCallableWatch* ComponentInteropManager::watchCallable(OMPCallableRegistration* callable,
	OMPCallableInvalidatedCallback callback, void* userdata)
{
	if (!onMainThread() || notifying_ || invocationDepth_ || !callableIsValid(callable) || !callback) return nullptr;
	auto watch = std::make_unique<OMPCallableWatch>();
	watch->callable = callable; watch->callback = callback; watch->userdata = userdata; watch->active = true;
	auto* result = watch.get(); callableWatches_.push_back(std::move(watch)); return result;
}

bool ComponentInteropManager::unwatchCallable(OMPCallableWatch* watch)
{
	if (!onMainThread() || notifying_ || invocationDepth_ || !watch) return false;
	auto found = std::find_if(callableWatches_.begin(), callableWatches_.end(),
		[watch](const auto& item) { return item.get() == watch; });
	if (found == callableWatches_.end() || !watch->active) return false;
	watch->active = false; watch->callable = nullptr; watch->callback = nullptr; watch->userdata = nullptr;
	return true;
}

void ComponentInteropManager::componentUnloading(IComponent* component)
{
	if (!component || notifying_) return;
	notifying_ = true;
	for (auto& callable : callables_)
		if (callable->owner == component) invalidateCallable(*callable);
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
	notifying_ = true;
	for (auto& callable : callables_) invalidateCallable(*callable);
	for (auto& watch : callableWatches_) watch->active = false;
	for (auto& handle : handles_) handle->active = false;
	for (auto& registration : registrations_) registration->active = false;
	for (auto& watch : watches_) watch->active = false;
	components_ = nullptr; notifying_ = false; invocationDepth_ = 0;
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
extern "C" OMP_API_EXPORT OMPCallableRegistration* Component_RegisterCallable(uint64_t ownerUID,
	const OMPCallableDescriptor* descriptor, OMPCallableCallback callback, void* userdata)
{ try { return ComponentInteropManager::instance().registerCallable(ownerUID, descriptor, callback, userdata); } catch (...) { return nullptr; } }
extern "C" OMP_API_EXPORT bool Component_UnregisterCallable(OMPCallableRegistration* callable)
{ try { return ComponentInteropManager::instance().unregisterCallable(callable); } catch (...) { return false; } }
extern "C" OMP_API_EXPORT OMPCallableRegistration* Component_FindCallable(OMPComponentHandle* component, CAPIStringView name)
{ try { return ComponentInteropManager::instance().findCallable(component, name); } catch (...) { return nullptr; } }
extern "C" OMP_API_EXPORT bool Component_CallableIsValid(OMPCallableRegistration* callable)
{ try { return ComponentInteropManager::instance().callableIsValid(callable); } catch (...) { return false; } }
extern "C" OMP_API_EXPORT uint32_t Component_GetCallableCount(OMPComponentHandle* component)
{ try { return ComponentInteropManager::instance().callableCount(component); } catch (...) { return 0; } }
extern "C" OMP_API_EXPORT OMPCallableRegistration* Component_GetCallableAt(OMPComponentHandle* component, uint32_t index)
{ try { return ComponentInteropManager::instance().callableAt(component, index); } catch (...) { return nullptr; } }
extern "C" OMP_API_EXPORT const OMPCallableDescriptor* Component_GetCallableDescriptor(OMPCallableRegistration* callable)
{ try { return ComponentInteropManager::instance().callableDescriptor(callable); } catch (...) { return nullptr; } }
extern "C" OMP_API_EXPORT bool Component_InvokeCallable(OMPCallableRegistration* callable,
	const OMPCallableValue* args, uint32_t count, OMPCallableValue* result,
	OMPCallableOutputBuffer* output, OMPCallableError* error, uint32_t flags)
{ try { return ComponentInteropManager::instance().invokeCallable(callable, args, count, result, output, error, flags); }
	catch (...) { setError(error, OMPCallableError_InternalFailure, "internal invocation failure"); return false; } }
extern "C" OMP_API_EXPORT OMPCallableWatch* Component_WatchCallable(OMPCallableRegistration* callable,
	OMPCallableInvalidatedCallback callback, void* userdata)
{ try { return ComponentInteropManager::instance().watchCallable(callable, callback, userdata); } catch (...) { return nullptr; } }
extern "C" OMP_API_EXPORT bool Component_UnwatchCallable(OMPCallableWatch* watch)
{ try { return ComponentInteropManager::instance().unwatchCallable(watch); } catch (...) { return false; } }
