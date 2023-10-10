// Copyright (c) Microsoft Corporation
// SPDX-License-Identifier: MIT
#pragma once

#include "cxplat_common.h"
#include "cxplat_memory.h"
#include "cxplat_processor.h"
#include "cxplat_rundown.h"
#include "cxplat_size.h"
#include "cxplat_workitem.h"

CXPLAT_EXTERN_C_BEGIN

cxplat_status_t
cxplat_initialize();

void
cxplat_cleanup();

CXPLAT_EXTERN_C_END
