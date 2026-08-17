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
    { ret: "bool", name: "Component_Unwatch", params: [{ name: "watch", type: "struct OMPComponentWatch*" }] }
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

struct OMPComponentAPIHeader {
    uint64_t interface_uid;
    uint32_t abi_version;
    uint32_t struct_size;
};

typedef void (*OMPComponentInvalidatedCallback)(struct OMPComponentHandle*, void*);

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
    { name: "OMPComponentWatch", ownership: "capi", lifetime: "until successfully passed to Component_Unwatch or component shutdown" }
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
    }
  ]
};

module.exports = { APIs, HEADER_TYPES, TYPES };
