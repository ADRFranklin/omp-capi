#include "Impl/ComponentInterop/ComponentInterop.hpp"

#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <stdexcept>
#include <thread>

#define CHECK(expression) do { if (!(expression)) { \
	std::fprintf(stderr, "check failed at %s:%d: %s\n", __FILE__, __LINE__, #expression); \
	std::abort(); \
} } while (false)

namespace
{
constexpr uint64_t OwnerUID = 0xAABBCCDDEEFF0011ULL;

OMPCallableStringView stringView(const char* text)
{
	return { OMP_CALLABLE_ABI_VERSION, sizeof(OMPCallableStringView), text,
		static_cast<uint32_t>(text ? std::strlen(text) : 0) };
}

CAPIStringView capiStringView(const char* text)
{
	return { static_cast<unsigned>(text ? std::strlen(text) : 0), text };
}

OMPCallableValue value(OMPCallableValueType type)
{
	OMPCallableValue result {};
	result.abi_version = OMP_CALLABLE_ABI_VERSION;
	result.struct_size = sizeof(result);
	result.type = type;
	return result;
}

OMPCallableDescriptor descriptor(const char* name, OMPCallableParameter* params,
	uint32_t count, OMPCallableValueType resultType)
{
	return { OMP_CALLABLE_ABI_VERSION, sizeof(OMPCallableDescriptor), stringView(name),
		stringView("test callable"), count, params, static_cast<uint32_t>(resultType),
		OMPCallableFlag_MainThreadOnly };
}

struct FakeComponent final : IComponent
{
	UID getUID() override { return OwnerUID; }
	StringView componentName() const override { return "callable-provider"; }
	SemanticVersion componentVersion() const override { return { 1, 0, 0, 0 }; }
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

bool add(OMPCallableContext* context, void*)
{
	context->result->type = OMPCallableValueType_Int64;
	context->result->value.int64_value = context->arguments[0].value.int64_value +
		context->arguments[1].value.int64_value;
	return true;
}

bool greet(OMPCallableContext* context, void*)
{
	const char* defaultPrefix = "Hello, ";
	auto prefix = context->argument_count == 2 ? context->arguments[1].value.string_value : stringView(defaultPrefix);
	auto name = context->arguments[0].value.string_value;
	const uint32_t required = prefix.length + name.length;
	context->output->length = required;
	if (context->output->capacity < required) return false;
	std::memcpy(context->output->data, prefix.data, prefix.length);
	std::memcpy(context->output->data + prefix.length, name.data, name.length);
	context->result->type = OMPCallableValueType_String;
	context->result->value.string_value = { OMP_CALLABLE_ABI_VERSION,
		sizeof(OMPCallableStringView), reinterpret_cast<const char*>(context->output->data), required };
	return true;
}

bool inspectBytes(OMPCallableContext* context, void*)
{
	context->result->type = OMPCallableValueType_UInt64;
	context->result->value.uint64_value = context->arguments[0].value.bytes_value.length;
	return true;
}

bool returnBytes(OMPCallableContext* context, void*)
{
	static constexpr uint8_t bytes[] { 9, 8, 7 };
	context->output->length = sizeof(bytes);
	if (context->output->capacity < sizeof(bytes)) return false;
	std::memcpy(context->output->data, bytes, sizeof(bytes));
	context->result->type = OMPCallableValueType_Bytes;
	context->result->value.bytes_value = { OMP_CALLABLE_ABI_VERSION,
		sizeof(OMPCallableBytesView), context->output->data, sizeof(bytes) };
	return true;
}

bool tamperOutput(OMPCallableContext* context, void*)
{
	static uint8_t providerStorage[] { 'b', 'a', 'd' };
	context->output->data = providerStorage;
	context->output->capacity = sizeof(providerStorage);
	context->output->length = sizeof(providerStorage);
	context->result->type = OMPCallableValueType_String;
	context->result->value.string_value = { OMP_CALLABLE_ABI_VERSION,
		sizeof(OMPCallableStringView), reinterpret_cast<const char*>(providerStorage), sizeof(providerStorage) };
	return true;
}

bool tamperError(OMPCallableContext* context, void*)
{
	context->error->message.data = reinterpret_cast<char*>(uintptr_t(1));
	context->error->code = OMPCallableError_Rejected;
	return false;
}

bool echo(OMPCallableContext* context, void*)
{
	*context->result = context->arguments[0];
	return true;
}

bool reject(OMPCallableContext* context, void*)
{
	context->error->code = OMPCallableError_Rejected;
	const char message[] = "provider rejected request";
	context->error->message.len = sizeof(message) - 1;
	if (context->error->message.data && context->error->message.capacity >= sizeof(message) - 1)
		std::memcpy(context->error->message.data, message, sizeof(message) - 1);
	return false;
}

bool throwing(OMPCallableContext*, void*) { throw std::runtime_error("boom"); }
void noopInvalidation(OMPCallableRegistration*, void*);

struct MutationState
{
	OMPCallableRegistration* callable = nullptr;
	bool unregisterResult = true;
	bool recursiveResult = true;
	bool watchResult = true;
	bool registerResult = true;
	uint32_t recursiveCode = OMPCallableError_None;
};

bool mutate(OMPCallableContext* context, void* userdata)
{
	auto& state = *static_cast<MutationState*>(userdata);
	state.unregisterResult = Component_UnregisterCallable(state.callable);
	state.watchResult = Component_WatchCallable(state.callable, noopInvalidation, nullptr) != nullptr;
	auto nestedDescriptor = descriptor("registeredDuringInvocation", nullptr, 0, OMPCallableValueType_Null);
	state.registerResult = Component_RegisterCallable(OwnerUID, &nestedDescriptor, echo, nullptr) != nullptr;
	OMPCallableValue nestedResult = value(OMPCallableValueType_Null);
	char message[64] {};
	OMPCallableError error { OMP_CALLABLE_ABI_VERSION, sizeof(OMPCallableError),
		OMPCallableError_None, 0, { sizeof(message), 0, message } };
	state.recursiveResult = Component_InvokeCallable(state.callable, nullptr, 0,
		&nestedResult, nullptr, &error, 0);
	state.recursiveCode = error.code;
	context->result->type = OMPCallableValueType_Null;
	return true;
}

int invalidations = 0;
void invalidated(OMPCallableRegistration*, void*) { ++invalidations; }
void throwingCallableInvalidation(OMPCallableRegistration*, void*) { throw 1; }
void noopInvalidation(OMPCallableRegistration*, void*) {}

struct InvalidationMutationState
{
	OMPCallableRegistration* other = nullptr;
	unsigned accepted = 0;
};

void mutateDuringInvalidation(OMPCallableRegistration*, void* userdata)
{
	auto& state = *static_cast<InvalidationMutationState*>(userdata);
	for (unsigned i = 0; i < 128; ++i)
		state.accepted += Component_WatchCallable(state.other, noopInvalidation, nullptr) != nullptr;
}

bool invoke(OMPCallableRegistration* callable, const OMPCallableValue* args, uint32_t count,
	OMPCallableValue& result, OMPCallableError& error, OMPCallableOutputBuffer* output = nullptr)
{
	error.code = OMPCallableError_None;
	error.message.len = 0;
	return Component_InvokeCallable(callable, args, count, &result, output, &error, 0);
}
}

int main()
{
	FakeComponent component;
	FakeList list; list.component = &component;
	auto& manager = ComponentInteropManager::instance();
	manager.initialize(&list);
	auto* owner = Component_Find(OwnerUID);
	CHECK(owner);

	OMPCallableParameter addParams[2] {};
	for (auto& param : addParams) { param.abi_version = OMP_CALLABLE_ABI_VERSION; param.struct_size = sizeof(param); param.type = OMPCallableValueType_Int64; }
	addParams[0].name = stringView("left"); addParams[1].name = stringView("right");
	auto addDescriptor = descriptor("add", addParams, 2, OMPCallableValueType_Int64);
	CHECK(Component_RegisterCallable(0, &addDescriptor, add, nullptr) == nullptr);
	CHECK(Component_RegisterCallable(OwnerUID, nullptr, add, nullptr) == nullptr);
	auto invalidDescriptor = addDescriptor; invalidDescriptor.abi_version = 0;
	CHECK(Component_RegisterCallable(OwnerUID, &invalidDescriptor, add, nullptr) == nullptr);
	invalidDescriptor = addDescriptor; invalidDescriptor.struct_size = sizeof(OMPCallableDescriptor) - 1;
	CHECK(Component_RegisterCallable(OwnerUID, &invalidDescriptor, add, nullptr) == nullptr);
	CHECK(Component_RegisterCallable(OwnerUID, &addDescriptor, nullptr, nullptr) == nullptr);
	auto invalidParameter = addParams[0]; invalidParameter.type = static_cast<OMPCallableValueType>(999);
	invalidDescriptor = descriptor("invalidType", &invalidParameter, 1, OMPCallableValueType_Null);
	CHECK(Component_RegisterCallable(OwnerUID, &invalidDescriptor, add, nullptr) == nullptr);
	OMPCallableParameter invalidOrder[2] { addParams[0], addParams[1] };
	invalidOrder[0].flags = OMPCallableParameterFlag_Optional;
	invalidDescriptor = descriptor("invalidOrder", invalidOrder, 2, OMPCallableValueType_Int64);
	CHECK(Component_RegisterCallable(OwnerUID, &invalidDescriptor, add, nullptr) == nullptr);
	invalidParameter = addParams[0]; invalidParameter.abi_version = 0;
	invalidDescriptor = descriptor("invalidParameterVersion", &invalidParameter, 1, OMPCallableValueType_Null);
	CHECK(Component_RegisterCallable(OwnerUID, &invalidDescriptor, add, nullptr) == nullptr);
	invalidParameter = addParams[0]; invalidParameter.struct_size = sizeof(OMPCallableParameter) - 1;
	invalidDescriptor = descriptor("invalidParameterSize", &invalidParameter, 1, OMPCallableValueType_Null);
	CHECK(Component_RegisterCallable(OwnerUID, &invalidDescriptor, add, nullptr) == nullptr);
	invalidDescriptor = addDescriptor; invalidDescriptor.return_type = static_cast<OMPCallableValueType>(999);
	CHECK(Component_RegisterCallable(OwnerUID, &invalidDescriptor, add, nullptr) == nullptr);
	invalidDescriptor = addDescriptor; invalidDescriptor.flags = UINT32_MAX;
	CHECK(Component_RegisterCallable(OwnerUID, &invalidDescriptor, add, nullptr) == nullptr);
	invalidDescriptor = addDescriptor; invalidDescriptor.parameter_count = UINT32_MAX;
	CHECK(Component_RegisterCallable(OwnerUID, &invalidDescriptor, add, nullptr) == nullptr);
	auto* addCallable = Component_RegisterCallable(OwnerUID, &addDescriptor, add, nullptr);
	CHECK(addCallable && Component_CallableIsValid(addCallable));
	CHECK(Component_RegisterCallable(OwnerUID, &addDescriptor, add, nullptr) == nullptr);
	CHECK(Component_GetCallableCount(owner) == 1);
	CHECK(Component_GetCallableAt(owner, 0) == addCallable);
	CHECK(Component_GetCallableAt(owner, 1) == nullptr);
	CHECK(Component_FindCallable(owner, capiStringView("add")) == addCallable);
	CHECK(Component_FindCallable(owner, capiStringView("missing")) == nullptr);
	auto* copied = Component_GetCallableDescriptor(addCallable);
	CHECK(copied && copied != &addDescriptor && copied->parameter_count == 2);
	char copiedName[] = "copied";
	char copiedDocumentation[] = "copied documentation";
	char copiedParameterName[] = "text";
	char copiedDefault[] = "fallback";
	OMPCallableParameter copiedParameter { OMP_CALLABLE_ABI_VERSION, sizeof(OMPCallableParameter),
		stringView(copiedParameterName), OMPCallableValueType_String,
		OMPCallableParameterFlag_Optional | OMPCallableParameterFlag_HasDefault, value(OMPCallableValueType_String) };
	copiedParameter.default_value.value.string_value = stringView(copiedDefault);
	auto copiedDescriptor = descriptor(copiedName, &copiedParameter, 1, OMPCallableValueType_Null);
	copiedDescriptor.documentation = stringView(copiedDocumentation);
	auto* copiedCallable = Component_RegisterCallable(OwnerUID, &copiedDescriptor, echo, nullptr);
	CHECK(copiedCallable);
	std::memset(copiedName, 'x', sizeof(copiedName) - 1);
	std::memset(copiedDocumentation, 'x', sizeof(copiedDocumentation) - 1);
	std::memset(copiedParameterName, 'x', sizeof(copiedParameterName) - 1);
	std::memset(copiedDefault, 'x', sizeof(copiedDefault) - 1);
	auto* deepCopy = Component_GetCallableDescriptor(copiedCallable);
	CHECK(deepCopy && std::memcmp(deepCopy->name.data, "copied", 6) == 0);
	CHECK(std::memcmp(deepCopy->documentation.data, "copied documentation", 20) == 0);
	CHECK(std::memcmp(deepCopy->parameters[0].name.data, "text", 4) == 0);
	CHECK(std::memcmp(deepCopy->parameters[0].default_value.value.string_value.data, "fallback", 8) == 0);

	char errorMessage[128] {};
	OMPCallableError error { OMP_CALLABLE_ABI_VERSION, sizeof(OMPCallableError),
		OMPCallableError_None, 0, { sizeof(errorMessage), 0, errorMessage } };
	OMPCallableValue result = value(OMPCallableValueType_Int64);
	OMPCallableValue args[2] { value(OMPCallableValueType_Int64), value(OMPCallableValueType_Int64) };
	args[0].value.int64_value = 20; args[1].value.int64_value = 22;
	auto invalidResult = result; invalidResult.struct_size = sizeof(OMPCallableValue) - 1;
	CHECK(!Component_InvokeCallable(addCallable, args, 2, &invalidResult, nullptr, &error, 0) &&
		error.code == OMPCallableError_InvalidArgument);
	CHECK(invoke(addCallable, args, 2, result, error));
	CHECK(result.type == OMPCallableValueType_Int64 && result.value.int64_value == 42);
	CHECK(!invoke(addCallable, args, 1, result, error) && error.code == OMPCallableError_ArgumentCount);
	args[1].type = OMPCallableValueType_Double;
	CHECK(!invoke(addCallable, args, 2, result, error) && error.code == OMPCallableError_ArgumentType);
	args[1].type = OMPCallableValueType_Int64;
	OMPCallableValue invalidString = value(OMPCallableValueType_String);
	invalidString.value.string_value = { OMP_CALLABLE_ABI_VERSION, sizeof(OMPCallableStringView), nullptr, 1 };
	OMPCallableValue invalidArgs[2] { invalidString, args[1] };
	CHECK(!invoke(addCallable, invalidArgs, 2, result, error) && error.code == OMPCallableError_InvalidArgument);
	CHECK(!Component_InvokeCallable(nullptr, nullptr, 0, &result, nullptr, &error, 0) && error.code == OMPCallableError_NotFound);
	CHECK(!Component_InvokeCallable(reinterpret_cast<OMPCallableRegistration*>(uintptr_t(1)), nullptr, 0,
		&result, nullptr, &error, 0) && error.code == OMPCallableError_InvalidHandle);
	error.reserved = 1;
	CHECK(!Component_InvokeCallable(addCallable, args, 2, &result, nullptr, &error, 0));
	error.reserved = 0;
	bool validOffThread = true;
	std::thread other([&] { validOffThread = Component_CallableIsValid(addCallable); });
	other.join();
	CHECK(!validOffThread);
	std::atomic<bool> keepChecking { true };
	std::thread concurrentReader([&] {
		while (keepChecking.load(std::memory_order_relaxed))
			CHECK(!Component_CallableIsValid(addCallable));
	});
	for (unsigned i = 0; i < 128; ++i)
	{
		char transientName[32] {};
		std::snprintf(transientName, sizeof(transientName), "transient%u", i);
		auto transientDescriptor = descriptor(transientName, nullptr, 0, OMPCallableValueType_Null);
		auto* transient = Component_RegisterCallable(OwnerUID, &transientDescriptor, echo, nullptr);
		CHECK(transient && Component_UnregisterCallable(transient));
	}
	keepChecking.store(false, std::memory_order_relaxed);
	concurrentReader.join();

	OMPCallableParameter greetParams[2] {};
	for (auto& param : greetParams) { param.abi_version = OMP_CALLABLE_ABI_VERSION; param.struct_size = sizeof(param); param.type = OMPCallableValueType_String; }
	greetParams[0].name = stringView("name"); greetParams[1].name = stringView("prefix");
	greetParams[1].flags = OMPCallableParameterFlag_Optional | OMPCallableParameterFlag_HasDefault;
	greetParams[1].default_value = value(OMPCallableValueType_String);
	greetParams[1].default_value.value.string_value = stringView("Hello, ");
	auto greetDescriptor = descriptor("greet", greetParams, 2, OMPCallableValueType_String);
	auto* greetCallable = Component_RegisterCallable(OwnerUID, &greetDescriptor, greet, nullptr);
	CHECK(greetCallable);
	OMPCallableValue name = value(OMPCallableValueType_String);
	name.value.string_value = stringView("Ada");
	uint8_t outputData[32] {};
	OMPCallableOutputBuffer output { OMP_CALLABLE_ABI_VERSION, sizeof(output), outputData, sizeof(outputData), 0 };
	result = value(OMPCallableValueType_String);
	CHECK(invoke(greetCallable, &name, 1, result, error, &output));
	CHECK(output.length == 10 && std::memcmp(output.data, "Hello, Ada", 10) == 0);
	OMPCallableValue greetingArgs[2] { name, value(OMPCallableValueType_String) };
	greetingArgs[1].value.string_value = stringView("Welcome, ");
	output.length = 0;
	CHECK(invoke(greetCallable, greetingArgs, 2, result, error, &output));
	CHECK(output.length == 12 && std::memcmp(output.data, "Welcome, Ada", 12) == 0);
	output.capacity = 2;
	CHECK(!invoke(greetCallable, &name, 1, result, error, &output) && error.code == OMPCallableError_OutputTooSmall && output.length == 10);
	output.capacity = sizeof(outputData); output.abi_version = 0;
	CHECK(!invoke(greetCallable, &name, 1, result, error, &output) && error.code == OMPCallableError_InvalidArgument);
	output.abi_version = OMP_CALLABLE_ABI_VERSION;

	OMPCallableParameter bytesParam { OMP_CALLABLE_ABI_VERSION, sizeof(OMPCallableParameter),
		stringView("payload"), OMPCallableValueType_Bytes, 0, value(OMPCallableValueType_Null) };
	auto bytesDescriptor = descriptor("inspectBytes", &bytesParam, 1, OMPCallableValueType_UInt64);
	auto* bytesCallable = Component_RegisterCallable(OwnerUID, &bytesDescriptor, inspectBytes, nullptr);
	uint8_t payload[] { 1, 2, 3, 4 };
	OMPCallableValue bytes = value(OMPCallableValueType_Bytes);
	bytes.value.bytes_value = { OMP_CALLABLE_ABI_VERSION, sizeof(OMPCallableBytesView), payload, sizeof(payload) };
	CHECK(invoke(bytesCallable, &bytes, 1, result, error) && result.value.uint64_value == 4);
	auto bytesResultDescriptor = descriptor("returnBytes", nullptr, 0, OMPCallableValueType_Bytes);
	auto* bytesResultCallable = Component_RegisterCallable(OwnerUID, &bytesResultDescriptor, returnBytes, nullptr);
	output.capacity = sizeof(outputData); output.length = 0;
	CHECK(invoke(bytesResultCallable, nullptr, 0, result, error, &output));
	CHECK(output.length == 3 && std::memcmp(output.data, "\x09\x08\x07", 3) == 0);

	auto tamperDescriptor = descriptor("tamperOutput", nullptr, 0, OMPCallableValueType_String);
	auto* tamperCallable = Component_RegisterCallable(OwnerUID, &tamperDescriptor, tamperOutput, nullptr);
	auto* originalOutputData = output.data;
	const auto originalOutputCapacity = output.capacity;
	CHECK(!invoke(tamperCallable, nullptr, 0, result, error, &output));
	CHECK(error.code == OMPCallableError_InternalFailure);
	CHECK(output.data == originalOutputData && output.capacity == originalOutputCapacity);
	auto tamperErrorDescriptor = descriptor("tamperError", nullptr, 0, OMPCallableValueType_Null);
	auto* tamperErrorCallable = Component_RegisterCallable(OwnerUID, &tamperErrorDescriptor, tamperError, nullptr);
	auto* originalErrorData = error.message.data;
	CHECK(!invoke(tamperErrorCallable, nullptr, 0, result, error));
	CHECK(error.code == OMPCallableError_InternalFailure && error.message.data == originalErrorData);

	const OMPCallableValueType scalarTypes[] {
		OMPCallableValueType_Null, OMPCallableValueType_Bool, OMPCallableValueType_Int32,
		OMPCallableValueType_UInt32, OMPCallableValueType_Int64, OMPCallableValueType_UInt64,
		OMPCallableValueType_Float, OMPCallableValueType_Double, OMPCallableValueType_Entity
	};
	const char* scalarNames[] { "echoNull", "echoBool", "echoInt32", "echoUInt32", "echoInt64",
		"echoUInt64", "echoFloat", "echoDouble", "echoEntity" };
	for (size_t i = 0; i < sizeof(scalarTypes) / sizeof(scalarTypes[0]); ++i)
	{
		OMPCallableParameter parameter { OMP_CALLABLE_ABI_VERSION, sizeof(OMPCallableParameter),
			stringView("value"), static_cast<uint32_t>(scalarTypes[i]), 0,
			value(OMPCallableValueType_Null) };
		auto echoDescriptor = descriptor(scalarNames[i], &parameter, 1, scalarTypes[i]);
		auto* echoCallable = Component_RegisterCallable(OwnerUID, &echoDescriptor, echo, nullptr);
		CHECK(echoCallable);
		auto input = value(scalarTypes[i]);
		switch (scalarTypes[i])
		{
		case OMPCallableValueType_Bool: input.value.boolean = true; break;
		case OMPCallableValueType_Int32: input.value.int32_value = -123; break;
		case OMPCallableValueType_UInt32: input.value.uint32_value = 123; break;
		case OMPCallableValueType_Int64: input.value.int64_value = -1234567890123LL; break;
		case OMPCallableValueType_UInt64: input.value.uint64_value = 1234567890123ULL; break;
		case OMPCallableValueType_Float: input.value.float_value = 1.25f; break;
		case OMPCallableValueType_Double: input.value.double_value = 2.5; break;
		case OMPCallableValueType_Entity:
			input.value.entity_value = { OMP_CALLABLE_ABI_VERSION, sizeof(OMPCallableEntityValue), 7, 0, 42 };
			break;
		default: break;
		}
		result = value(scalarTypes[i]);
		CHECK(invoke(echoCallable, &input, 1, result, error));
		CHECK(result.type == input.type && std::memcmp(&result.value, &input.value, sizeof(input.value)) == 0);
	}
	OMPCallableParameter boolParameter { OMP_CALLABLE_ABI_VERSION, sizeof(OMPCallableParameter),
		stringView("value"), OMPCallableValueType_Bool, 0, value(OMPCallableValueType_Null) };
	auto boolDescriptor = descriptor("validateBool", &boolParameter, 1, OMPCallableValueType_Bool);
	auto* boolCallable = Component_RegisterCallable(OwnerUID, &boolDescriptor, echo, nullptr);
	auto invalidBool = value(OMPCallableValueType_Bool); invalidBool.value.boolean = 2;
	CHECK(!invoke(boolCallable, &invalidBool, 1, result, error) && error.code == OMPCallableError_InvalidArgument);

	auto noArgs = descriptor("fail", nullptr, 0, OMPCallableValueType_Null);
	auto* failCallable = Component_RegisterCallable(OwnerUID, &noArgs, reject, nullptr);
	CHECK(!invoke(failCallable, nullptr, 0, result, error) && error.code == OMPCallableError_Rejected && error.message.len != 0);
	noArgs.name = stringView("throw");
	auto* throwCallable = Component_RegisterCallable(OwnerUID, &noArgs, throwing, nullptr);
	CHECK(!invoke(throwCallable, nullptr, 0, result, error) && error.code == OMPCallableError_InternalFailure);

	MutationState mutation;
	noArgs.name = stringView("mutate");
	auto* mutationCallable = Component_RegisterCallable(OwnerUID, &noArgs, mutate, &mutation);
	mutation.callable = mutationCallable;
	CHECK(invoke(mutationCallable, nullptr, 0, result, error));
	CHECK(!mutation.unregisterResult && !mutation.recursiveResult && !mutation.watchResult &&
		!mutation.registerResult && mutation.recursiveCode == OMPCallableError_Busy);

	InvalidationMutationState invalidationMutation { greetCallable, 0 };
	CHECK(Component_WatchCallable(addCallable, mutateDuringInvalidation, &invalidationMutation));
	auto* callableWatch = Component_WatchCallable(addCallable, invalidated, nullptr);
	CHECK(callableWatch);
	CHECK(Component_WatchCallable(addCallable, throwingCallableInvalidation, nullptr));
	CHECK(Component_UnregisterCallable(addCallable));
	CHECK(invalidationMutation.accepted == 0);
	CHECK(invalidations == 1 && !Component_CallableIsValid(addCallable));
	CHECK(addCallable->callback == nullptr && addCallable->userdata == nullptr &&
		addCallable->name.empty() && addCallable->parameters.empty());
	CHECK(!invoke(addCallable, args, 2, result, error) && error.code == OMPCallableError_InvalidHandle);
	CHECK(!Component_UnregisterCallable(addCallable));
	CHECK(!Component_UnwatchCallable(callableWatch));

	auto* ownerWatch = Component_WatchCallable(greetCallable, invalidated, nullptr);
	CHECK(ownerWatch);
	list.component = nullptr;
	CHECK(!invoke(greetCallable, &name, 1, result, error, &output) && error.code == OMPCallableError_ComponentUnavailable);
	list.component = &component;
	manager.componentUnloading(&component);
	CHECK(invalidations == 2);
	CHECK(!Component_CallableIsValid(greetCallable));
	CHECK(!invoke(greetCallable, &name, 1, result, error, &output) && error.code == OMPCallableError_InvalidHandle);
	manager.shutdown();
	CHECK(!Component_CallableIsValid(greetCallable));

	list.component = &component;
	manager.initialize(&list);
	CHECK(Component_RegisterCallable(OwnerUID, &addDescriptor, add, nullptr));
	manager.shutdown();
	return 0;
}
