// Copyright (c) Microsoft Corporation
// SPDX-License-Identifier: MIT
#pragma once
#include <assert.h>
#include <winerror.h>

#define CXPLAT_RUNTIME_ASSERT(x) assert(x)
#ifdef NDEBUG // Release build.
#define CXPLAT_DEBUG_ASSERT(x) (void)(x)
#else // Debug build.
#define CXPLAT_DEBUG_ASSERT(x) assert(x)
#define CXPLAT_DEBUGGING_FEATURES_ENABLED
#endif //! NDEBUG

// Map specific cxplat_status_t values to HRESULT values.
#define CXPLAT_PLATFORM_STATUS_SUCCESS S_OK
#define CXPLAT_PLATFORM_STATUS_NO_MEMORY __HRESULT_FROM_WIN32(ERROR_OUTOFMEMORY)
#define CXPLAT_PLATFORM_STATUS_ARITHMETIC_OVERFLOW __HRESULT_FROM_WIN32(ERROR_ARITHMETIC_OVERFLOW)
#define CXPLAT_PLATFORM_STATUS_INVALID_STATE __HRESULT_FROM_WIN32(ERROR_INVALID_STATE)
#define CXPLAT_PLATFORM_STATUS_NOT_FOUND __HRESULT_FROM_WIN32(ERROR_NOT_FOUND)

#define CXPLAT_SUCCEEDED(status) SUCCEEDED((HRESULT)(status))

typedef struct cxplat_rundown_reference_t
{
    void* reserved;
} cxplat_rundown_reference_t;
