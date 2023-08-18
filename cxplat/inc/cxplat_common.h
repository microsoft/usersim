// Copyright (c) Microsoft Corporation
// SPDX-License-Identifier: MIT
#pragma once

// Include platform-specific definitions of common defines.
#include "cxplat_platform.h"

#include <assert.h>

#define CXPLAT_RUNTIME_ASSERT(x) assert(x)
#ifdef _DEBUG
#define CXPLAT_DEBUG_ASSERT(x) assert(x)
#else
#define CXPLAT_DEBUG_ASSERT(x) (void)(x)
#endif //!_DEBUG

#ifdef __cplusplus
#define NOEXCEPT noexcept
#else
#define NOEXCEPT
#endif

// This enumeration is not considered strict, in that other values are permitted.
// There may be multiple values that constitute "success".  Use CXPLAT_SUCCEEDED(value)
// to determine success rather than comparing against CXPLAT_SUCCESS.
typedef enum
{
    CXPLAT_STATUS_SUCCESS = CXPLAT_PLATFORM_STATUS_SUCCESS,
    CXPLAT_STATUS_NO_MEMORY = CXPLAT_PLATFORM_STATUS_NO_MEMORY,
} cxplat_status_t;
