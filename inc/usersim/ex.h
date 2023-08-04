// Copyright (c) Microsoft Corporation
// SPDX-License-Identifier: MIT

#pragma once
#include "..\src\platform.h"
#include "ke.h"

#include <synchapi.h>

#if defined(__cplusplus)
extern "C"
{
#endif

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
    typedef struct _EX_RUNDOWN_REF
    {
        void* reserved;
    } EX_RUNDOWN_REF;

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
        _Inout_ _Requires_lock_held_(*_Curr_) _Releases_lock_(*_Curr_) EX_PUSH_LOCK* push_lock,
        _In_ unsigned long flags);

    USERSIM_API
    _Releases_shared_lock_(push_lock->lock) void ExReleasePushLockSharedEx(
        _Inout_ _Requires_lock_held_(*_Curr_) _Releases_lock_(*_Curr_) EX_PUSH_LOCK* push_lock,
        _In_ unsigned long flags);

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
    _Ret_maybenull_
    void*
    ExAllocatePoolUninitialized(_In_ POOL_TYPE pool_type, _In_ size_t number_of_bytes, _In_ unsigned long tag);

    USERSIM_API
    _Ret_maybenull_
    void*
    ExAllocatePoolWithTag(
        _In_ __drv_strictTypeMatch(__drv_typeExpr) POOL_TYPE pool_type, SIZE_T number_of_bytes, ULONG tag);

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

#if defined(__cplusplus)
}

// The functions below throw C++ exceptions so tests can catch them to verify error behavior.
USERSIM_API void
ExFreePoolCPP(_Frees_ptr_ void* p);

USERSIM_API void
ExFreePoolWithTagCPP(_Frees_ptr_ void* p, ULONG tag);

USERSIM_API _Ret_maybenull_ void*
ExAllocatePoolWithTagCPP(
    _In_ __drv_strictTypeMatch(__drv_typeExpr) POOL_TYPE pool_type, SIZE_T number_of_bytes, ULONG tag);

/**
 * @brief Allocate memory.
 * @param[in] pool_type Pool type to use.
 * @param[in] size Size of memory to allocate.
 * @param[in] tag Pool tag to use.
 * @param[in] initialize False to return "uninitialized" memory.
 * @returns Pointer to memory block allocated, or null on failure.
 */
__drv_allocatesMem(Mem) _Must_inspect_result_ _Ret_writes_maybenull_(size) void* usersim_allocate_with_tag(
    _In_ __drv_strictTypeMatch(__drv_typeExpr) POOL_TYPE pool_type,
    size_t size,
    uint32_t tag,
    bool initialize);

    /**
 * @brief Allocate memory.
 * @param[in] size Size of memory to allocate.
 * @returns Pointer to memory block allocated, or null on failure.
 */
__drv_allocatesMem(Mem) _Must_inspect_result_ _Ret_writes_maybenull_(size) void* usersim_allocate(size_t size);

/**
 * @brief Reallocate memory.
 * @param[in] memory Allocation to be reallocated.
 * @param[in] old_size Old size of memory to reallocate.
 * @param[in] new_size New size of memory to reallocate.
 * @returns Pointer to memory block allocated, or null on failure.
 */
__drv_allocatesMem(Mem) _Must_inspect_result_ _Ret_writes_maybenull_(new_size) void* usersim_reallocate(
    _In_ _Post_invalid_ void* memory, size_t old_size, size_t new_size);

/**
 * @brief Reallocate memory with tag.
 * @param[in] memory Allocation to be reallocated.
 * @param[in] old_size Old size of memory to reallocate.
 * @param[in] new_size New size of memory to reallocate.
 * @param[in] tag Pool tag to use.
 * @returns Pointer to memory block allocated, or null on failure.
 */
__drv_allocatesMem(Mem) _Must_inspect_result_ _Ret_writes_maybenull_(new_size) void* usersim_reallocate_with_tag(
    _In_ _Post_invalid_ void* memory, size_t old_size, size_t new_size, uint32_t tag);

/**
 * @brief Free memory.
 * @param[in] memory Allocation to be freed.
 */
void
usersim_free(_Frees_ptr_opt_ void* memory);

/**
 * @brief Allocate memory that has a starting address that is cache aligned.
 * @param[in] size Size of memory to allocate
 * @returns Pointer to memory block allocated, or null on failure.
 */
USERSIM_API
__drv_allocatesMem(Mem) _Must_inspect_result_
    _Ret_writes_maybenull_(size) void* usersim_allocate_cache_aligned(size_t size);

/**
 * @brief Allocate memory that has a starting address that is cache aligned with tag.
 * @param[in] size Size of memory to allocate
 * @param[in] tag Pool tag to use.
 * @returns Pointer to memory block allocated, or null on failure.
 */
__drv_allocatesMem(Mem) _Must_inspect_result_
    _Ret_writes_maybenull_(size) void* usersim_allocate_cache_aligned_with_tag(size_t size, uint32_t tag);

/**
 * @brief Free memory that has a starting address that is cache aligned.
 * @param[in] memory Allocation to be freed.
 */
void
usersim_free_cache_aligned(_Frees_ptr_opt_ void* memory);

USERSIM_API _Ret_maybenull_ void*
ExAllocatePoolUninitializedCPP(_In_ POOL_TYPE pool_type, _In_ size_t number_of_bytes, _In_ unsigned long tag);

USERSIM_API void
ExRaiseAccessViolationCPP();

USERSIM_API void
ExRaiseDatatypeMisalignmentCPP();

void usersim_initialize_ex(bool leak_detector);
void usersim_clean_up_ex();

#ifdef __cplusplus
#include <memory>
namespace usersim_helper {

struct _usersim_free_functor
{
    void
    operator()(void* memory)
    {
        usersim_free(memory);
    }
};

typedef std::unique_ptr<void, _usersim_free_functor> usersim_memory_ptr;

} // namespace usersim_helper

#endif

#endif
