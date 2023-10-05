// Copyright (c) Microsoft Corporation
// SPDX-License-Identifier: MIT

#pragma once
#include "..\src\platform.h"
#include "cxplat.h"
#include "ke.h"

#include <synchapi.h>

CXPLAT_EXTERN_C_BEGIN

#define EX_DEFAULT_PUSH_LOCK_FLAGS 0
#define ExAcquirePushLockExclusive(Lock) ExAcquirePushLockExclusiveEx(Lock, EX_DEFAULT_PUSH_LOCK_FLAGS)
#define ExAcquirePushLockShared(Lock) ExAcquirePushLockSharedEx(Lock, EX_DEFAULT_PUSH_LOCK_FLAGS)
#define ExReleasePushLockExclusive(Lock) ExReleasePushLockExclusiveEx(Lock, EX_DEFAULT_PUSH_LOCK_FLAGS)
#define ExReleasePushLockShared(Lock) ExReleasePushLockSharedEx(Lock, EX_DEFAULT_PUSH_LOCK_FLAGS)
#define ExAcquireSpinLockExclusive(Lock) ExAcquireSpinLockExclusiveEx(Lock)
#define ExAcquireSpinLockShared(Lock) ExAcquireSpinLockSharedEx(Lock)
#define ExAcquireSpinLockExclusiveAtDpcLevel(Lock) ExAcquireSpinLockExclusiveAtDpcLevelEx(Lock)
#define ExReleaseSpinLockExclusive(Lock, Irql) ExReleaseSpinLockExclusiveEx(Lock, Irql)
#define ExReleaseSpinLockShared(Lock, Irql) ExReleaseSpinLockSharedEx(Lock, Irql)
#define ExReleaseSpinLockExclusiveFromDpcLevel(Lock) ExReleaseSpinLockExclusiveFromDpcLevelEx(Lock)

// Bug check codes.
typedef enum
{
    BAD_POOL_CALLER = 0xC2,
} bug_check_code_t;

typedef struct _EX_PUSH_LOCK
{
    SRWLOCK lock;
} EX_PUSH_LOCK;
typedef struct _EX_SPIN_LOCK
{
    SRWLOCK lock;
} EX_SPIN_LOCK;
typedef cxplat_rundown_reference_t EX_RUNDOWN_REF;

//
// Pool Allocation routines (in pool.c)
//
typedef _Enum_is_bitflag_ enum _POOL_TYPE {
    NonPagedPool,
    NonPagedPoolExecute = NonPagedPool,
    PagedPool,
    NonPagedPoolMustSucceed = NonPagedPool + 2,
    DontUseThisType,
    NonPagedPoolCacheAligned = NonPagedPool + 4,
    PagedPoolCacheAligned,
    NonPagedPoolCacheAlignedMustS = NonPagedPool + 6,
    MaxPoolType,

    //
    // Define base types for NonPaged (versus Paged) pool, for use in cracking
    // the underlying pool type.
    //

    NonPagedPoolBase = 0,
    NonPagedPoolBaseMustSucceed = NonPagedPoolBase + 2,
    NonPagedPoolBaseCacheAligned = NonPagedPoolBase + 4,
    NonPagedPoolBaseCacheAlignedMustS = NonPagedPoolBase + 6,

    //
    // Note these per session types are carefully chosen so that the appropriate
    // masking still applies as well as MaxPoolType above.
    //

    NonPagedPoolSession = 32,
    PagedPoolSession = NonPagedPoolSession + 1,
    NonPagedPoolMustSucceedSession = PagedPoolSession + 1,
    DontUseThisTypeSession = NonPagedPoolMustSucceedSession + 1,
    NonPagedPoolCacheAlignedSession = DontUseThisTypeSession + 1,
    PagedPoolCacheAlignedSession = NonPagedPoolCacheAlignedSession + 1,
    NonPagedPoolCacheAlignedMustSSession = PagedPoolCacheAlignedSession + 1,

    NonPagedPoolNx = 512,
    NonPagedPoolNxCacheAligned = NonPagedPoolNx + 4,
    NonPagedPoolSessionNx = NonPagedPoolNx + 32,

} _Enum_is_bitflag_ POOL_TYPE;

typedef ULONG64 POOL_FLAGS;
#define POOL_FLAG_NON_PAGED 0x00000040

typedef cxplat_lookaside_list_t LOOKASIDE_LIST_EX;

typedef _IRQL_requires_same_ _Function_class_(ALLOCATE_FUNCTION_EX) PVOID ALLOCATE_FUNCTION_EX(
    _In_ POOL_TYPE pool_type, _In_ SIZE_T number_og_bytes, _In_ ULONG tag, _Inout_ LOOKASIDE_LIST_EX* lookaside);

typedef _IRQL_requires_same_ _Function_class_(FREE_FUNCTION_EX) VOID
    FREE_FUNCTION_EX(_In_ __drv_freesMem(Mem) void* buffer, _Inout_ LOOKASIDE_LIST_EX* lookaside);

USERSIM_API
void
ExInitializeRundownProtection(_Out_ EX_RUNDOWN_REF* rundown_ref);

USERSIM_API
void
ExReInitializeRundownProtection(_Inout_ EX_RUNDOWN_REF* rundown_ref);

USERSIM_API
void
ExWaitForRundownProtectionRelease(_Inout_ EX_RUNDOWN_REF* rundown_ref);

USERSIM_API
BOOLEAN
ExAcquireRundownProtection(_Inout_ EX_RUNDOWN_REF* rundown_ref);

USERSIM_API
void
ExReleaseRundownProtection(_Inout_ EX_RUNDOWN_REF* rundown_ref);

USERSIM_API
_Acquires_exclusive_lock_(push_lock->lock) void ExAcquirePushLockExclusiveEx(
    _Inout_ _Requires_lock_not_held_(*_Curr_) _Acquires_lock_(*_Curr_) EX_PUSH_LOCK* push_lock,
    _In_ unsigned long flags);

USERSIM_API
_Acquires_shared_lock_(push_lock->lock) void ExAcquirePushLockSharedEx(
    _Inout_ _Requires_lock_not_held_(*_Curr_) _Acquires_lock_(*_Curr_) EX_PUSH_LOCK* push_lock,
    _In_ unsigned long flags);

USERSIM_API
_Releases_exclusive_lock_(push_lock->lock) void ExReleasePushLockExclusiveEx(
    _Inout_ _Requires_lock_held_(*_Curr_) _Releases_lock_(*_Curr_) EX_PUSH_LOCK* push_lock, _In_ unsigned long flags);

USERSIM_API
_Releases_shared_lock_(push_lock->lock) void ExReleasePushLockSharedEx(
    _Inout_ _Requires_lock_held_(*_Curr_) _Releases_lock_(*_Curr_) EX_PUSH_LOCK* push_lock, _In_ unsigned long flags);

USERSIM_API
_Acquires_exclusive_lock_(spin_lock->lock) KIRQL
    ExAcquireSpinLockExclusiveEx(_Inout_ _Requires_lock_not_held_(*_Curr_) _Acquires_lock_(*_Curr_)
                                     EX_SPIN_LOCK* spin_lock);

USERSIM_API
_Acquires_exclusive_lock_(spin_lock->lock) void ExAcquireSpinLockExclusiveAtDpcLevelEx(
    _Inout_ _Requires_lock_not_held_(*_Curr_) _Acquires_lock_(*_Curr_) EX_SPIN_LOCK* spin_lock);

USERSIM_API
_Acquires_shared_lock_(spin_lock->lock) KIRQL
    ExAcquireSpinLockSharedEx(_Inout_ _Requires_lock_not_held_(*_Curr_) _Acquires_lock_(*_Curr_)
                                  EX_SPIN_LOCK* spin_lock);

USERSIM_API
_Releases_exclusive_lock_(spin_lock->lock) void ExReleaseSpinLockExclusiveEx(
    _Inout_ _Requires_lock_held_(*_Curr_) _Releases_lock_(*_Curr_) EX_SPIN_LOCK* spin_lock, KIRQL old_irql);

USERSIM_API
_Releases_shared_lock_(spin_lock->lock) void ExReleaseSpinLockSharedEx(
    _Inout_ _Requires_lock_held_(*_Curr_) _Releases_lock_(*_Curr_) EX_SPIN_LOCK* spin_lock, KIRQL old_irql);

USERSIM_API
_Releases_exclusive_lock_(spin_lock->lock) void ExReleaseSpinLockExclusiveFromDpcLevelEx(
    _Inout_ _Requires_lock_held_(*_Curr_) _Releases_lock_(*_Curr_) EX_SPIN_LOCK* spin_lock);

USERSIM_API
_Ret_maybenull_ void*
ExAllocatePoolUninitialized(
    _In_ __drv_strictTypeMatch(__drv_typeExpr) POOL_TYPE pool_type, size_t number_of_bytes, unsigned long tag);

USERSIM_API
_Ret_maybenull_ void*
ExAllocatePoolWithTag(
    _In_ __drv_strictTypeMatch(__drv_typeExpr) POOL_TYPE pool_type, SIZE_T number_of_bytes, ULONG tag);

USERSIM_API
_Ret_maybenull_ void*
ExAllocatePool2(
    _In_ POOL_FLAGS pool_flags, SIZE_T number_of_bytes, ULONG tag);

USERSIM_API
void
ExFreePool(_Frees_ptr_ void* p);

USERSIM_API
void
ExFreePoolWithTag(_Frees_ptr_ void* p, ULONG tag);

USERSIM_API
void
ExInitializePushLock(_Out_ EX_PUSH_LOCK* push_lock);

USERSIM_API
_IRQL_requires_max_(PASSIVE_LEVEL) NTKERNELAPI NTSTATUS ExUuidCreate(_Out_ UUID* uuid);

USERSIM_API void
ExRaiseAccessViolation();

USERSIM_API void
ExRaiseDatatypeMisalignment();

_IRQL_requires_max_(DISPATCH_LEVEL) USERSIM_API NTSTATUS ExInitializeLookasideListEx(
    _Out_ LOOKASIDE_LIST_EX* lookaside_list,
    _In_opt_ ALLOCATE_FUNCTION_EX* allocate,
    _In_opt_ FREE_FUNCTION_EX* free,
    POOL_TYPE pool_type,
    unsigned long flags,
    size_t size,
    unsigned long tag,
    unsigned short depth);

_IRQL_requires_max_(DISPATCH_LEVEL) USERSIM_API void ExDeleteLookasideListEx(_Inout_ LOOKASIDE_LIST_EX* lookaside_list);

_IRQL_requires_max_(DISPATCH_LEVEL) USERSIM_API _Ret_maybenull_
    void* ExAllocateFromLookasideListEx(_Inout_ LOOKASIDE_LIST_EX* lookaside_list);

_IRQL_requires_max_(DISPATCH_LEVEL) USERSIM_API
    void ExFreeToLookasideListEx(_Inout_ LOOKASIDE_LIST_EX* lookaside_list, _In_ _Post_invalid_ void* entry);

CXPLAT_EXTERN_C_END

#if defined(__cplusplus)

// The functions below throw C++ exceptions so tests can catch them to verify error behavior.
USERSIM_API void
ExFreePoolCPP(_Frees_ptr_ void* p);

USERSIM_API void
ExFreePoolWithTagCPP(_Frees_ptr_ void* p, ULONG tag);

USERSIM_API _Ret_maybenull_ void*
ExAllocatePool2CPP(__drv_strictTypeMatch(__drv_typeExpr) POOL_FLAGS pool_flags, size_t number_of_bytes, ULONG tag);

USERSIM_API _Ret_maybenull_ void*
ExAllocatePoolWithTagCPP(__drv_strictTypeMatch(__drv_typeExpr) POOL_TYPE pool_type, size_t number_of_bytes, ULONG tag);

USERSIM_API _Ret_maybenull_ void*
ExAllocatePoolUninitializedCPP(POOL_TYPE pool_type, size_t number_of_bytes, unsigned long tag);

USERSIM_API void
ExRaiseAccessViolationCPP();

USERSIM_API void
ExRaiseDatatypeMisalignmentCPP();

void
usersim_initialize_ex(bool leak_detector);
void
usersim_clean_up_ex();

#endif
