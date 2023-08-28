// Copyright (c) Microsoft Corporation
// SPDX-License-Identifier: MIT
#pragma once
#include "cxplat_platform.h"

CXPLAT_EXTERN_C_BEGIN

/**
 * @brief Initialize the rundown reference table entry for the given context.
 *
 * @param[in] context The address of a cxplat_rundown_reference_t structure.
 */
void
cxplat_initialize_rundown_protection(_Out_ cxplat_rundown_reference_t* rundown_reference);

/**
 * @brief Reinitialize the rundown reference table entry for the given context.
 *
 * @param[in] context The address of a previously run down cxplat_rundown_reference_t structure.
 */
void
cxplat_reinitialize_rundown_protection(_Inout_ cxplat_rundown_reference_t* rundown_reference);

/**
 * @brief Wait for the rundown reference count to reach 0 for the given context.
 *
 * @param[in] context The address of a cxplat_rundown_reference_t structure.
 */
void
cxplat_wait_for_rundown_protection_release(_Inout_ cxplat_rundown_reference_t* rundown_reference);

/**
 * @brief Acquire a rundown reference for the given context.
 *
 * @param[in] context The address of a cxplat_rundown_reference_t structure.
 * @retval TRUE Rundown has not started.
 * @retval FALSE Rundown has started.
 */
int
cxplat_acquire_rundown_protection(_Inout_ cxplat_rundown_reference_t* rundown_reference);

/**
 * @brief Release a rundown reference for the given context.
 *
 * @param[in] context The address of a cxplat_rundown_reference_t structure.
 */
void
cxplat_release_rundown_protection(_Inout_ cxplat_rundown_reference_t* rundown_reference);

CXPLAT_EXTERN_C_END
