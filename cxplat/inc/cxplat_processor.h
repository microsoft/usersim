// Copyright (c) Microsoft Corporation
// SPDX-License-Identifier: MIT
#pragma once

#include <stdint.h>

CXPLAT_EXTERN_C_BEGIN

/**
 * @brief Get the current processor number.
 * @returns The current processor number.
 */
_Must_inspect_result_ uint32_t
cxplat_get_current_processor_number();

/**
 * @brief Get the maximum number of logical processors on the system.
 * @returns The maximum number of logical processors on the system.
 */
_Must_inspect_result_ uint32_t
cxplat_get_maximum_processor_count();

CXPLAT_EXTERN_C_END
