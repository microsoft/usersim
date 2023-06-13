// Copyright (c) Microsoft Corporation
// SPDX-License-Identifier: MIT

#include "platform.h"
#include "kernel_um.h"
#include "ke.h"

// Ke* functions.

NTKERNELAPI
ULONG
KeQueryMaximumProcessorCountEx(_In_ USHORT GroupNumber)
{
    return GetMaximumProcessorCount(GroupNumber);
}

thread_local KIRQL _usersim_current_irql = PASSIVE_LEVEL;

KIRQL
KeGetCurrentIrql() 
{
    return _usersim_current_irql;
}

VOID
KeRaiseIrql(_In_ KIRQL NewIrql, _Out_ PKIRQL OldIrql)
{
    *OldIrql = KeGetCurrentIrql();
    _usersim_current_irql = NewIrql;
}

KIRQL
KeRaiseIrqlToDpcLevel()
{
    KIRQL old_irql;
    KeRaiseIrql(DISPATCH_LEVEL, &old_irql);
    return old_irql;
}

void
KeLowerIrql(_In_ KIRQL NewIrql)
{
    _usersim_current_irql = NewIrql;
}

void
KeEnterCriticalRegion(void)
{
}

void
KeLeaveCriticalRegion(void)
{
}

void
KeInitializeSpinLock(_Out_ PKSPIN_LOCK spin_lock)
{
    auto lock = reinterpret_cast<SRWLOCK*>(spin_lock);
    *lock = SRWLOCK_INIT;
}


_Requires_lock_not_held_(*spin_lock) _Acquires_lock_(*spin_lock)
    _IRQL_requires_max_(DISPATCH_LEVEL) void KeAcquireSpinLock(_Inout_ PKSPIN_LOCK spin_lock, _Out_ PKIRQL OldIrql)
{
    *(OldIrql) = KeAcquireSpinLockRaiseToDpc(spin_lock);
}

_Requires_lock_not_held_(*spin_lock) _Acquires_lock_(*spin_lock) _IRQL_requires_max_(DISPATCH_LEVEL) KIRQL
KeAcquireSpinLockRaiseToDpc(_Inout_ PKSPIN_LOCK spin_lock)
{
    // Skip Fault Injection.
    auto lock = reinterpret_cast<SRWLOCK*>(spin_lock);
    AcquireSRWLockExclusive(lock);
    return KeRaiseIrqlToDpcLevel();
}

_Requires_lock_held_(*spin_lock) _Releases_lock_(*spin_lock) _IRQL_requires_(DISPATCH_LEVEL) void
KeReleaseSpinLock(
    _Inout_ PKSPIN_LOCK spin_lock, _In_ _IRQL_restores_ KIRQL new_irql)
{
    KeLowerIrql(new_irql);
    auto lock = reinterpret_cast<SRWLOCK*>(spin_lock);
    ReleaseSRWLockExclusive(lock);
}

unsigned long long
KeQueryInterruptTime()
{
    unsigned long long time = 0;
    QueryInterruptTime(&time);

    return time;
}