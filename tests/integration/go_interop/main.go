package main

/*
#cgo CFLAGS: -I../../../lib/open.mp-capi/include
#include <ompcapi.h>

struct InteropExampleAPI {
	struct OMPComponentAPIHeader header;
	int32_t (*add)(int32_t, int32_t);
};

extern int32_t InteropGoMultiply(int32_t left, int32_t right);

static int32_t call_add(const struct InteropExampleAPI* api, int32_t left, int32_t right) {
	return api->add(left, right);
}

static struct OMPComponentHandle* find_component(struct OMPAPI_t* api, uint64_t uid) {
	return api->ComponentInterop.Find(uid);
}

static const void* query_api(struct OMPAPI_t* api, struct OMPComponentHandle* component,
	uint64_t uid, uint32_t version, uint32_t size) {
	return api->ComponentInterop.QueryAPI(component, uid, version, size);
}

static struct OMPComponentAPIHeader* get_go_api(void) {
	static struct InteropExampleAPI api = {
		{ UINT64_C(0x474F41504954424C), 1, sizeof(struct InteropExampleAPI) },
		InteropGoMultiply
	};
	return &api.header;
}
*/
import "C"
import "unsafe"

const parityComponentUID = C.uint64_t(0xD93D4096397AC9D1)
const exampleInterfaceUID = C.uint64_t(0x4350415049544553)

//export InteropGoMultiply
func InteropGoMultiply(left C.int32_t, right C.int32_t) C.int32_t {
	return left * right
}

//export InteropGoQuery
func InteropGoQuery(api *C.struct_OMPAPI_t) C.int32_t {
	component := C.find_component(api, parityComponentUID)
	if component == nil {
		return -1
	}
	table := C.query_api(api, component, exampleInterfaceUID, 1, C.uint32_t(C.sizeof_struct_InteropExampleAPI))
	if table == nil {
		return -2
	}
	return C.call_add((*C.struct_InteropExampleAPI)(unsafe.Pointer(table)), 19, 23)
}

//export InteropGoAPI
func InteropGoAPI() *C.struct_OMPComponentAPIHeader {
	return C.get_go_api()
}

func main() {}
