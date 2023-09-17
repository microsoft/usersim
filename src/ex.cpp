// Copyright (c) Microsoft Corporation
// SPDX-License-Identifier: MIT

#include "cxplat_fault_injection.h"
#include "kernel_um.h"
#include "platform.h"
#include "usersim/ex.h"
#include "usersim/ke.h"

#include <condition_variable>
#include <map>
#include <mutex>
#include <sstream>
#include <tuple>

// Ex* functions.

void
ExInitializeRundownProtection(_Out_ EX_RUNDOWN_REF* rundown_reference)
{
    cxplat_initialize_rundown_protection(rundown_reference);
}

void
ExReInitializeRundownProtection(_Inout_ EX_RUNDOWN_REF* rundown_reference)
{
    cxplat_reinitialize_rundown_protection(rundown_reference);
}

void
ExWaitForRundownProtectionRelease(_Inout_ EX_RUNDOWN_REF* rundown_reference)
{
    cxplat_wait_for_rundown_protection_release(rundown_reference);
}

BOOLEAN
ExAcquireRundownProtection(_Inout_ EX_RUNDOWN_REF* rundown_reference)
{
    return (BOOLEAN)cxplat_acquire_rundown_protection(rundown_reference);
}

void
ExReleaseRundownProtection(_Inout_ EX_RUNDOWN_REF* rundown_reference)
{
    cxplat_release_rundown_protection(rundown_reference);
}

_Acquires_exclusive_lock_(push_lock->lock) void ExAcquirePushLockExclusiveEx(
    _Inout_ _Requires_lock_not_held_(*_Curr_) _Acquires_lock_(*_Curr_) EX_PUSH_LOCK* push_lock,
    _In_ unsigned long flags)
{
    UNREFERENCED_PARAMETER(flags);
    AcquireSRWLockExclusive(&push_lock->lock);
}

_Acquires_shared_lock_(push_lock->lock) void ExAcquirePushLockSharedEx(
    _Inout_ _Requires_lock_not_held_(*_Curr_) _Acquires_lock_(*_Curr_) EX_PUSH_LOCK* push_lock,
    _In_ unsigned long flags)
{
    UNREFERENCED_PARAMETER(flags);
    AcquireSRWLockShared(&push_lock->lock);
}

_Releases_exclusive_lock_(push_lock->lock) void ExReleasePushLockExclusiveEx(
    _Inout_ _Requires_lock_held_(*_Curr_) _Releases_lock_(*_Curr_) EX_PUSH_LOCK* push_lock, _In_ unsigned long flags)
{
    UNREFERENCED_PARAMETER(flags);
    ReleaseSRWLockExclusive(&push_lock->lock);
}

_Releases_shared_lock_(push_lock->lock) void ExReleasePushLockSharedEx(
    _Inout_ _Requires_lock_held_(*_Curr_) _Releases_lock_(*_Curr_) EX_PUSH_LOCK* push_lock, _In_ unsigned long flags)
{
    UNREFERENCED_PARAMETER(flags);
    ReleaseSRWLockShared(&push_lock->lock);
}

_Acquires_exclusive_lock_(spin_lock->lock) KIRQL
    ExAcquireSpinLockExclusiveEx(_Inout_ _Requires_lock_not_held_(*_Curr_) _Acquires_lock_(*_Curr_)
                                     EX_SPIN_LOCK* spin_lock)
{
    AcquireSRWLockExclusive(&spin_lock->lock);
    return PASSIVE_LEVEL;
}

_Acquires_exclusive_lock_(spin_lock->lock) void ExAcquireSpinLockExclusiveAtDpcLevelEx(
    _Inout_ _Requires_lock_not_held_(*_Curr_) _Acquires_lock_(*_Curr_) EX_SPIN_LOCK* spin_lock)
{
    AcquireSRWLockExclusive(&spin_lock->lock);
}

_Acquires_shared_lock_(spin_lock->lock) KIRQL
    ExAcquireSpinLockSharedEx(_Inout_ _Requires_lock_not_held_(*_Curr_) _Acquires_lock_(*_Curr_)
                                  EX_SPIN_LOCK* spin_lock)
{
    AcquireSRWLockShared(&spin_lock->lock);
    return PASSIVE_LEVEL;
}

_Releases_exclusive_lock_(spin_lock->lock) void ExReleaseSpinLockExclusiveEx(
    _Inout_ _Requires_lock_held_(*_Curr_) _Releases_lock_(*_Curr_) EX_SPIN_LOCK* spin_lock, KIRQL old_irql)
{
    UNREFERENCED_PARAMETER(old_irql);
    ReleaseSRWLockExclusive(&spin_lock->lock);
}

_Releases_exclusive_lock_(spin_lock->lock) void ExReleaseSpinLockExclusiveFromDpcLevelEx(
    _Inout_ _Requires_lock_held_(*_Curr_) _Releases_lock_(*_Curr_) EX_SPIN_LOCK* spin_lock)
{
    ReleaseSRWLockExclusive(&spin_lock->lock);
}

_Releases_shared_lock_(spin_lock->lock) void ExReleaseSpinLockSharedEx(
    _Inout_ _Requires_lock_held_(*_Curr_) _Releases_lock_(*_Curr_) EX_SPIN_LOCK* spin_lock, KIRQL old_irql)
{
    UNREFERENCED_PARAMETER(old_irql);
    ReleaseSRWLockShared(&spin_lock->lock);
}

#define USERSIM_CACHE_LINE_SIZE 64
typedef struct
{
    union
    {
        cxplat_pool_flags_t pool_flags;
        char align[USERSIM_CACHE_LINE_SIZE];
    };
} usersim_allocation_header_t;

static_assert(sizeof(usersim_allocation_header_t) == USERSIM_CACHE_LINE_SIZE);

cxplat_pool_flags_t
_pool_type_to_flags(POOL_TYPE pool_type, bool initialize)
{
    cxplat_pool_flags_t pool_flags = (initialize) ? CXPLAT_POOL_FLAG_NONE : CXPLAT_POOL_FLAG_UNINITIALIZED;
    switch (pool_type) {
    case NonPagedPoolNx:
        return (cxplat_pool_flags_t)(pool_flags | CXPLAT_POOL_FLAG_NON_PAGED);
    case NonPagedPoolNxCacheAligned:
        return (cxplat_pool_flags_t)(pool_flags | CXPLAT_POOL_FLAG_NON_PAGED | CXPLAT_POOL_FLAG_CACHE_ALIGNED);
    case PagedPool:
        return (cxplat_pool_flags_t)(pool_flags | CXPLAT_POOL_FLAG_PAGED);
    case PagedPoolCacheAligned:
        return (cxplat_pool_flags_t)(pool_flags | CXPLAT_POOL_FLAG_PAGED | CXPLAT_POOL_FLAG_CACHE_ALIGNED);
    default:
        // Others not yet implemented.
        KeBugCheckCPP(BAD_POOL_CALLER);
        return CXPLAT_POOL_FLAG_NONE;
    }
}

_Ret_maybenull_ void*
ExAllocatePoolUninitializedCPP(_In_ POOL_TYPE pool_type, _In_ size_t number_of_bytes, _In_ unsigned long tag)
{
    if (tag == 0) {
        KeBugCheckExCPP(BAD_POOL_CALLER, 0x9B, pool_type, number_of_bytes, 0);
    }

    cxplat_pool_flags_t pool_flags = _pool_type_to_flags(pool_type, false);
    usersim_allocation_header_t* header = (usersim_allocation_header_t*)cxplat_allocate(pool_flags, sizeof(*header) + number_of_bytes, tag);
    if (header) {
        header->pool_flags = pool_flags;
    }
    return (header + 1);
}

_Ret_maybenull_ void*
ExAllocatePoolUninitialized(
    _In_ __drv_strictTypeMatch(__drv_typeExpr) POOL_TYPE pool_type, SIZE_T number_of_bytes, ULONG tag)
{
    return ExAllocatePoolUninitializedCPP(pool_type, number_of_bytes, tag);
}

_Ret_maybenull_ void*
ExAllocatePoolWithTagCPP(
    _In_ __drv_strictTypeMatch(__drv_typeExpr) POOL_TYPE pool_type, SIZE_T number_of_bytes, ULONG tag)
{
    if (tag == 0 || number_of_bytes == 0) {
        KeBugCheckExCPP(BAD_POOL_CALLER, 0x9B, pool_type, number_of_bytes, 0);
    }
    cxplat_pool_flags_t pool_flags = _pool_type_to_flags(pool_type, true);
    usersim_allocation_header_t* header =
        (usersim_allocation_header_t*)cxplat_allocate(pool_flags, sizeof(*header) + number_of_bytes, tag);
    if (header) {
        header->pool_flags = pool_flags;
    }
    return (header + 1);
}

_Ret_maybenull_ void*
ExAllocatePoolWithTag(_In_ __drv_strictTypeMatch(__drv_typeExpr) POOL_TYPE pool_type, SIZE_T number_of_bytes, ULONG tag)
{
    return ExAllocatePoolWithTagCPP(pool_type, number_of_bytes, tag);
}

void
ExFreePoolCPP(_Frees_ptr_ void* p)
{
    if (p == nullptr) {
        KeBugCheckExCPP(BAD_POOL_CALLER, 0x46, 0, 0, 0);
    }
    usersim_allocation_header_t* header = ((usersim_allocation_header_t*)p) - 1;
    cxplat_free(header, header->pool_flags, 0);
}

void
ExFreePool(_Frees_ptr_ void* p)
{
    ExFreePoolCPP(p);
}

void
ExFreePoolWithTagCPP(_Frees_ptr_ void* p, ULONG tag)
{
    if (p == nullptr) {
        KeBugCheckExCPP(BAD_POOL_CALLER, 0x46, 0, 0, 0);
    }
    usersim_allocation_header_t* header = ((usersim_allocation_header_t*)p) - 1;
    cxplat_free(header, header->pool_flags, tag);
}

void
ExFreePoolWithTag(_Frees_ptr_ void* p, ULONG tag)
{
    ExFreePoolWithTagCPP(p, tag);
}

void
ExInitializePushLock(_Out_ EX_PUSH_LOCK* push_lock)
{
    push_lock->lock = SRWLOCK_INIT;
}

_IRQL_requires_max_(PASSIVE_LEVEL) NTKERNELAPI NTSTATUS ExUuidCreate(_Out_ UUID* uuid)
{
    if (cxplat_fault_injection_inject_fault()) {
        return STATUS_NOT_SUPPORTED;
    }

    if (UuidCreate(uuid) == RPC_S_OK) {
        return STATUS_SUCCESS;
    } else {
        return STATUS_NOT_SUPPORTED;
    }
}

void
ExRaiseAccessViolationCPP()
{
    // This should really use RaiseException(STATUS_ACCESS_VIOLATION, 0, 0, 0)
    // but SEH doesn't seem to play well with Catch2 so for now we use C++
    // exceptions.
    std::ostringstream ss;
    ss << "Exception: " << STATUS_ACCESS_VIOLATION << "\n";
    usersim_throw_exception(ss.str());
}

void
ExRaiseAccessViolation()
{
    ExRaiseAccessViolationCPP();
}

void
ExRaiseDatatypeMisalignmentCPP()
{
    // This should really use RaiseException(STATUS_DATATYPE_MISALIGNMENT, 0, 0, 0)
    // but SEH doesn't seem to play well with Catch2 so for now we use C++
    // exceptions.
    std::ostringstream ss;
    ss << "Exception: " << STATUS_DATATYPE_MISALIGNMENT << "\n";
    usersim_throw_exception(ss.str());
}

void
ExRaiseDatatypeMisalignment()
{
    ExRaiseDatatypeMisalignmentCPP();
}
