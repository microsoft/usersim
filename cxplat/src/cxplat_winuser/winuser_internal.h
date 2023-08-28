// Copyright (c) Microsoft Corporation
// SPDX-License-Identifier: MIT
#pragma once
#include <winerror.h>

#define CXPLAT_STATUS_FROM_WIN32(code) ((cxplat_status_t)__HRESULT_FROM_WIN32(code))

cxplat_status_t cxplat_winuser_initialize_thread_pool();

void
cxplat_winuser_clean_up_thread_pool();
