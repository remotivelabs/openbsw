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

namespace systems
{
static uint32_t const TIMEOUT_CAN_SYSTEM_IN_MS = 1U;
static int const MAX_SENT_PER_RUN              = 3;
static int const MAX_RECEIVED_PER_RUN          = 3;

namespace
{

#if defined(PLATFORM_CAN0_USE_FD)
bool const USE_CAN_FD = true;
#else
bool const USE_CAN_FD = false;
#endif

// clang-format off
::can::SocketCanTransceiver::DeviceConfig can0Config
{
    "vcan0", ::busid::CAN_0,
    USE_CAN_FD, /*enableCanFd*/
    USE_CAN_FD  /*enableBitRateSwitch*/
};

::can::SocketCanTransceiver::DeviceConfig can1Config
{
    "vcan1", ::busid::CAN_1,
    USE_CAN_FD, /*enableCanFd*/
    USE_CAN_FD  /*enableBitRateSwitch*/
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
