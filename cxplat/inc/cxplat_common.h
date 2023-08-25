// Copyright (c) Microsoft Corporation
// SPDX-License-Identifier: MIT
#pragma once

// Include platform-specific definitions of common defines.
#include "cxplat_platform.h"

#include <assert.h>

#define CXPLAT_RUNTIME_ASSERT(x) assert(x)
#ifdef NDEBUG
#define CXPLAT_DEBUG_ASSERT(x) (void)(x)
#else
#define CXPLAT_DEBUG_ASSERT(x) assert(x)
#endif //! NDEBUG

#ifdef __cplusplus
#define CXPLAT_NOEXCEPT noexcept
#define CXPLAT_EXTERN_C extern "C"
#define CXPLAT_EXTERN_C_BEGIN \
    extern "C"                \
    {
#define CXPLAT_EXTERN_C_END }
#else
#define CXPLAT_NOEXCEPT
#define CXPLAT_EXTERN_C
#define CXPLAT_EXTERN_C_BEGIN
#define CXPLAT_EXTERN_C_END
#endif

// This enumeration is not considered strict, in that other values are permitted.
// There may be multiple values that constitute "success".  Use CXPLAT_SUCCEEDED(value)
// to determine success rather than comparing against CXPLAT_SUCCESS.
typedef _Return_type_success_(CXPLAT_SUCCEEDED(return )) enum {
    CXPLAT_STATUS_SUCCESS = CXPLAT_PLATFORM_STATUS_SUCCESS,
    CXPLAT_STATUS_NO_MEMORY = CXPLAT_PLATFORM_STATUS_NO_MEMORY,
    CXPLAT_STATUS_ARITHMETIC_OVERFLOW = CXPLAT_PLATFORM_STATUS_ARITHMETIC_OVERFLOW,
    CXPLAT_STATUS_UNSUCCESSFUL = CXPLAT_PLATFORM_STATUS_UNSUCCESSFUL,
} cxplat_status_t;
