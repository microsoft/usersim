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

static_assert(sizeof(cxplat_spin_lock_t) == sizeof(KSPIN_LOCK_QUEUE), "Size mismatch");
static_assert(sizeof(cxplat_lock_queue_handle_t) == sizeof(KLOCK_QUEUE_HANDLE), "Size mismatch");

_Requires_lock_not_held_(*lock_handle) _Acquires_lock_(*lock_handle) _Post_same_lock_(*spin_lock, *lock_handle)
    _IRQL_requires_max_(DISPATCH_LEVEL) _IRQL_saves_global_(QueuedSpinLock, lock_handle)
        _IRQL_raises_(DISPATCH_LEVEL) void cxplat_acquire_in_stack_queued_spin_lock(
            _Inout_ cxplat_spin_lock_t* spin_lock, _Out_ cxplat_lock_queue_handle_t* lock_handle)
{
    KeAcquireInStackQueuedSpinLock((PKSPIN_LOCK)spin_lock, (PKLOCK_QUEUE_HANDLE)lock_handle);
}

_Requires_lock_held_(*lock_handle) _Releases_lock_(*lock_handle) _IRQL_requires_(DISPATCH_LEVEL)
    _IRQL_restores_global_(QueuedSpinLock, lock_handle) void cxplat_release_in_stack_queued_spin_lock(
        _In_ cxplat_lock_queue_handle_t* lock_handle)
{
    KeReleaseInStackQueuedSpinLock((PKLOCK_QUEUE_HANDLE)lock_handle);
}

_Requires_lock_not_held_(*lock_handle) _Acquires_lock_(*lock_handle) _Post_same_lock_(*spin_lock, *lock_handle)
    _IRQL_requires_max_(DISPATCH_LEVEL) _IRQL_saves_global_(QueuedSpinLock, lock_handle)
        _IRQL_raises_(DISPATCH_LEVEL) void cxplat_acquire_in_stack_queued_spin_lock_at_dpc(
            _Inout_ cxplat_spin_lock_t* spin_lock, _Out_ cxplat_lock_queue_handle_t* lock_handle)
{
    KeAcquireInStackQueuedSpinLockAtDpcLevel((PKSPIN_LOCK)spin_lock, (PKLOCK_QUEUE_HANDLE)lock_handle);
}

_Requires_lock_held_(*lock_handle) _Releases_lock_(*lock_handle) _IRQL_requires_(DISPATCH_LEVEL)
    _IRQL_restores_global_(QueuedSpinLock, lock_handle) void cxplat_release_in_stack_queued_spin_lock_from_dpc(
        _In_ cxplat_lock_queue_handle_t* lock_handle)
{
    KeReleaseInStackQueuedSpinLockFromDpcLevel((PKLOCK_QUEUE_HANDLE)lock_handle);
}
