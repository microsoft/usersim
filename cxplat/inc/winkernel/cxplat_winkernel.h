// Copyright (c) Microsoft Corporation
// SPDX-License-Identifier: MIT
#pragma once
#if !defined(_AMD64_) && defined(_M_AMD64)
#define _AMD64_
#endif
#include <ntdef.h> // for NTSTATUS
#include <ntstatus.h>

// Map specific cxplat_status_t values to HRESULT values.
#define CXPLAT_PLATFORM_STATUS_SUCCESS STATUS_SUCCESS
#define CXPLAT_PLATFORM_STATUS_NO_MEMORY STATUS_NO_MEMORY

#define CXPLAT_SUCCEEDED(status) NT_SUCCESS(status)
