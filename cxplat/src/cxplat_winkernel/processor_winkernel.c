// Copyright (c) Microsoft Corporation
// SPDX-License-Identifier: MIT
#include "cxplat.h"

#include <wdm.h>

_Must_inspect_result_ uint32_t
cxplat_get_maximum_processor_count()
{
    return KeQueryMaximumProcessorCountEx(ALL_PROCESSOR_GROUPS);
}

_Must_inspect_result_ uint32_t
cxplat_get_current_processor_number()
{
    return KeGetCurrentProcessorIndex();
}

_Must_inspect_result_ uint32_t
cxplat_get_active_processor_count()
{
    return KeQueryActiveProcessorCountEx(ALL_PROCESSOR_GROUPS);
}
