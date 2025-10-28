// Copyright (c) Microsoft Corporation
// SPDX-License-Identifier: MIT
#include "cxplat.h"
#include "wdm.h"

uint64_t
cxplat_query_time_since_boot_precise(bool include_suspended_time)
{
    uint64_t qpc_time;
    if (include_suspended_time) {
        // KeQueryUnbiasedInterruptTimePrecise returns the current interrupt-time count in 100-nanosecond units.
        // Unbiased Interrupt time is the total time since boot including time spent suspended.
        // https://docs.microsoft.com/en-us/windows-hardware/drivers/ddi/wdm/nf-wdm-kequeryunbiasedinterrupttimeprecise
        return KeQueryUnbiasedInterruptTimePrecise(&qpc_time);
    } else {
        // KeQueryInterruptTimePrecise returns the current interrupt-time count in 100-nanosecond units.
        // (Biased) Interrupt time is the total time since boot excluding time spent suspended.
        // https://docs.microsoft.com/en-us/windows-hardware/drivers/ddi/wdm/nf-wdm-kequeryinterrupttimeprecise
        return KeQueryInterruptTimePrecise(&qpc_time);
    }
}

uint64_t
cxplat_query_time_since_boot_approximate(bool include_suspended_time)
{
    if (include_suspended_time) {
        // KeQueryUnbiasedInterruptTime returns the current interrupt-time count in 100-nanosecond units.
        // Unbiased Interrupt time is the total time since boot including time spent suspended.
        // https://docs.microsoft.com/en-us/windows-hardware/drivers/ddi/wdm/nf-wdm-kequeryunbiasedinterrupttime
        return KeQueryUnbiasedInterruptTime();
    } else {
        // KeQueryInterruptTimePrecise returns the current interrupt-time count in 100-nanosecond units.
        // (Biased) Interrupt time is the total time since boot excluding time spent suspended.
        // https://docs.microsoft.com/en-us/windows-hardware/drivers/ddi/wdm/nf-wdm-kequeryinterrupttime
        return KeQueryInterruptTime();
    }
}
