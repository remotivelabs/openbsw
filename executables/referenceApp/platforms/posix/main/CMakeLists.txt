add_library(main src/main.cpp src/lifecycle/StaticBsp.cpp)

target_include_directories(main PUBLIC include)

target_link_libraries(
    main
    PRIVATE bspUart asyncBinding lifecycle safeSupervisor
    PUBLIC bspEepromDriver)

if (BUILD_TARGET_RTOS STREQUAL "FREERTOS")
    add_library(osHooks src/osHooks/freertos/osHooks.cpp)
    target_link_libraries(osHooks PRIVATE freeRtos etl)
elseif (BUILD_TARGET_RTOS STREQUAL "THREADX")
    add_library(osHooks src/osHooks/threadx/osHooks.cpp)
    target_link_libraries(osHooks PRIVATE threadX)
endif ()

if (PLATFORM_SUPPORT_CAN)
    target_sources(main PRIVATE src/systems/CanSystem.cpp)

    # The most frames that the CAN task moves each way, per bus, every 1 ms run.
    # Empty (the default): 3.
    set(REFERENCE_APP_CAN_FRAMES_PER_RUN
        ""
        CACHE STRING "CAN frames sent and received per bus per run (empty: 3)")
    if (REFERENCE_APP_CAN_FRAMES_PER_RUN)
        if (NOT REFERENCE_APP_CAN_FRAMES_PER_RUN MATCHES "^[1-9][0-9]*$")
            message(
                FATAL_ERROR
                    "REFERENCE_APP_CAN_FRAMES_PER_RUN must be a positive number, "
                    "not '${REFERENCE_APP_CAN_FRAMES_PER_RUN}'")
        endif ()
        set_property(
            SOURCE src/systems/CanSystem.cpp
            APPEND
            PROPERTY
                COMPILE_DEFINITIONS
                "REFERENCE_APP_CAN_FRAMES_PER_RUN=${REFERENCE_APP_CAN_FRAMES_PER_RUN}"
        )
    endif ()

    target_link_libraries(main PRIVATE configuration socketCanTransceiver)
endif ()

if (PLATFORM_SUPPORT_ETHERNET)
    target_sources(main PRIVATE src/systems/TapEthernetSystem.cpp)
    target_link_libraries(main PRIVATE lwipSocket tapEthernetDriver
                                       ethConfiguration configuration)

endif ()
