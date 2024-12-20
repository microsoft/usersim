// Copyright (c) Microsoft Corporation
// SPDX-License-Identifier: MIT
#pragma once

#include <stdint.h>

CXPLAT_EXTERN_C_BEGIN

/**
 * @brief Get the current processor number.
 * @returns The current processor number.
 */
_Must_inspect_result_ uint32_t
cxplat_get_current_processor_number();

/**
 * @brief Get the number of currently active processors on the system.
 * @returns The number of currently active processors on the system.
 */
_Must_inspect_result_ uint32_t
cxplat_get_active_processor_count();

/**
 * @brief Get the maximum number of logical processors on the system.
 * @returns The maximum number of logical processors on the system.
 */
_Must_inspect_result_ uint32_t
cxplat_get_maximum_processor_count();

typedef struct cxplat_spin_lock
{
    uint64_t Reserved[2];
} cxplat_queue_spin_lock_t;

typedef struct cxplat_lock_queue_handle
{
    uint64_t Reserved_1[2];
    uint8_t Reserved_2;
} cxplat_lock_queue_handle_t;

typedef uintptr_t cxplat_spin_lock_t;

typedef uint8_t cxplat_irql_t;

_Requires_lock_not_held_(*lock_handle) _Acquires_lock_(*lock_handle) _Post_same_lock_(*spin_lock, *lock_handle)
    _IRQL_requires_max_(DISPATCH_LEVEL) _IRQL_saves_global_(QueuedSpinLock, lock_handle)
        _IRQL_raises_(DISPATCH_LEVEL) void cxplat_acquire_in_stack_queued_spin_lock(
            _Inout_ cxplat_queue_spin_lock_t* spin_lock, _Out_ cxplat_lock_queue_handle_t* lock_handle);

_Requires_lock_held_(*lock_handle) _Releases_lock_(*lock_handle) _IRQL_requires_(DISPATCH_LEVEL)
    _IRQL_restores_global_(QueuedSpinLock, lock_handle) void cxplat_release_in_stack_queued_spin_lock(
        _In_ cxplat_lock_queue_handle_t* lock_handle);

_Requires_lock_not_held_(*lock_handle) _Acquires_lock_(*lock_handle) _Post_same_lock_(*spin_lock, *lock_handle)
    _IRQL_requires_max_(DISPATCH_LEVEL) _IRQL_saves_global_(QueuedSpinLock, lock_handle)
        _IRQL_raises_(DISPATCH_LEVEL) void cxplat_acquire_in_stack_queued_spin_lock_at_dpc(
            _Inout_ cxplat_queue_spin_lock_t* spin_lock, _Out_ cxplat_lock_queue_handle_t* lock_handle);

_Requires_lock_held_(*lock_handle) _Releases_lock_(*lock_handle) _IRQL_requires_(DISPATCH_LEVEL)
    _IRQL_restores_global_(QueuedSpinLock, lock_handle) void cxplat_release_in_stack_queued_spin_lock_from_dpc(
        _In_ cxplat_lock_queue_handle_t* lock_handle);

_Requires_lock_not_held_(*spin_lock) _Acquires_lock_(*spin_lock) _IRQL_requires_max_(DISPATCH_LEVEL) _IRQL_saves_
    _IRQL_raises_(DISPATCH_LEVEL)
cxplat_irql_t
cxplat_acquire_spin_lock(_Inout_ cxplat_spin_lock_t* spin_lock);

_Requires_lock_held_(*spin_lock) _Releases_lock_(*spin_lock)
    _IRQL_requires_(DISPATCH_LEVEL) void cxplat_release_spin_lock(
        _Inout_ cxplat_spin_lock_t* spin_lock, _In_ _IRQL_restores_ cxplat_irql_t old_irql);

_Requires_lock_not_held_(*spin_lock) _Acquires_lock_(*spin_lock) _IRQL_requires_min_(
    DISPATCH_LEVEL) void cxplat_acquire_spin_lock_at_dpc_level(_Inout_ cxplat_spin_lock_t* spin_lock);

_Requires_lock_held_(*spin_lock) _Releases_lock_(*spin_lock) _IRQL_requires_min_(
    DISPATCH_LEVEL) void cxplat_release_spin_lock_from_dpc_level(_Inout_ cxplat_spin_lock_t* spin_lock);

_IRQL_requires_max_(HIGH_LEVEL) _IRQL_raises_(irql) _IRQL_saves_ cxplat_irql_t
    cxplat_raise_irql(_In_ cxplat_irql_t irql);

_IRQL_requires_max_(HIGH_LEVEL) void cxplat_lower_irql(_In_ _Notliteral_ _IRQL_restores_ cxplat_irql_t irql);

_IRQL_requires_max_(HIGH_LEVEL) _IRQL_saves_ cxplat_irql_t cxplat_get_current_irql();

CXPLAT_EXTERN_C_END
