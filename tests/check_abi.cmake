if(NOT EXISTS "${COMPONENT_FILE}")
    message(FATAL_ERROR "Component was not built: ${COMPONENT_FILE}")
endif()
execute_process(COMMAND nm -D --defined-only "${COMPONENT_FILE}"
    RESULT_VARIABLE status OUTPUT_VARIABLE symbols)
if(NOT status EQUAL 0)
    message(FATAL_ERROR "Could not inspect component symbols")
endif()
foreach(symbol IN ITEMS ComponentEntryPoint Core_TickCount Player_GetID
        Vehicle_Create Object_Create Actor_Create Pickup_Create Component_Create
        Event_AddHandler)
    string(FIND "${symbols}" " ${symbol}" symbol_position)
    if(symbol_position EQUAL -1)
        message(FATAL_ERROR "Missing expected C ABI symbol: ${symbol}")
    endif()
endforeach()
execute_process(COMMAND readelf -d "${COMPONENT_FILE}"
    RESULT_VARIABLE status OUTPUT_VARIABLE dependencies)
if(NOT status EQUAL 0)
    message(FATAL_ERROR "Could not inspect dynamic dependencies")
endif()
if(dependencies MATCHES "OMP-SDK|OMP-CAPI")
    message(FATAL_ERROR "Unexpected open.mp runtime dependency")
endif()
