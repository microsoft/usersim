// Copyright (c) Microsoft Corporation
// SPDX-License-Identifier: MIT
#pragma once

#include "cxplat_common.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @brief Initialize fault injection.  This must be called before any other
     * fault injection functions. This function is not thread safe.
     *
     * @param[in] stack_depth Number of stack frames to capture when a fault is
     * injected.
     * @retval CXPLAT_STATUS_SUCCESS The operation was successful.
     * @retval CXPLAT_STATUS_OUT_OF_MEMORY Operation failed due to memory allocation failure.
     */
    cxplat_status_t
    cxplat_fault_injection_initialize(size_t stack_depth) NOEXCEPT;

    /**
     * @brief Uninitialize fault injection. This must be called after all other
     * fault injection functions. This function is not thread safe.
     */
    void
    cxplat_fault_injection_uninitialize() NOEXCEPT;

    /**
     * @brief Enable fault injection. This function is thread safe.
     *
     * @retval true Fault should be injected.
     * @retval false Fault should not be injected.
     */
    bool
    cxplat_fault_injection_inject_fault() NOEXCEPT;

    /**
     * @brief Test if fault injection is enabled. This function is thread safe.
     *
     * @retval true Fault injection is enabled.
     * @retval false Fault injection is disabled.
     */
    bool
    cxplat_fault_injection_is_enabled() NOEXCEPT;

#ifdef __cplusplus
}
#endif
