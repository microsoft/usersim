// Copyright (c) Microsoft Corporation
// SPDX-License-Identifier: MIT
#pragma once
#include <stdbool.h>

/**
 * @brief Test if fault injection is enabled. This function is thread safe.
 *
 * @retval true Fault injection is enabled.
 * @retval false Fault injection is disabled.
 */
inline bool
cxplat_fault_injection_is_enabled()
{
    return false;
}
