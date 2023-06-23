// Copyright (c) Microsoft Corporation
// SPDX-License-Identifier: MIT

#include "fault_injection.h"
#include "kernel_um.h"
#include "platform.h"

#include <stdio.h>

unsigned long __cdecl DbgPrintEx(
    _In_ unsigned long component_id, _In_ unsigned long level, _In_z_ _Printf_format_string_ PCSTR format, ...)
{
    UNREFERENCED_PARAMETER(component_id);
    UNREFERENCED_PARAMETER(level);
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    return MAXULONG32;
}

void
FatalListEntryError(_In_ void* p1, _In_ void* p2, _In_ void* p3)
{
    UNREFERENCED_PARAMETER(p1);
    UNREFERENCED_PARAMETER(p2);
    UNREFERENCED_PARAMETER(p3);
    usersim_assert("FatalListEntryError");
}