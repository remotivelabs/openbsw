/********************************************************************************
 * Copyright (c) 2024 Accenture
 *
 * This program and the accompanying materials are made available under the
 * terms of the Apache License Version 2.0 which is available at
 * https://www.apache.org/licenses/LICENSE-2.0
 *
 * SPDX-License-Identifier: Apache-2.0
 ********************************************************************************/

#include "systems/CanSystem.h"

#include <can/CanLogger.h>

#include <cstdlib>
#include <cstring>

namespace systems
{
using ::util::logger::CAN;
using ::util::logger::Logger;

// The most frames that the CAN task moves each way, per bus, per run (every
// TIMEOUT_CAN_SYSTEM_IN_MS). A build can raise it with REFERENCE_APP_CAN_FRAMES_PER_RUN for an
// application that sends more frames at once.
#ifndef REFERENCE_APP_CAN_FRAMES_PER_RUN
#define REFERENCE_APP_CAN_FRAMES_PER_RUN 3
#endif

static uint32_t const TIMEOUT_CAN_SYSTEM_IN_MS = 1U;
static int const MAX_SENT_PER_RUN              = REFERENCE_APP_CAN_FRAMES_PER_RUN;
static int const MAX_RECEIVED_PER_RUN          = REFERENCE_APP_CAN_FRAMES_PER_RUN;

namespace
{

#if defined(PLATFORM_CAN0_USE_FD)
char const* const DEFAULT_CAN_FD = "1";
#else
char const* const DEFAULT_CAN_FD = "0";
#endif

// The environment can override the interfaces and the frame format, so that one binary can be
// attached to whatever interfaces a host or container provides:
//   OPENBSW_CAN0_IF  SocketCAN interface for CAN_0 (default vcan0)
//   OPENBSW_CAN1_IF  SocketCAN interface for CAN_1 (default vcan1)
//   OPENBSW_CAN_FD   1: CAN FD with bit rate switching, 0: classic CAN
//                    (default as PLATFORM_CAN0_USE_FD)
char const* envOrDefault(char const* const name, char const* const fallback)
{
    char const* const value = ::std::getenv(name);
    return ((value != nullptr) && (value[0] != '\0')) ? value : fallback;
}

bool canFdEnabled()
{
    return ::std::strcmp(envOrDefault("OPENBSW_CAN_FD", DEFAULT_CAN_FD), "0") != 0;
}

// clang-format off
::can::SocketCanTransceiver::DeviceConfig const can0Config
{
    envOrDefault("OPENBSW_CAN0_IF", "vcan0"), ::busid::CAN_0,
    canFdEnabled(), /*enableCanFd*/
    canFdEnabled()  /*enableBitRateSwitch*/
};

::can::SocketCanTransceiver::DeviceConfig const can1Config
{
    envOrDefault("OPENBSW_CAN1_IF", "vcan1"), ::busid::CAN_1,
    canFdEnabled(), /*enableCanFd*/
    canFdEnabled()  /*enableBitRateSwitch*/
};
// clang-format on
} // namespace

CanSystem::CanSystem(::async::ContextType context)
: _timeout(), _context(context), _can0Transceiver(can0Config), _can1Transceiver(can1Config)
{
    setTransitionContext(context);
}

void CanSystem::init() { transitionDone(); }

void CanSystem::run()
{
    // NOLINTBEGIN(cppcoreguidelines-pro-type-vararg): Logger API is variadic by design.
    Logger::info(
        CAN,
        "CAN_0 on %s, CAN_1 on %s, CAN FD %s",
        can0Config.name,
        can1Config.name,
        can0Config.enableCanFd ? "on" : "off");
    // NOLINTEND(cppcoreguidelines-pro-type-vararg)
    _can0Transceiver.init();
    _can0Transceiver.open();
    _can1Transceiver.init();
    _can1Transceiver.open();
    ::async::scheduleAtFixedRate(
        _context, *this, _timeout, TIMEOUT_CAN_SYSTEM_IN_MS, ::async::TimeUnit::MILLISECONDS);
    transitionDone();
}

void CanSystem::shutdown()
{
    _timeout.cancel();
    _can1Transceiver.close();
    _can1Transceiver.shutdown();
    _can0Transceiver.close();
    _can0Transceiver.shutdown();
    transitionDone();
}

::can::ICanTransceiver* CanSystem::getCanTransceiver(uint8_t busId)
{
    if (busId == ::busid::CAN_0)
    {
        return &_can0Transceiver;
    }
    if (busId == ::busid::CAN_1)
    {
        return &_can1Transceiver;
    }
    return nullptr;
}

void CanSystem::execute()
{
    _can0Transceiver.run(MAX_SENT_PER_RUN, MAX_RECEIVED_PER_RUN);
    _can1Transceiver.run(MAX_SENT_PER_RUN, MAX_RECEIVED_PER_RUN);
}

} // namespace systems
