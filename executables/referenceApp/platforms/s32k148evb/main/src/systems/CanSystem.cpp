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

#include "async/Config.h"
#include "async/Hook.h"
#include "bsp/can/canConfiguration.h"
#include "busid/BusId.h"
#include "lifecycle/ILifecycleManager.h"
#include "lifecycle/StaticBsp.h"
#include "util/command/IParentCommand.h"

#include <lifecycle/StaticBsp.h>

namespace systems
{

::systems::CanSystem::CanSystem(::async::ContextType context, StaticBsp& staticBsp)
: ::lifecycle::SingleContextLifecycleComponent(context)
, ::etl::singleton_base<CanSystem>(*this)
, _context(context)
, _transceiver0(
      context,
      ::busid::CAN_0,
      Can0Config,
      staticBsp.getCanPhy(),
      staticBsp.getPowerStateController())
, _transceiver1(
      context,
      ::busid::CAN_1,
      Can1Config,
      staticBsp.getCanPhy(),
      staticBsp.getPowerStateController())
, _canRxRunnable(*this)
{}

void CanSystem::init() { transitionDone(); }

void CanSystem::run()
{
    _canRxRunnable.setEnabled(true);

    (void)_transceiver0.init();
    (void)_transceiver0.open();

    (void)_transceiver1.init();
    (void)_transceiver1.open();

    transitionDone();
}

void CanSystem::shutdown()
{
    (void)_transceiver0.close();
    _transceiver0.shutdown();

    (void)_transceiver1.close();
    _transceiver1.shutdown();

    _canRxRunnable.setEnabled(false);

    transitionDone();
}

::can::ICanTransceiver* CanSystem::getCanTransceiver(uint8_t busId)
{
    if (busId == ::busid::CAN_0)
    {
        return &_transceiver0;
    }
    if (busId == ::busid::CAN_1)
    {
        return &_transceiver1;
    }
    return nullptr;
}

void CanSystem::dispatchRxTask() { ::async::execute(_context, _canRxRunnable); }

CanSystem::CanRxRunnable::CanRxRunnable(CanSystem& parent) : _parent(parent), _enabled(false) {}

void CanSystem::CanRxRunnable::execute()
{
    if (_enabled)
    {
        _parent._transceiver0.receiveTask();
        _parent._transceiver1.receiveTask();
    }
}

} // namespace systems

extern "C"
{
void call_can_isr_RX()
{
    ::asyncEnterIsrGroup(ISR_GROUP_CAN);

    uint8_t framesReceived;
    {
        ::async::LockType const lock;
        framesReceived = bios::CanFlex2Transceiver::receiveInterrupt(0);
    }

    if (framesReceived > 0)
    {
        ::systems::CanSystem::instance().dispatchRxTask();
    }

    ::asyncLeaveIsrGroup(ISR_GROUP_CAN);
}

void call_can_isr_TX()
{
    ::asyncEnterIsrGroup(ISR_GROUP_CAN);
    bios::CanFlex2Transceiver::transmitInterrupt(0);
    ::asyncLeaveIsrGroup(ISR_GROUP_CAN);
}

// FlexCAN1 has driver index 1 (see FlexCANDevice: base address 0x40025000).
void call_can1_isr_RX()
{
    ::asyncEnterIsrGroup(ISR_GROUP_CAN);

    uint8_t framesReceived;
    {
        ::async::LockType const lock;
        framesReceived = bios::CanFlex2Transceiver::receiveInterrupt(1);
    }

    if (framesReceived > 0)
    {
        ::systems::CanSystem::instance().dispatchRxTask();
    }

    ::asyncLeaveIsrGroup(ISR_GROUP_CAN);
}

void call_can1_isr_TX()
{
    ::asyncEnterIsrGroup(ISR_GROUP_CAN);
    bios::CanFlex2Transceiver::transmitInterrupt(1);
    ::asyncLeaveIsrGroup(ISR_GROUP_CAN);
}
}
