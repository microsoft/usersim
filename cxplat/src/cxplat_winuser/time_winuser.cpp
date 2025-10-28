// Copyright (c) Microsoft Corporation
// SPDX-License-Identifier: MIT
#include "cxplat.h"
#include "realtimeapiset.h"

#pragma comment(lib, "Mincore.lib")

uint64_t
cxplat_query_time_since_boot_precise(bool include_suspended_time)
{
    uint64_t qpc_time;
    if (include_suspended_time) {
        // KeQueryUnbiasedInterruptTimePrecise returns the current interrupt-time count in 100-nanosecond units.
        // Unbiased Interrupt time is the total time since boot including time spent suspended.
        // https://docs.microsoft.com/en-us/windows-hardware/drivers/ddi/wdm/nf-wdm-kequeryunbiasedinterrupttimeprecise
        QueryUnbiasedInterruptTimePrecise(&qpc_time);
    } else {
        // KeQueryInterruptTimePrecise returns the current interrupt-time count in 100-nanosecond units.
        // (Biased) Interrupt time is the total time since boot excluding time spent suspended.
        // https://docs.microsoft.com/en-us/windows-hardware/drivers/ddi/wdm/nf-wdm-kequeryinterrupttimeprecise
        QueryInterruptTimePrecise(&qpc_time);
    }
    return qpc_time;
}

uint64_t
cxplat_query_time_since_boot_approximate(bool include_suspended_time)
{
    uint64_t qpc_time;
    if (include_suspended_time) {
        // KeQueryUnbiasedInterruptTime returns the current interrupt-time count in 100-nanosecond units.
        // Unbiased Interrupt time is the total time since boot including time spent suspended.
        // https://docs.microsoft.com/en-us/windows-hardware/drivers/ddi/wdm/nf-wdm-kequeryunbiasedinterrupttime
        QueryUnbiasedInterruptTime(&qpc_time);
    } else {
        // KeQueryInterruptTimePrecise returns the current interrupt-time count in 100-nanosecond units.
        // (Biased) Interrupt time is the total time since boot excluding time spent suspended.
        // https://docs.microsoft.com/en-us/windows-hardware/drivers/ddi/wdm/nf-wdm-kequeryinterrupttime
        QueryInterruptTime(&qpc_time);
    }

    return qpc_time;
}
