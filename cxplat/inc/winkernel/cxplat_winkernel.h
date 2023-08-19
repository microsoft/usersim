// Copyright (c) Microsoft Corporation
// SPDX-License-Identifier: MIT
#pragma once
#include <ntdef.h>
#include <ntstatus.h>

// Map specific cxplat_status_t values to HRESULT values.
#define CXPLAT_PLATFORM_STATUS_SUCCESS STATUS_SUCCESS
#define CXPLAT_PLATFORM_STATUS_NO_MEMORY STATUS_NO_MEMORY

#define CXPLAT_SUCCEEDED(status) NT_SUCCESS(status)
