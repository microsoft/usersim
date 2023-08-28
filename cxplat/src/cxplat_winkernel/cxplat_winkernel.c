// Copyright (c) Microsoft Corporation
// SPDX-License-Identifier: MIT

// This file contains initialization/cleanup routines for the Windows kernel-mode cxplat library.
#include "cxplat.h"

static ULONG _cxplat_initialization_count = 0;

cxplat_status_t
cxplat_initialize()
{
    if (_cxplat_initialization_count > 0) {
        return CXPLAT_STATUS_INVALID_STATE;
    }
    _cxplat_initialization_count++;
    return CXPLAT_STATUS_SUCCESS;
}

void
cxplat_cleanup()
{
    CXPLAT_DEBUG_ASSERT(_cxplat_initialization_count > 0);
    _cxplat_initialization_count--;
}
