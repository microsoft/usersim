// Copyright (c) Microsoft Corporation
// SPDX-License-Identifier: MIT
#pragma once
#if !defined(_AMD64_) && defined(_M_AMD64)
#define _AMD64_
#endif
#include <ntdef.h> // for NTSTATUS
#include <ntstatus.h>

#define CXPLAT_RUNTIME_ASSERT(x) NT_ASSERT(x)
#ifdef NDEBUG
#define CXPLAT_DEBUG_ASSERT(x) (void)(x)
#else
#define CXPLAT_DEBUG_ASSERT(x) NT_ASSERT(x)
#endif //! NDEBUG

// Map specific cxplat_status_t values to HRESULT values.
#define CXPLAT_PLATFORM_STATUS_SUCCESS STATUS_SUCCESS
#define CXPLAT_PLATFORM_STATUS_NO_MEMORY STATUS_NO_MEMORY
#define CXPLAT_PLATFORM_STATUS_ARITHMETIC_OVERFLOW STATUS_INTEGER_OVERFLOW
#define CXPLAT_PLATFORM_STATUS_INVALID_STATE STATUS_INVALID_STATE_TRANSITION

#define CXPLAT_SUCCEEDED(status) NT_SUCCESS(status)

typedef struct _EX_RUNDOWN_REF cxplat_rundown_reference_t;

typedef struct _LOOKASIDE_LIST_EX cxplat_lookaside_list_t;
