# An application can replace the referenceApp's demo (DemoSystem and the CAN
# demo listener) without changing this tree: REFERENCE_APP_APPLICATION_DIR
# names a directory with
#
#   CMakeLists.txt              adds the application's sources (and any further
#                               include directories or libraries) to the
#                               app.referenceApp target
#   include/systems/DemoSystem.h  a systems::DemoSystem with the constructor
#                               that app.cpp calls; it is found before the
#                               referenceApp's own
#
# The application logs with the APP logger component (app/AppLogger.h), which
# takes the place of the demo's DEMO.
#
# Empty (the default) builds the demo.
set(REFERENCE_APP_APPLICATION_DIR
    ""
    CACHE PATH "Application to build in place of the referenceApp demo")

add_executable(
    app.referenceApp
    src/app/app.cpp
    src/console/console.cpp
    src/logger/logger.cpp
    src/systems/RuntimeSystem.cpp
    src/systems/SysAdminSystem.cpp
    src/systems/SafetySystem.cpp
    src/main.cpp)

if (NOT REFERENCE_APP_APPLICATION_DIR)
    target_sources(app.referenceApp PRIVATE src/systems/DemoSystem.cpp)
endif ()

if (PLATFORM_SUPPORT_TRANSPORT)
    target_sources(app.referenceApp PRIVATE src/systems/TransportSystem.cpp)
    if (PLATFORM_SUPPORT_UDS)
        target_sources(app.referenceApp PRIVATE src/systems/UdsSystem.cpp
                                                src/uds/ReadIdentifierPot.cpp)
        set(REFERENCE_APP_SW_VERSION
            ""
            CACHE STRING
                  "Software version reported in UDS DID F195 (empty: built-in default)")
        if (REFERENCE_APP_SW_VERSION)
            # On UdsSystem.cpp only, so that a new version recompiles one file.
            set_property(
                SOURCE src/systems/UdsSystem.cpp
                APPEND
                PROPERTY COMPILE_DEFINITIONS
                         "REFERENCE_APP_SW_VERSION=\"${REFERENCE_APP_SW_VERSION}\"")
        endif ()
        if (PLATFORM_SUPPORT_UDS_DEMO_SERVICES)
            target_sources(
                app.referenceApp
                PRIVATE src/uds/DemoClearDtc.cpp
                        src/uds/DemoControlDtcSetting.cpp
                        src/uds/DemoRoutine.cpp
                        src/uds/DemoDtcManager.cpp
                        src/uds/DemoReadDtcInfo.cpp
                        src/uds/DemoSecurityAccess.cpp
                        src/uds/DemoWriteVin.cpp)
        endif ()
    endif ()
endif ()

if (PLATFORM_SUPPORT_ETHERNET)
    target_sources(
        app.referenceApp
        PRIVATE src/systems/EthernetSystem.cpp
                src/systems/SomeIpSystem.cpp
                src/someip/EventListener.cpp
                src/someip/PeriodicRoutine.cpp
                src/someip/ProvidedServiceData.cpp
                src/someip/ProvidedServiceImpl.cpp
                src/someip/ServiceListener.cpp
                src/someip/TimeServiceReply.cpp
                src/someip/TimeServiceRequest.cpp)
    if (PLATFORM_SUPPORT_TRANSPORT)
        target_sources(app.referenceApp
                       PRIVATE src/systems/DoIpServerSystem.cpp)
    endif ()

    if (PLATFORM_SUPPORT_CAN)
        target_sources(app.referenceApp PRIVATE src/systems/RoutingSystem.cpp)
    endif ()
endif ()

if (PLATFORM_SUPPORT_CAN)
    if (NOT REFERENCE_APP_APPLICATION_DIR)
        target_sources(app.referenceApp PRIVATE src/app/CanDemoListener.cpp)
    endif ()
    if (PLATFORM_SUPPORT_TRANSPORT)
        target_sources(
            app.referenceApp
            PRIVATE src/systems/DoCanSystem.cpp
                    src/can/DoCanMultiAddressingTransportLayer.cpp)
    endif ()
endif ()

if (PLATFORM_SUPPORT_STORAGE)
    target_sources(app.referenceApp PRIVATE src/systems/StorageSystem.cpp)
endif ()

set_target_properties(app.referenceApp PROPERTIES SUFFIX ".elf")

if (PLATFORM_SUPPORT_ROM_CHECK)
    add_custom_command(
        TARGET app.referenceApp
        POST_BUILD
        COMMAND python3 ${OPENBSW_DIR}/tools/safety/crcCheck.py
        COMMENT
            "Running post-build script to run the CRC and inject the result into the ELF file"
    )
endif ()

if (TARGET startUp)
    get_target_property(LINKER_SCRIPT startUp PROP_LINKER_SCRIPT)
    if (DEFINED LINKER_SCRIPT)
        target_link_options(app.referenceApp PRIVATE -T ${LINKER_SCRIPT})
    endif ()
endif ()

target_include_directories(app.referenceApp PRIVATE include)

target_link_libraries(
    app.referenceApp
    PRIVATE asyncBinding
            asyncConsole
            bspMcu
            configuration
            etl
            etlImpl
            lifecycle
            consoleCommands
            logger
            printf
            loggerIntegration
            main
            stdioConsoleInput
            osRtos
            asyncRtosImpl
            util
            time
            runtime
            safeLifecycle
            safeUtils
            safeWatchdog
            bspUart
            socBsp)

if (PLATFORM_SUPPORT_TRANSPORT)
    target_link_libraries(app.referenceApp PRIVATE transport
                                                   transportRouterSimple)
    if (PLATFORM_SUPPORT_UDS)
        target_link_libraries(app.referenceApp PRIVATE uds udsConfigurationImpl)
    endif ()
endif ()

if (PLATFORM_SUPPORT_CAN)
    target_link_libraries(app.referenceApp PRIVATE cpp2can)
    if (PLATFORM_SUPPORT_TRANSPORT)
        target_link_libraries(app.referenceApp PRIVATE docan)
    endif ()
endif ()

if (PLATFORM_SUPPORT_ETHERNET)
    target_link_libraries(
        app.referenceApp
        PRIVATE cpp2ethernet
                cpp2someip
                lwipSocket
                lwipcore
                ethConfiguration)
    if (PLATFORM_SUPPORT_TRANSPORT)
        target_link_libraries(app.referenceApp PRIVATE doip)
    endif ()

    if (PLATFORM_SUPPORT_CAN)
        target_link_libraries(app.referenceApp PRIVATE blob routing)
    endif ()
endif ()

if (PLATFORM_SUPPORT_STORAGE)
    target_link_libraries(app.referenceApp PRIVATE storage)
endif ()

if (PLATFORM_SUPPORT_ROM_CHECK)
    target_link_libraries(app.referenceApp PRIVATE safeMemory)
endif ()

if (PLATFORM_SUPPORT_MIDDLEWARE)
    # --whole-archive is required because AllocatorSelectorDefinitions.cpp
    # registers allocator callbacks into middleware::memory that are not
    # directly referenced by app.referenceApp symbols.
    target_link_libraries(
        app.referenceApp PRIVATE -Wl,--whole-archive middlewareConfiguration
                                 -Wl,--no-whole-archive)
endif ()

if (BUILD_RUST)
    target_compile_definitions(app.referenceApp PRIVATE BUILD_RUST)
    target_sources(app.referenceApp PRIVATE src/app/rust/RustRuntime.cpp
                                            src/app/rust/RustRuntimeConfig.cpp)
    target_link_libraries(app.referenceApp PRIVATE bspInterrupts)
    # Link against Rust static library(should be only one static library)
    target_link_libraries(app.referenceApp PRIVATE openbsw_logger
                                                   rust_hello_world)
    # Add include directory for Rust C/C++ headers
    target_include_directories(
        app.referenceApp
        PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../rustHelloWorld/include)
endif ()

if (REFERENCE_APP_APPLICATION_DIR)
    message(STATUS "referenceApp application: ${REFERENCE_APP_APPLICATION_DIR}")
    set_property(
        SOURCE src/logger/logger.cpp
        APPEND
        PROPERTY COMPILE_DEFINITIONS REFERENCE_APP_EXTERNAL_APPLICATION)
    target_include_directories(app.referenceApp BEFORE
                               PRIVATE ${REFERENCE_APP_APPLICATION_DIR}/include)
    add_subdirectory(${REFERENCE_APP_APPLICATION_DIR}
                     ${CMAKE_CURRENT_BINARY_DIR}/externalApplication)
endif ()
