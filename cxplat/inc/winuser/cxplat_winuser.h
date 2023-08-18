// Copyright (c) Microsoft Corporation
// SPDX-License-Identifier: MIT
#pragma once
#include <winerror.h>

// Map specific cxplat_status_t values to HRESULT values.
#define CXPLAT_PLATFORM_STATUS_SUCCESS S_OK
#define CXPLAT_PLATFORM_STATUS_NO_MEMORY __HRESULT_FROM_WIN32(ERROR_OUTOFMEMORY)
