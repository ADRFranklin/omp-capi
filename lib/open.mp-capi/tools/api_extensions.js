const APIs = {
  ComponentInterop: [
    { ret: "struct OMPComponentHandle*", name: "Component_Find", params: [{ name: "uid", type: "uint64_t" }] },
    { ret: "bool", name: "Component_IsValid", params: [{ name: "component", type: "struct OMPComponentHandle*" }] },
    { ret: "uint64_t", name: "Component_GetUID", params: [{ name: "component", type: "struct OMPComponentHandle*" }] },
    { ret: "int", name: "Component_GetName", params: [{ name: "component", type: "struct OMPComponentHandle*" }, { name: "output", type: "CAPIStringBuffer*" }] },
    { ret: "bool", name: "Component_GetVersion", params: [{ name: "component", type: "struct OMPComponentHandle*" }, { name: "output", type: "ComponentVersion*" }] },
    { ret: "int32_t", name: "Component_GetType", params: [{ name: "component", type: "struct OMPComponentHandle*" }] },
    { ret: "struct OMPComponentAPIRegistration*", name: "Component_RegisterAPI", params: [{ name: "owner_uid", type: "uint64_t" }, { name: "table", type: "const struct OMPComponentAPIHeader*" }] },
    { ret: "bool", name: "Component_UnregisterAPI", params: [{ name: "registration", type: "struct OMPComponentAPIRegistration*" }] },
    { ret: "const void*", name: "Component_QueryAPI", params: [{ name: "component", type: "struct OMPComponentHandle*" }, { name: "interface_uid", type: "uint64_t" }, { name: "abi_version", type: "uint32_t" }, { name: "struct_size", type: "uint32_t" }] },
    { ret: "bool", name: "Component_APIIsValid", params: [{ name: "component", type: "struct OMPComponentHandle*" }, { name: "table", type: "const void*" }] },
    { ret: "struct OMPComponentWatch*", name: "Component_Watch", params: [{ name: "component", type: "struct OMPComponentHandle*" }, { name: "callback", type: "OMPComponentInvalidatedCallback" }, { name: "userdata", type: "void*" }] },
    { ret: "bool", name: "Component_Unwatch", params: [{ name: "watch", type: "struct OMPComponentWatch*" }] },
    { ret: "struct OMPCallableRegistration*", name: "Component_RegisterCallable", params: [{ name: "owner_uid", type: "uint64_t" }, { name: "descriptor", type: "const struct OMPCallableDescriptor*" }, { name: "callback", type: "OMPCallableCallback" }, { name: "userdata", type: "void*" }] },
    { ret: "bool", name: "Component_UnregisterCallable", params: [{ name: "callable", type: "struct OMPCallableRegistration*" }] },
    { ret: "struct OMPCallableRegistration*", name: "Component_FindCallable", params: [{ name: "component", type: "struct OMPComponentHandle*" }, { name: "name", type: "struct CAPIStringView" }] },
    { ret: "bool", name: "Component_CallableIsValid", params: [{ name: "callable", type: "struct OMPCallableRegistration*" }] },
    { ret: "uint32_t", name: "Component_GetCallableCount", params: [{ name: "component", type: "struct OMPComponentHandle*" }] },
    { ret: "struct OMPCallableRegistration*", name: "Component_GetCallableAt", params: [{ name: "component", type: "struct OMPComponentHandle*" }, { name: "index", type: "uint32_t" }] },
    { ret: "const struct OMPCallableDescriptor*", name: "Component_GetCallableDescriptor", params: [{ name: "callable", type: "struct OMPCallableRegistration*" }] },
    { ret: "bool", name: "Component_InvokeCallable", params: [{ name: "callable", type: "struct OMPCallableRegistration*" }, { name: "args", type: "const struct OMPCallableValue*" }, { name: "arg_count", type: "uint32_t" }, { name: "result", type: "struct OMPCallableValue*" }, { name: "output", type: "struct OMPCallableOutputBuffer*" }, { name: "error", type: "struct OMPCallableError*" }, { name: "flags", type: "uint32_t" }] },
    { ret: "struct OMPCallableWatch*", name: "Component_WatchCallable", params: [{ name: "callable", type: "struct OMPCallableRegistration*" }, { name: "callback", type: "OMPCallableInvalidatedCallback" }, { name: "userdata", type: "void*" }] },
    { ret: "bool", name: "Component_UnwatchCallable", params: [{ name: "watch", type: "struct OMPCallableWatch*" }] }
  ],
  Network: [
    { ret: "struct OMPNetSubscription*", name: "Network_Subscribe", params: [{ name: "direction", type: "enum OMPNetDirection" }, { name: "id", type: "int32_t" }, { name: "priority", type: "int8_t" }, { name: "callback", type: "OMPNetCallback" }, { name: "userdata", type: "void*" }] },
    { ret: "struct OMPNetSubscription*", name: "Network_SubscribeAll", params: [{ name: "direction", type: "enum OMPNetDirection" }, { name: "priority", type: "int8_t" }, { name: "callback", type: "OMPNetCallback" }, { name: "userdata", type: "void*" }] },
    { ret: "bool", name: "Network_Unsubscribe", params: [{ name: "subscription", type: "struct OMPNetSubscription*" }] },
    { ret: "bool", name: "Network_BufferResize", params: [{ name: "buffer", type: "struct OMPNetBuffer*" }, { name: "bit_length", type: "uint32_t" }] },
    { ret: "bool", name: "Network_SendPacket", params: [{ name: "player", type: "void*" }, { name: "data", type: "const uint8_t*" }, { name: "bit_length", type: "uint32_t" }, { name: "channel", type: "int32_t" }, { name: "dispatch_events", type: "bool" }] },
    { ret: "bool", name: "Network_SendRPC", params: [{ name: "player", type: "void*" }, { name: "id", type: "int32_t" }, { name: "data", type: "const uint8_t*" }, { name: "bit_length", type: "uint32_t" }, { name: "channel", type: "int32_t" }, { name: "dispatch_events", type: "bool" }] },
    { ret: "uint32_t", name: "Network_BroadcastPacket", params: [{ name: "network_type", type: "int32_t" }, { name: "except_player", type: "void*" }, { name: "data", type: "const uint8_t*" }, { name: "bit_length", type: "uint32_t" }, { name: "channel", type: "int32_t" }, { name: "dispatch_events", type: "bool" }] },
    { ret: "uint32_t", name: "Network_BroadcastRPC", params: [{ name: "network_type", type: "int32_t" }, { name: "except_player", type: "void*" }, { name: "id", type: "int32_t" }, { name: "data", type: "const uint8_t*" }, { name: "bit_length", type: "uint32_t" }, { name: "channel", type: "int32_t" }, { name: "dispatch_events", type: "bool" }] },
    { ret: "uint32_t", name: "Network_Count", params: [] },
    { ret: "int32_t", name: "Network_Type", params: [{ name: "index", type: "uint32_t" }] }
  ]
};

const HEADER_TYPES = `
/* Networking views are borrowed for the duration of a synchronous callback. */
struct OMPNetBuffer {
    uint8_t* data;
    uint32_t bit_length;
    uint32_t capacity_bits;
    uint32_t read_offset_bits;
    void* internal;
};

struct OMPNetSubscription;
struct OMPComponentHandle;
struct OMPComponentAPIRegistration;
struct OMPComponentWatch;
struct OMPCallableRegistration;
struct OMPCallableWatch;

struct OMPComponentAPIHeader {
    uint64_t interface_uid;
    uint32_t abi_version;
    uint32_t struct_size;
};

typedef void (*OMPComponentInvalidatedCallback)(struct OMPComponentHandle*, void*);

#define OMP_CALLABLE_ABI_VERSION 1u

enum OMPCallableValueType {
    OMPCallableValueType_Null = 0,
    OMPCallableValueType_Bool = 1,
    OMPCallableValueType_Int32 = 2,
    OMPCallableValueType_UInt32 = 3,
    OMPCallableValueType_Int64 = 4,
    OMPCallableValueType_UInt64 = 5,
    OMPCallableValueType_Float = 6,
    OMPCallableValueType_Double = 7,
    OMPCallableValueType_String = 8,
    OMPCallableValueType_Bytes = 9,
    OMPCallableValueType_Entity = 10
};

enum OMPCallableErrorCode {
    OMPCallableError_None = 0,
    OMPCallableError_NotFound = 1,
    OMPCallableError_InvalidHandle = 2,
    OMPCallableError_ArgumentCount = 3,
    OMPCallableError_ArgumentType = 4,
    OMPCallableError_InvalidArgument = 5,
    OMPCallableError_ComponentUnavailable = 6,
    OMPCallableError_Rejected = 7,
    OMPCallableError_AllocationFailure = 8,
    OMPCallableError_InternalFailure = 9,
    OMPCallableError_Busy = 10,
    OMPCallableError_OutputTooSmall = 11
};

enum OMPCallableParameterFlags {
    OMPCallableParameterFlag_None = 0,
    OMPCallableParameterFlag_Optional = 1u << 0,
    OMPCallableParameterFlag_HasDefault = 1u << 1
};

enum OMPCallableFlags {
    OMPCallableFlag_None = 0,
    OMPCallableFlag_Deprecated = 1u << 0,
    OMPCallableFlag_MainThreadOnly = 1u << 1,
    OMPCallableFlag_MayCallback = 1u << 2
};

struct OMPCallableStringView {
    uint32_t abi_version;
    uint32_t struct_size;
    const char* data;
    uint32_t length;
};

struct OMPCallableBytesView {
    uint32_t abi_version;
    uint32_t struct_size;
    const uint8_t* data;
    uint32_t length;
};

struct OMPCallableEntityValue {
    uint32_t abi_version;
    uint32_t struct_size;
    uint32_t entity_type;
    uint32_t reserved;
    uint64_t id;
};

struct OMPCallableValue {
    uint32_t abi_version;
    uint32_t struct_size;
    uint32_t type;
    uint32_t flags;
    union {
        uint8_t boolean;
        int32_t int32_value;
        uint32_t uint32_value;
        int64_t int64_value;
        uint64_t uint64_value;
        float float_value;
        double double_value;
        struct OMPCallableStringView string_value;
        struct OMPCallableBytesView bytes_value;
        struct OMPCallableEntityValue entity_value;
    } value;
};

struct OMPCallableParameter {
    uint32_t abi_version;
    uint32_t struct_size;
    struct OMPCallableStringView name;
    uint32_t type;
    uint32_t flags;
    struct OMPCallableValue default_value;
};

struct OMPCallableDescriptor {
    uint32_t abi_version;
    uint32_t struct_size;
    struct OMPCallableStringView name;
    struct OMPCallableStringView documentation;
    uint32_t parameter_count;
    const struct OMPCallableParameter* parameters;
    uint32_t return_type;
    uint32_t flags;
};

struct OMPCallableOutputBuffer {
    uint32_t abi_version;
    uint32_t struct_size;
    uint8_t* data;
    uint32_t capacity;
    uint32_t length;
};

struct OMPCallableError {
    uint32_t abi_version;
    uint32_t struct_size;
    uint32_t code;
    uint32_t reserved;
    struct CAPIStringBuffer message;
};

struct OMPCallableContext {
    uint32_t abi_version;
    uint32_t struct_size;
    const struct OMPCallableValue* arguments;
    uint32_t argument_count;
    struct OMPCallableValue* result;
    struct OMPCallableOutputBuffer* output;
    struct OMPCallableError* error;
    uint32_t flags;
    uint32_t reserved[4];
};

typedef bool (*OMPCallableCallback)(struct OMPCallableContext*, void*);
typedef void (*OMPCallableInvalidatedCallback)(struct OMPCallableRegistration*, void*);

enum OMPNetResult {
    OMPNetResult_Continue = 0,
    OMPNetResult_Drop = 1
};

enum OMPNetDirection {
    OMPNetDirection_IncomingPacket = 0,
    OMPNetDirection_OutgoingPacket = 1,
    OMPNetDirection_IncomingRPC = 2,
    OMPNetDirection_OutgoingRPC = 3
};

typedef enum OMPNetResult (*OMPNetCallback)(void*, int32_t, struct OMPNetBuffer*, void*);
`;

const TYPES = {
  schema_version: 1,
  opaque: [
    { name: "OMPNetSubscription", ownership: "capi", lifetime: "until successfully passed to Network_Unsubscribe or component shutdown" },
    { name: "OMPComponentHandle", ownership: "capi", lifetime: "until its component is invalidated or the CAPI component shuts down" },
    { name: "OMPComponentAPIRegistration", ownership: "capi", lifetime: "until successfully passed to Component_UnregisterAPI or component shutdown" },
    { name: "OMPComponentWatch", ownership: "capi", lifetime: "until successfully passed to Component_Unwatch or component shutdown" },
    { name: "OMPCallableRegistration", ownership: "capi", lifetime: "stable tombstone; valid until unregistration, owner unload, or CAPI shutdown" },
    { name: "OMPCallableWatch", ownership: "capi", lifetime: "until invalidation or successfully passed to Component_UnwatchCallable" }
  ],
  structs: [
    {
      name: "OMPNetBuffer",
      lifetime: "borrowed for the duration of an OMPNetCallback invocation",
      mutable: true,
      fields: [
        { name: "data", type: "uint8_t*", units: "bytes", ownership: "borrowed" },
        { name: "bit_length", type: "uint32_t", units: "bits" },
        { name: "capacity_bits", type: "uint32_t", units: "bits" },
        { name: "read_offset_bits", type: "uint32_t", units: "bits" },
        { name: "internal", type: "void*", visibility: "private" }
      ]
    },
    {
      name: "OMPComponentAPIHeader",
      fields: [
        { name: "interface_uid", type: "uint64_t" },
        { name: "abi_version", type: "uint32_t" },
        { name: "struct_size", type: "uint32_t", units: "bytes" }
      ]
    },
    {
      name: "OMPCallableStringView",
      abi_version: 1,
      lifetime: "borrowed for the duration specified by the containing value or descriptor",
      description: "Length-delimited UTF-8 text; data may be null only when length is zero"
    },
    {
      name: "OMPCallableBytesView",
      abi_version: 1,
      lifetime: "borrowed for the duration specified by the containing value or descriptor",
      description: "Length-delimited bytes; data may be null only when length is zero"
    },
    {
      name: "OMPCallableEntityValue",
      abi_version: 1,
      description: "Language-neutral entity type and stable numeric identifier"
    },
    {
      name: "OMPCallableValue",
      abi_version: 1,
      description: "Versioned tagged value union; pointer members are limited to borrowed UTF-8 and byte views"
    },
    {
      name: "OMPCallableParameter",
      abi_version: 1,
      description: "Deep-copied parameter metadata with an optional default value"
    },
    {
      name: "OMPCallableDescriptor",
      abi_version: 1,
      lifetime: "registry-owned and valid while its callable handle remains valid"
    },
    {
      name: "OMPCallableOutputBuffer",
      abi_version: 1,
      ownership: "caller",
      description: "Caller-provided writable storage for string and byte results; capacity and length are bytes"
    },
    {
      name: "OMPCallableError",
      abi_version: 1,
      ownership: "caller",
      description: "Structured error with a caller-provided UTF-8 message buffer"
    },
    {
      name: "OMPCallableContext",
      abi_version: 1,
      lifetime: "borrowed for one synchronous callback invocation"
    }
  ],
  enums: [
    {
      name: "OMPNetResult",
      values: { OMPNetResult_Continue: 0, OMPNetResult_Drop: 1 }
    },
    {
      name: "OMPNetDirection",
      values: {
        OMPNetDirection_IncomingPacket: 0,
        OMPNetDirection_OutgoingPacket: 1,
        OMPNetDirection_IncomingRPC: 2,
        OMPNetDirection_OutgoingRPC: 3
      }
    },
    {
      name: "OMPCallableValueType",
      values: {
        OMPCallableValueType_Null: 0, OMPCallableValueType_Bool: 1,
        OMPCallableValueType_Int32: 2, OMPCallableValueType_UInt32: 3,
        OMPCallableValueType_Int64: 4, OMPCallableValueType_UInt64: 5,
        OMPCallableValueType_Float: 6, OMPCallableValueType_Double: 7,
        OMPCallableValueType_String: 8, OMPCallableValueType_Bytes: 9,
        OMPCallableValueType_Entity: 10
      }
    },
    {
      name: "OMPCallableErrorCode",
      values: {
        OMPCallableError_None: 0, OMPCallableError_NotFound: 1,
        OMPCallableError_InvalidHandle: 2, OMPCallableError_ArgumentCount: 3,
        OMPCallableError_ArgumentType: 4, OMPCallableError_InvalidArgument: 5,
        OMPCallableError_ComponentUnavailable: 6, OMPCallableError_Rejected: 7,
        OMPCallableError_AllocationFailure: 8, OMPCallableError_InternalFailure: 9,
        OMPCallableError_Busy: 10, OMPCallableError_OutputTooSmall: 11
      }
    },
    {
      name: "OMPCallableParameterFlags",
      flags: true,
      values: {
        OMPCallableParameterFlag_None: 0,
        OMPCallableParameterFlag_Optional: 1,
        OMPCallableParameterFlag_HasDefault: 2
      }
    },
    {
      name: "OMPCallableFlags",
      flags: true,
      values: {
        OMPCallableFlag_None: 0,
        OMPCallableFlag_Deprecated: 1,
        OMPCallableFlag_MainThreadOnly: 2,
        OMPCallableFlag_MayCallback: 4
      }
    }
  ],
  callbacks: [
    {
      name: "OMPNetCallback",
      ret: "enum OMPNetResult",
      synchronous: true,
      reentrant: true,
      thread: "server main thread",
      params: [
        { name: "player", type: "void*", nullable: true, ownership: "borrowed" },
        { name: "id", type: "int32_t" },
        { name: "buffer", type: "struct OMPNetBuffer*", nullable: false, ownership: "borrowed" },
        { name: "userdata", type: "void*", nullable: true, ownership: "caller" }
      ]
    },
    {
      name: "OMPComponentInvalidatedCallback",
      ret: "void",
      synchronous: true,
      reentrant: true,
      thread: "server main thread",
      params: [
        { name: "component", type: "struct OMPComponentHandle*", nullable: false, ownership: "borrowed" },
        { name: "userdata", type: "void*", nullable: true, ownership: "caller" }
      ]
    },
    {
      name: "OMPCallableCallback",
      ret: "bool",
      synchronous: true,
      reentrant: false,
      thread: "server main thread",
      params: [
        { name: "context", type: "struct OMPCallableContext*", nullable: false, ownership: "borrowed" },
        { name: "userdata", type: "void*", nullable: true, ownership: "provider" }
      ]
    },
    {
      name: "OMPCallableInvalidatedCallback",
      ret: "void",
      synchronous: true,
      reentrant: false,
      thread: "server main thread",
      params: [
        { name: "callable", type: "struct OMPCallableRegistration*", nullable: false, ownership: "borrowed" },
        { name: "userdata", type: "void*", nullable: true, ownership: "caller" }
      ]
    }
  ]
};

module.exports = { APIs, HEADER_TYPES, TYPES };
