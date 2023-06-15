// Copyright (c) Microsoft Corporation
// SPDX-License-Identifier: MIT

#pragma once
#include "..\src\platform.h"

#if defined(__cplusplus)
extern "C"
{
#endif

    typedef uint8_t KIRQL;

    typedef KIRQL* PKIRQL;

    _Requires_lock_not_held_(*spin_lock) _Acquires_lock_(*spin_lock) _IRQL_requires_max_(DISPATCH_LEVEL)
    void KeAcquireSpinLock(_Inout_ PKSPIN_LOCK spin_lock, _Out_ PKIRQL old_irql);

    void
    KeEnterCriticalRegion(void);

    void
    KeLeaveCriticalRegion(void);

    void
    KeInitializeSpinLock(_Out_ PKSPIN_LOCK spin_lock);

    _Requires_lock_not_held_(*spin_lock) _Acquires_lock_(*spin_lock) _IRQL_requires_max_(DISPATCH_LEVEL) KIRQL
    KeAcquireSpinLockRaiseToDpc(_Inout_ PKSPIN_LOCK spin_lock);

    _Requires_lock_held_(*spin_lock) _Releases_lock_(*spin_lock) _IRQL_requires_(DISPATCH_LEVEL)
    void KeReleaseSpinLock(
        _Inout_ PKSPIN_LOCK spin_lock, _In_ _IRQL_restores_ KIRQL new_irql);

    KIRQL
    KeGetCurrentIrql();

    VOID
    KeRaiseIrql(_In_ KIRQL new_irql, _Out_ PKIRQL old_irql);

    KIRQL
    KeRaiseIrqlToDpcLevel();

    void
    KeLowerIrql(_In_ KIRQL new_irql);

    NTKERNELAPI
    ULONG
    KeQueryMaximumProcessorCountEx(_In_ USHORT group_number);

    unsigned long long
    KeQueryInterruptTime();

#if defined(__cplusplus)
}
#endif
