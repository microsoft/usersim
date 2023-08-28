// Copyright (c) Microsoft Corporation
// SPDX-License-Identifier: MIT
#include "cxplat.h"
#include <ntddk.h>

// TODO(issue #90): convert these to macros or inline functions
void
cxplat_initialize_rundown_protection(_Out_ cxplat_rundown_reference_t* rundown_reference)
{
    ExInitializeRundownProtection(rundown_reference);
}

void
cxplat_reinitialize_rundown_protection(_Inout_ cxplat_rundown_reference_t* rundown_reference)
{
    ExReInitializeRundownProtection(rundown_reference);
}

void
cxplat_wait_for_rundown_protection_release(_Inout_ cxplat_rundown_reference_t* rundown_reference)
{
    ExWaitForRundownProtectionRelease(rundown_reference);
}

int
cxplat_acquire_rundown_protection(_Inout_ cxplat_rundown_reference_t* rundown_reference)
{
    return (int)ExAcquireRundownProtection(rundown_reference);
}

void
cxplat_release_rundown_protection(_Inout_ cxplat_rundown_reference_t* rundown_reference)
{
    ExReleaseRundownProtection(rundown_reference);
}

