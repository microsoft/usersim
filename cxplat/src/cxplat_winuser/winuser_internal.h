// Copyright (c) Microsoft Corporation
// SPDX-License-Identifier: MIT
#pragma once
#include <winerror.h>

#define CXPLAT_STATUS_FROM_WIN32(code) ((cxplat_status_t)__HRESULT_FROM_WIN32(code))
