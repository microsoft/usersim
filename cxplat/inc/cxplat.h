// Copyright (c) Microsoft Corporation
// SPDX-License-Identifier: MIT
#pragma once
#include "cxplat/common.h"

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