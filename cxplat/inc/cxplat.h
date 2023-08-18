// Copyright (c) Microsoft Corporation
// SPDX-License-Identifier: MIT
#pragma once

#include "cxplat_fault_injection.h"
#include "cxplat_memory.h"

#ifdef __cplusplus
extern "C"
{
#endif

    int // 0 on success, errno value on failure.
    cxplat_initialize();

    void
    cxplat_cleanup();

#ifdef __cplusplus
};
#endif
