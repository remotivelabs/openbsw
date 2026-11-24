/********************************************************************************
 * Copyright (c) 2025 Accenture
 *
 * This program and the accompanying materials are made available under the
 * terms of the Apache License Version 2.0 which is available at
 * https://www.apache.org/licenses/LICENSE-2.0
 *
 * SPDX-License-Identifier: Apache-2.0
 ********************************************************************************/

#pragma once

#include "uds/lifecycle/IUdsLifecycleConnector.h"

#include <lifecycle/LifecycleManager.h>

namespace uds
{
/**
 * Connects the UDS ECU Reset service (0x11) to the lifecycle. A shutdown request takes the
 * system down to run level 0; the application then performs the software reset once that level
 * is reached (see the lifecycle monitor in app.cpp).
 */
class UdsLifecycleConnector : public IUdsLifecycleConnector
{
public:
    explicit UdsLifecycleConnector(lifecycle::LifecycleManager& lifecycleManager)
    : _lifecycleManager(lifecycleManager)
    {}

    bool isModeChangePossible() const { return true; }

    bool requestPowerdown(bool /* rapid */, uint8_t& /* time */) { return true; }

    bool requestShutdown(ShutdownType /* type */, uint32_t /* timeout */)
    {
        _lifecycleManager.transitionToLevel(0U);
        return true;
    }

private:
    lifecycle::LifecycleManager& _lifecycleManager;
};

} // namespace uds
