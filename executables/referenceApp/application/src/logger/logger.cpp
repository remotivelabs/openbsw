/********************************************************************************
 * Copyright (c) 2025 Accenture
 *
 * This program and the accompanying materials are made available under the
 * terms of the Apache License Version 2.0 which is available at
 * https://www.apache.org/licenses/LICENSE-2.0
 *
 * SPDX-License-Identifier: Apache-2.0
 ********************************************************************************/

#include "logger/logger.h"

/* start: adding logger includes */

#include <lifecycle/LifecycleLogger.h>
#include <logger/ConsoleLogger.h>
#include <safeUtils/SafetyLogger.h>
#ifdef PLATFORM_SUPPORT_TRANSPORT
#include <transport/TpRouterLogger.h>
#endif // PLATFORM_SUPPORT_TRANSPORT
#ifdef PLATFORM_SUPPORT_CAN
#include <can/CanLogger.h>
#ifdef PLATFORM_SUPPORT_TRANSPORT
#include <docan/common/DoCanLogger.h>
#endif // PLATFORM_SUPPORT_TRANSPORT
#endif // PLATFORM_SUPPORT_CAN
#if defined(PLATFORM_SUPPORT_TRANSPORT) && defined(PLATFORM_SUPPORT_UDS)
#include <uds/UdsLogger.h>
#endif // PLATFORM_SUPPORT_UDS && TRANSPORT
#ifdef PLATFORM_SUPPORT_ETHERNET
#include <ethernet/EthernetLogger.h>
#include <lwipSocket/utils/LwipLogger.h>
#include <someip/defaultlogger.h>
#include <tcp/TcpLogger.h>
#include <udp/UdpLogger.h>
#ifdef PLATFORM_SUPPORT_TRANSPORT
#include <doip/server/DoIpServerLogger.h>
#endif // PLATFORM_SUPPORT_TRANSPORT
#endif // PLATFORM_SUPPORT_ETHERNET

/* end: adding logger includes */

DEFINE_LOGGER_COMPONENT(BSP);
DEFINE_LOGGER_COMPONENT(COMMON);
DEFINE_LOGGER_COMPONENT(DEMO);
DEFINE_LOGGER_COMPONENT(BCM);
DEFINE_LOGGER_COMPONENT(GLOBAL);
DEFINE_LOGGER_COMPONENT(SAFETY);
DEFINE_LOGGER_COMPONENT(ETHERNET);
DEFINE_LOGGER_COMPONENT(DOIP);
DEFINE_LOGGER_COMPONENT(UDS);
DEFINE_LOGGER_COMPONENT(LWIP);
DEFINE_LOGGER_COMPONENT(RUST);
DEFINE_LOGGER_COMPONENT(ROUTING);
DEFINE_LOGGER_COMPONENT(SOMEIP);

#include <async/AsyncBinding.h>
#include <console/AsyncCommandWrapper.h>
#include <logger/ComponentConfig.h>
#include <logger/ComponentMapping.h>
#include <logger/DefaultLoggerCommand.h>
#include <logger/LoggerComposition.h>

START_LOGGER_COMPONENT_MAPPING_INFO_TABLE(loggerComponentInfoTable)
/* start: adding logger components */
LOGGER_COMPONENT_MAPPING_INFO(_WARN, BSP, ::util::format::Color::DEFAULT_COLOR)
LOGGER_COMPONENT_MAPPING_INFO(_WARN, COMMON, ::util::format::Color::DEFAULT_COLOR)
LOGGER_COMPONENT_MAPPING_INFO(_INFO, DEMO, ::util::format::Color::DEFAULT_COLOR)
LOGGER_COMPONENT_MAPPING_INFO(_INFO, BCM, ::util::format::Color::DEFAULT_COLOR)
LOGGER_COMPONENT_MAPPING_INFO(_DEBUG, GLOBAL, ::util::format::Color::DEFAULT_COLOR)
LOGGER_COMPONENT_MAPPING_INFO(_WARN, LIFECYCLE, ::util::format::Color::DARK_GRAY)
LOGGER_COMPONENT_MAPPING_INFO(_WARN, CONSOLE, ::util::format::Color::DEFAULT_COLOR)
LOGGER_COMPONENT_MAPPING_INFO(_WARN, SAFETY, ::util::format::Color::DEFAULT_COLOR)
LOGGER_COMPONENT_MAPPING_INFO(_WARN, RUST, ::util::format::Color::GREEN)
#ifdef PLATFORM_SUPPORT_CAN
LOGGER_COMPONENT_MAPPING_INFO(_INFO, CAN, ::util::format::Color::LIGHT_BLUE)
#ifdef PLATFORM_SUPPORT_TRANSPORT
LOGGER_COMPONENT_MAPPING_INFO(_WARN, DOCAN, ::util::format::Color::LIGHT_GRAY)
#endif // PLATFORM_SUPPORT_TRANSPORT
#endif // PLATFORM_SUPPORT_CAN
#ifdef PLATFORM_SUPPORT_TRANSPORT
LOGGER_COMPONENT_MAPPING_INFO(_WARN, TPROUTER, ::util::format::Color::LIGHT_YELLOW)
#ifdef PLATFORM_SUPPORT_UDS
LOGGER_COMPONENT_MAPPING_INFO(_WARN, UDS, ::util::format::Color::LIGHT_YELLOW)
#endif // PLATFORM_SUPPORT_UDS
#endif // PLATFORM_SUPPORT_TRANSPORT
#ifdef PLATFORM_SUPPORT_ETHERNET
LOGGER_COMPONENT_MAPPING_INFO(_WARN, ETHERNET, ::util::format::Color::MAGENTA)
LOGGER_COMPONENT_MAPPING_INFO(_WARN, TCP, ::util::format::Color::CYAN)
LOGGER_COMPONENT_MAPPING_INFO(_WARN, UDP, ::util::format::Color::LIGHT_GRAY)
LOGGER_COMPONENT_MAPPING_INFO(_WARN, LWIP, ::util::format::Color::LIGHT_YELLOW)
LOGGER_COMPONENT_MAPPING_INFO(_NOLOGGING, SOMEIP, ::util::format::Color::LIGHT_CYAN)
#ifdef PLATFORM_SUPPORT_TRANSPORT
LOGGER_COMPONENT_MAPPING_INFO(_WARN, DOIP, ::util::format::Color::LIGHT_GREEN)
#endif // PLATFORM_SUPPORT_TRANSPORT
#endif // PLATFORM_SUPPORT_ETHERNET
#if defined(PLATFORM_SUPPORT_ETHERNET) && defined(PLATFORM_SUPPORT_CAN)
LOGGER_COMPONENT_MAPPING_INFO(_WARN, ROUTING, ::util::format::Color::LIGHT_CYAN)
#endif // defined(PLATFORM_SUPPORT_ETHERNET) && defined(PLATFORM_SUPPORT_CAN)

/* end: adding logger components */
END_LOGGER_COMPONENT_MAPPING_INFO_TABLE();

DEFINE_LOGGER_COMPONENT_MAPPING(
    LoggerComponentMappingType,
    loggerComponentMapping,
    loggerComponentInfoTable,
    ::util::logger::LevelInfo::getDefaultTable(),
    GLOBAL);

namespace logger
{
using ComponentConfigType = ComponentConfig<LoggerComponentMappingType::MappingSize>;
ComponentConfigType loggerComponentConfig(loggerComponentMapping);
LoggerComposition loggerComposition(loggerComponentMapping, "BCM");

DefaultLoggerCommand loggerCommand(loggerComponentConfig);

::console::AsyncCommandWrapper
    asyncCommandWrapper(loggerCommand.root(), ::async::AsyncBinding::AdapterType::TASK_IDLE);

void init()
{
    loggerComposition.start(
        LoggerComposition::ConfigStart::create<ComponentConfigType, &ComponentConfigType::start>(
            loggerComponentConfig));
}

void run() { loggerComposition.run(); }

void flush()
{
    loggerComposition.stop(
        LoggerComposition::ConfigStop::create<ComponentConfigType, &ComponentConfigType::shutdown>(
            loggerComponentConfig));
}

using ::util::logger::Logger;
using ::util::logger::LWIP;

// lwIP requires a C-style variadic callback; delegate to Logger's va_list overload.
// NOLINTBEGIN(cppcoreguidelines-pro-type-vararg)
extern "C" void log_lwipInfo(char const* message, ...)
{
    va_list ap;
    va_start(ap, message);
    Logger::log(LWIP, ::util::logger::LEVEL_INFO, message, ap);
    va_end(ap);
}

// NOLINTEND(cppcoreguidelines-pro-type-vararg)

extern "C" void log_lwipError(char const* message) { Logger::error(LWIP, message); }

} // namespace logger
