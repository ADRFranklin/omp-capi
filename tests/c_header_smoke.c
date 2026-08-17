#include <ompcapi.h>

static enum OMPNetResult callback(void* player, int32_t id,
	struct OMPNetBuffer* buffer, void* userdata)
{
	(void)player; (void)id; (void)buffer; (void)userdata;
	return OMPNetResult_Continue;
}

void c_header_smoke(struct OMPAPI_t* api)
{
	api->Network.Subscribe(OMPNetDirection_IncomingPacket, 207, 0, callback, 0);
}
