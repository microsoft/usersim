// Copyright (c) Microsoft Corporation
// SPDX-License-Identifier: MIT
#pragma once

#include "cxplat_fault_injection.h"
#include "cxplat_memory.h"

#ifdef __cplusplus
extern "C"
{
#endif

    cxplat_status_t
    cxplat_initialize();

    void
    cxplat_cleanup();

#ifdef __cplusplus
};
#endif
