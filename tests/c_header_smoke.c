#include <ompcapi.h>
#include <stddef.h>

_Static_assert(sizeof(struct Component_t) == sizeof(Component_Create_t),
	"existing Component_t ABI must not grow");
_Static_assert(offsetof(struct OMPAPI_t, ComponentInterop) > offsetof(struct OMPAPI_t, Network),
	"new API groups must be appended to OMPAPI_t");
_Static_assert(offsetof(struct ComponentInterop_t, RegisterCallable) > offsetof(struct ComponentInterop_t, Unwatch),
	"callable APIs must be appended to ComponentInterop_t");
_Static_assert(sizeof(((struct OMPCallableValue*)0)->type) == sizeof(uint32_t),
	"callable type tags must have fixed-width storage");
_Static_assert(sizeof(((struct OMPCallableValue*)0)->value.boolean) == sizeof(uint8_t),
	"callable booleans must have fixed-width storage");

static enum OMPNetResult callback(void* player, int32_t id,
	struct OMPNetBuffer* buffer, void* userdata)
{
	(void)player; (void)id; (void)buffer; (void)userdata;
	return OMPNetResult_Continue;
}

void c_header_smoke(struct OMPAPI_t* api)
{
	api->Network.Subscribe(OMPNetDirection_IncomingPacket, 207, 0, callback, 0);
	struct OMPComponentHandle* component = api->ComponentInterop.Find(UINT64_C(0x1234));
	api->ComponentInterop.QueryAPI(component, UINT64_C(0x5678), 1, sizeof(struct OMPComponentAPIHeader));
	api->ComponentInterop.FindCallable(component, (struct CAPIStringView) { 3, "add" });
}
