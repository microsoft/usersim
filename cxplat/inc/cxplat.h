// Copyright (c) Microsoft Corporation
// SPDX-License-Identifier: MIT
#pragma once

#include "cxplat_fault_injection.h"
#include "cxplat_memory.h"
#include "cxplat_size.h"

CXPLAT_EXTERN_C_BEGIN

cxplat_status_t
cxplat_initialize();

void
cxplat_cleanup();

CXPLAT_EXTERN_C_END
