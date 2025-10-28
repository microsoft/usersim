// Copyright (c) Microsoft Corporation
// SPDX-License-Identifier: MIT
#include "../tags.h"
#include "cxplat.h"
#include "winuser_internal.h"

#include <windows.h>
#include <winternl.h>

typedef struct cxplat_processor_group_info_t
{
    KAFFINITY Mask;  // Bit mask of active processors in the group.
    uint32_t Offset; // Base process index offset this group starts at.
} cxplat_processor_group_info_t;

static cxplat_processor_group_info_t* _cxplat_processor_group_info = nullptr;

static _IRQL_requires_max_(PASSIVE_LEVEL) _Must_inspect_result_ cxplat_status_t _get_processor_group_info(
    _In_ LOGICAL_PROCESSOR_RELATIONSHIP relationship,
    _Outptr_ _At_(*buffer, __drv_allocatesMem(Mem)) _Pre_defensive_ PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX* buffer,
    _Out_ PDWORD buffer_length)
{
    *buffer_length = 0;
    GetLogicalProcessorInformationEx(relationship, nullptr, buffer_length);
    if (*buffer_length == 0) {
        return CXPLAT_STATUS_FROM_WIN32(GetLastError());
    }

    *buffer = (PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX)cxplat_allocate(
        CXPLAT_POOL_FLAG_NON_PAGED, *buffer_length, CXPLAT_TAG_LOGICAL_PROCESSOR_INFO);
    if (*buffer == nullptr) {
        return CXPLAT_STATUS_NO_MEMORY;
    }

    if (!GetLogicalProcessorInformationEx(relationship, *buffer, buffer_length)) {
        cxplat_free(*buffer, CXPLAT_POOL_FLAG_NON_PAGED, CXPLAT_TAG_LOGICAL_PROCESSOR_INFO);
        return CXPLAT_STATUS_FROM_WIN32(GetLastError());
    }

    return CXPLAT_STATUS_SUCCESS;
}

cxplat_status_t
cxplat_winuser_initialize_processor_info()
{
    DWORD info_size = 0;
    SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX* info = NULL;
    cxplat_status_t status = _get_processor_group_info(RelationGroup, &info, &info_size);
    if (!CXPLAT_SUCCEEDED(status)) {
        return status;
    }

    _cxplat_processor_group_info = (cxplat_processor_group_info_t*)cxplat_allocate(
        CXPLAT_POOL_FLAG_NON_PAGED,
        info->Group.ActiveGroupCount * sizeof(cxplat_processor_group_info_t),
        CXPLAT_TAG_PROCESSOR_GROUP_INFO);
    if (_cxplat_processor_group_info == nullptr) {
        cxplat_free(info, CXPLAT_POOL_FLAG_NON_PAGED, CXPLAT_TAG_LOGICAL_PROCESSOR_INFO);
        return CXPLAT_STATUS_NO_MEMORY;
    }

    uint32_t current_processor_count = 0;
    for (WORD i = 0; i < info->Group.ActiveGroupCount; ++i) {
        _cxplat_processor_group_info[i].Mask = info->Group.GroupInfo[i].ActiveProcessorMask;
        _cxplat_processor_group_info[i].Offset = current_processor_count;
        current_processor_count += info->Group.GroupInfo[i].MaximumProcessorCount;
    }
    cxplat_free(info, CXPLAT_POOL_FLAG_NON_PAGED, CXPLAT_TAG_LOGICAL_PROCESSOR_INFO);
    return CXPLAT_STATUS_SUCCESS;
}

void
cxplat_winuser_clean_up_processor_info()
{
    cxplat_free(_cxplat_processor_group_info, CXPLAT_POOL_FLAG_NON_PAGED, CXPLAT_TAG_PROCESSOR_GROUP_INFO);
    _cxplat_processor_group_info = nullptr;
}

_Must_inspect_result_ uint32_t
cxplat_get_maximum_processor_count()
{
    return GetMaximumProcessorCount(ALL_PROCESSOR_GROUPS);
}

_Must_inspect_result_ uint32_t
cxplat_get_current_processor_number()
{
    PROCESSOR_NUMBER processor_number;
    GetCurrentProcessorNumberEx(&processor_number);
    return _cxplat_processor_group_info[processor_number.Group].Offset + processor_number.Number;
}

_Must_inspect_result_ uint32_t
cxplat_get_active_processor_count()
{
    return GetActiveProcessorCount(ALL_PROCESSOR_GROUPS);
}

_Requires_lock_not_held_(*lock_handle) _Acquires_lock_(*lock_handle) _Post_same_lock_(*spin_lock, *lock_handle)
    _IRQL_requires_max_(DISPATCH_LEVEL) _IRQL_saves_global_(QueuedSpinLock, lock_handle)
        _IRQL_raises_(DISPATCH_LEVEL) void cxplat_acquire_in_stack_queued_spin_lock(
            _Inout_ cxplat_queue_spin_lock_t* spin_lock, _Out_ cxplat_lock_queue_handle_t* lock_handle)
{
    auto lock = reinterpret_cast<SRWLOCK*>(spin_lock);
    AcquireSRWLockExclusive(lock);

    lock_handle->Reserved_1[0] = reinterpret_cast<uint64_t>(lock);
}

_Requires_lock_held_(*lock_handle) _Releases_lock_(*lock_handle) _IRQL_requires_(DISPATCH_LEVEL)
    _IRQL_restores_global_(QueuedSpinLock, lock_handle) void cxplat_release_in_stack_queued_spin_lock(
        _In_ cxplat_lock_queue_handle_t* lock_handle)
{
    auto lock = reinterpret_cast<SRWLOCK*>(lock_handle->Reserved_1[0]);
    ReleaseSRWLockExclusive(lock);
}

_Requires_lock_not_held_(*lock_handle) _Acquires_lock_(*lock_handle) _Post_same_lock_(*spin_lock, *lock_handle)
    _IRQL_requires_max_(DISPATCH_LEVEL) _IRQL_saves_global_(QueuedSpinLock, lock_handle)
        _IRQL_raises_(DISPATCH_LEVEL) void cxplat_acquire_in_stack_queued_spin_lock_at_dpc(
            _Inout_ cxplat_queue_spin_lock_t* spin_lock, _Out_ cxplat_lock_queue_handle_t* lock_handle)
{
    auto lock = reinterpret_cast<SRWLOCK*>(spin_lock);
    AcquireSRWLockExclusive(lock);

    lock_handle->Reserved_1[0] = reinterpret_cast<uint64_t>(lock);
}

_Requires_lock_held_(*lock_handle) _Releases_lock_(*lock_handle) _IRQL_requires_(DISPATCH_LEVEL)
    _IRQL_restores_global_(QueuedSpinLock, lock_handle) void cxplat_release_in_stack_queued_spin_lock_from_dpc(
        _In_ cxplat_lock_queue_handle_t* lock_handle)
{
    auto lock = reinterpret_cast<SRWLOCK*>(lock_handle->Reserved_1[0]);
    ReleaseSRWLockExclusive(lock);
}

_Requires_lock_not_held_(*spin_lock) _Acquires_lock_(*spin_lock) _IRQL_requires_max_(DISPATCH_LEVEL) _IRQL_saves_
    _IRQL_raises_(DISPATCH_LEVEL)
cxplat_irql_t
cxplat_acquire_spin_lock(_Inout_ cxplat_spin_lock_t* spin_lock)
{
    cxplat_irql_t old_irql = cxplat_raise_irql(DISPATCH_LEVEL);
    auto lock = reinterpret_cast<SRWLOCK*>(spin_lock);
    AcquireSRWLockExclusive(lock);
    return old_irql;
}

_Requires_lock_held_(*spin_lock) _Releases_lock_(*spin_lock)
    _IRQL_requires_(DISPATCH_LEVEL) void cxplat_release_spin_lock(
        _Inout_ cxplat_spin_lock_t* spin_lock, _In_ _IRQL_restores_ cxplat_irql_t old_irql)
{
    auto lock = reinterpret_cast<SRWLOCK*>(spin_lock);
    ReleaseSRWLockExclusive(lock);
    cxplat_lower_irql(old_irql);
}

_Requires_lock_not_held_(*spin_lock) _Acquires_lock_(*spin_lock) _IRQL_requires_min_(
    DISPATCH_LEVEL) void cxplat_acquire_spin_lock_at_dpc_level(_Inout_ cxplat_spin_lock_t* spin_lock)
{
    auto lock = reinterpret_cast<SRWLOCK*>(spin_lock);
    AcquireSRWLockExclusive(lock);
}

_Requires_lock_held_(*spin_lock) _Releases_lock_(*spin_lock) _IRQL_requires_min_(
    DISPATCH_LEVEL) void cxplat_release_spin_lock_from_dpc_level(_Inout_ cxplat_spin_lock_t* spin_lock)
{
    auto lock = reinterpret_cast<SRWLOCK*>(spin_lock);
    ReleaseSRWLockExclusive(lock);
}

thread_local cxplat_irql_t _cxplat_current_irql = PASSIVE_LEVEL;

_IRQL_requires_max_(HIGH_LEVEL) _IRQL_raises_(irql) _IRQL_saves_ cxplat_irql_t
    cxplat_raise_irql(_In_ cxplat_irql_t irql)
{
    auto old_irql = _cxplat_current_irql;
    _cxplat_current_irql = irql;
    return old_irql;
}

_IRQL_requires_max_(HIGH_LEVEL) void cxplat_lower_irql(_In_ _Notliteral_ _IRQL_restores_ cxplat_irql_t irql)
{
    _cxplat_current_irql = irql;
}

_IRQL_requires_max_(HIGH_LEVEL) _IRQL_saves_ cxplat_irql_t cxplat_get_current_irql() { return _cxplat_current_irql; }
