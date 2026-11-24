/********************************************************************************
 * Copyright (c) 2024 Accenture
 *
 * This program and the accompanying materials are made available under the
 * terms of the Apache License Version 2.0 which is available at
 * https://www.apache.org/licenses/LICENSE-2.0
 *
 * SPDX-License-Identifier: Apache-2.0
 ********************************************************************************/

#include "lifecycle/StaticBsp.h"

#include <async/AsyncBinding.h>
#include <async/Config.h>
#include <etl/alignment.h>
#include <lifecycle/LifecycleManager.h>
#include <safeSupervisor/SafeSupervisor.h>

#include <csignal>
#include <ctime>
#include <unistd.h>

#ifdef PLATFORM_SUPPORT_CAN
#include "systems/CanSystem.h"
#endif // PLATFORM_SUPPORT_CAN

extern void terminal_cleanup(void);
extern void main_thread_setup(void);
#ifdef PLATFORM_SUPPORT_ETHERNET
#include "systems/TapEthernetSystem.h"
#endif // PLATFORM_SUPPORT_ETHERNET

extern void app_main();

namespace platform
{
StaticBsp staticBsp;

StaticBsp& getStaticBsp() { return staticBsp; }

/**
 * Called by the application's idle handler once per iteration. Without it the idle task
 * busy-polls (logger, console, ...) and the POSIX port keeps one host core fully loaded.
 * Sleeping for one system tick caps the polling rate at the tick rate. The tick signal
 * still preempts the sleeping idle thread, so the other tasks are not affected.
 */
void platformIdle()
{
    timespec const tick{0, static_cast<long>(ASYNC_CONFIG_TICK_IN_US) * 1000L};
    (void)nanosleep(&tick, nullptr);
}

#ifdef PLATFORM_SUPPORT_CAN
::etl::typed_storage<::systems::CanSystem> canSystem;
#endif // PLATFORM_SUPPORT_CAN

#ifdef PLATFORM_SUPPORT_ETHERNET
::etl::typed_storage<::systems::TapEthernetSystem> tapEthernetSystem;
#endif // PLATFORM_SUPPORT_ETHERNET

void platformLifecycleAdd(::lifecycle::LifecycleManager& lifecycleManager, uint8_t const level)
{
    (void)lifecycleManager;
    if (level == 2)
    {
#ifdef PLATFORM_SUPPORT_CAN
        lifecycleManager.addComponent("can", canSystem.create(TASK_CAN), level);
#endif // PLATFORM_SUPPORT_CAN
#ifdef PLATFORM_SUPPORT_ETHERNET
        lifecycleManager.addComponent("eth", tapEthernetSystem.create(TASK_ETHERNET), level);
#endif // PLATFORM_SUPPORT_ETHERNET
    }
}

} // namespace platform

#ifdef PLATFORM_SUPPORT_CAN
namespace systems
{
::can::ICanSystem& getCanSystem() { return *::platform::canSystem; }
} // namespace systems
#endif // PLATFORM_SUPPORT_CAN

#ifdef PLATFORM_SUPPORT_ETHERNET
namespace systems
{
::ethernet::IEthernetDriverSystem& getEthernetSystem() { return *::platform::tapEthernetSystem; }
} // namespace systems
#endif // PLATFORM_SUPPORT_ETHERNET

extern "C"
{
void putchar_(char character) { putchar(character); }
}

void intHandler(int /* sig */)
{
    terminal_cleanup();
    _exit(0);
}

int main()
{
    signal(SIGINT, intHandler);
    main_thread_setup();
    ::safety::safeSupervisorConstructor.construct();
    ::platform::staticBsp.init();
    app_main(); // entry point for the generic part
    return (1); // we never reach this point
}
