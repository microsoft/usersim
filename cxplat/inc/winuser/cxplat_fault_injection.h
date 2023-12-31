// Copyright (c) Microsoft Corporation
// SPDX-License-Identifier: MIT
#pragma once
#include "cxplat_common.h"

#ifndef CXPLAT_DEBUGGING_FEATURES_ENABLED
#define cxplat_fault_injection_is_enabled() false
#define cxplat_fault_injection_inject_fault() false
#else
#include <stdbool.h>

CXPLAT_EXTERN_C_BEGIN

/**
 * @brief Initialize fault injection.  This must be called before any other
 * fault injection functions. This function is not thread safe.
 *
 * @param[in] stack_depth Number of stack frames to capture when a fault is
 * injected.
 *
 * @retval CXPLAT_STATUS_SUCCESS The operation was successful.
 * @retval CXPLAT_STATUS_OUT_OF_MEMORY Operation failed due to memory allocation failure.
 * @retval CXPLAT_STATUS_INVALID_STATE Operation failed due to invalid state (e.g. already initialized).
 */
cxplat_status_t
cxplat_fault_injection_initialize(size_t stack_depth) CXPLAT_NOEXCEPT;

/**
 * @brief Uninitialize fault injection. This must be called after all other
 * fault injection functions. This function is not thread safe.
 */
void
cxplat_fault_injection_uninitialize() CXPLAT_NOEXCEPT;

/**
 * @brief Enable fault injection. This function is thread safe.
 *
 * @retval true Fault should be injected.
 * @retval false Fault should not be injected.
 */
bool
cxplat_fault_injection_inject_fault() CXPLAT_NOEXCEPT;

/**
 * @brief Test if fault injection is enabled. This function is thread safe.
 *
 * @retval true Fault injection is enabled.
 * @retval false Fault injection is disabled.
 */
bool
cxplat_fault_injection_is_enabled() CXPLAT_NOEXCEPT;

/**
 * @brief Reset fault injection. This function is thread safe.
 */
void
cxplat_fault_injection_reset() CXPLAT_NOEXCEPT;

/**
 * @brief Add a module to the fault injection list. This function is thread safe.
 *
 * @param[in] module_handle
 * @retval CXPLAT_STATUS_SUCCESS The operation was successful.
 * @retval CXPLAT_STATUS_OUT_OF_MEMORY Operation failed due to memory allocation failure.
 * @retval CXPLAT_STATUS_INVALID_STATE Operation failed due to invalid state (e.g. already added).
 */
cxplat_status_t
cxplat_fault_injection_add_module(_In_ void* module_handle) CXPLAT_NOEXCEPT;

/**
 * @brief Remove a module from the fault injection list. This function is thread safe.
 *
 * @param[in] module_handle
 * @retval CXPLAT_STATUS_SUCCESS The operation was successful.
 * @retval CXPLAT_STATUS_OUT_OF_MEMORY Operation failed due to memory allocation failure.
 * @retval CXPLAT_STATUS_INVALID_STATE Operation failed due to invalid state (e.g. already removed).
 */
cxplat_status_t
cxplat_fault_injection_remove_module(_In_ void* module_handle) CXPLAT_NOEXCEPT;

CXPLAT_EXTERN_C_END
#endif
