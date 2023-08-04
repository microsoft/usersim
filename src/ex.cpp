// Copyright (c) Microsoft Corporation
// SPDX-License-Identifier: MIT

#include "fault_injection.h"
#include "leak_detector.h"
#include "platform.h"
#include "kernel_um.h"
#include "usersim/ex.h"
#include "usersim/ke.h"

#include <condition_variable>
#include <map>
#include <mutex>
#include <sstream>
#include <tuple>

// Ex* functions.

extern "C" size_t usersim_fuzzing_memory_limit = MAXSIZE_T;
usersim_leak_detector_ptr _usersim_leak_detector_ptr;

/***
 * @brief This following class implements a mock of the Windows Kernel's rundown reference implementation.
 * 1) It uses a map to track the number of references to a given EX_RUNDOWN_REF structure.
 * 2) The address of the EX_RUNDOWN_REF structure is used as the key to the map.
 * 3) A single condition variable is used to wait for the ref count of any EX_RUNDOWN_REF structure to reach 0.
 * 4) The class is a singleton and is created during static initialization and destroyed during static destruction.
 */
typedef class _rundown_ref_table
{
  private:
    static std::unique_ptr<_rundown_ref_table> _instance;

  public:
    // The constructor and destructor should be private to ensure that the class is a singleton, but that is not
    // possible because the singleton instance is stored in a unique_ptr which requires the constructor to be public.
    // The instance of the class can be accessed using the instance() method.
    _rundown_ref_table() = default;
    ~_rundown_ref_table() = default;

    /**
     * @brief Get the singleton instance of the rundown ref table.
     *
     * @return The singleton instance of the rundown ref table.
     */
    static _rundown_ref_table&
    instance()
    {
        return *_instance;
    }

    /**
     * @brief Initialize the rundown ref table entry for the given context.
     *
     * @param[in] context The address of a EX_RUNDOWN_REF structure.
     */
    void
    initialize_rundown_ref(_In_ const void* context)
    {
        std::unique_lock lock(_lock);

        // Re-initialize the entry if it already exists.
        if (_rundown_ref_counts.find((uint64_t)context) != _rundown_ref_counts.end()) {
            _rundown_ref_counts.erase((uint64_t)context);
        }

        _rundown_ref_counts[(uint64_t)context] = {false, 0};
    }

    /**
     * @brief Reinitialize the rundown ref table entry for the given context.
     *
     * @param[in] context The address of a previously run down EX_RUNDOWN_REF structure.
     */
    void
    reinitialize_rundown_ref(_In_ const void* context)
    {
        std::unique_lock lock(_lock);

        // Fail if the entry is not initialized.
        if (_rundown_ref_counts.find((uint64_t)context) == _rundown_ref_counts.end()) {
            throw std::runtime_error("rundown ref table not initialized");
        }

        auto& [rundown, ref_count] = _rundown_ref_counts[(uint64_t)context];

        // Check if the entry is not rundown.
        if (!rundown) {
            throw std::runtime_error("rundown ref table not rundown");
        }

        if (ref_count != 0) {
            throw std::runtime_error("rundown ref table corruption");
        }

        rundown = false;
    }

    /**
     * @brief Acquire a rundown ref for the given context.
     *
     * @param[in] context The address of a EX_RUNDOWN_REF structure.
     * @retval true Rundown has not started.
     * @retval false Rundown has started.
     */
    bool
    acquire_rundown_ref(_In_ const void* context)
    {
        std::unique_lock lock(_lock);

        // Fail if the entry is not initialized.
        if (_rundown_ref_counts.find((uint64_t)context) == _rundown_ref_counts.end()) {
            throw std::runtime_error("rundown ref table not initialized");
        }

        // Check if the entry is already rundown.
        if (std::get<0>(_rundown_ref_counts[(uint64_t)context])) {
            return false;
        }

        // Increment the ref count if the entry is not rundown.
        std::get<1>(_rundown_ref_counts[(uint64_t)context])++;

        return true;
    }

    /**
     * @brief Release a rundown ref for the given context.
     *
     * @param[in] context The address of a EX_RUNDOWN_REF structure.
     */
    void
    release_rundown_ref(_In_ const void* context)
    {
        std::unique_lock lock(_lock);

        // Fail if the entry is not initialized.
        if (_rundown_ref_counts.find((uint64_t)context) == _rundown_ref_counts.end()) {
            throw std::runtime_error("rundown ref table not initialized");
        }

        if (std::get<1>(_rundown_ref_counts[(uint64_t)context]) == 0) {
            throw std::runtime_error("rundown ref table already released");
        }

        std::get<1>(_rundown_ref_counts[(uint64_t)context])--;

        if (std::get<1>(_rundown_ref_counts[(uint64_t)context]) == 0) {
            _rundown_ref_cv.notify_all();
        }
    }

    /**
     * @brief Wait for the rundown ref count to reach 0 for the given context.
     *
     * @param[in] context The address of a EX_RUNDOWN_REF structure.
     */
    void
    wait_for_rundown_ref(_In_ const void* context)
    {
        std::unique_lock lock(_lock);

        // Fail if the entry is not initialized.
        if (_rundown_ref_counts.find((uint64_t)context) == _rundown_ref_counts.end()) {
            throw std::runtime_error("rundown ref table not initialized");
        }

        auto& [rundown, ref_count] = _rundown_ref_counts[(uint64_t)context];
        rundown = true;
        // Wait for the ref count to reach 0.
        _rundown_ref_cv.wait(lock, [&ref_count] { return ref_count == 0; });
    }

  private:
    std::mutex _lock;
    std::map<uint64_t, std::tuple<bool, uint64_t>> _rundown_ref_counts;
    std::condition_variable _rundown_ref_cv;
} rundown_ref_table_t;

/**
 * @brief The singleton instance of the rundown ref table. Created during static initialization and destroyed during
 * static destruction.
 */
std::unique_ptr<_rundown_ref_table> rundown_ref_table_t::_instance = std::make_unique<rundown_ref_table_t>();


void
ExInitializeRundownProtection(_Out_ EX_RUNDOWN_REF* rundown_ref)
{
#pragma warning(push)
#pragma warning(suppress : 6001) // Uninitialized memory. The rundown_ref is used as a key in a map and is not
                                 // dereferenced.
    rundown_ref_table_t::instance().initialize_rundown_ref(rundown_ref);
#pragma warning(pop)
}

void
ExReInitializeRundownProtection(_Inout_ EX_RUNDOWN_REF* rundown_ref)
{
    rundown_ref_table_t::instance().reinitialize_rundown_ref(rundown_ref);
}

void
ExWaitForRundownProtectionRelease(_Inout_ EX_RUNDOWN_REF* rundown_ref)
{
    rundown_ref_table_t::instance().wait_for_rundown_ref(rundown_ref);
}

BOOLEAN
ExAcquireRundownProtection(_Inout_ EX_RUNDOWN_REF* rundown_ref)
{
    if (rundown_ref_table_t::instance().acquire_rundown_ref(rundown_ref)) {
        return TRUE;
    } else {
        return FALSE;
    }
}

void
ExReleaseRundownProtection(_Inout_ EX_RUNDOWN_REF* rundown_ref)
{
    rundown_ref_table_t::instance().release_rundown_ref(rundown_ref);
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

 _Ret_maybenull_ void*
ExAllocatePoolUninitializedCPP(
    _In_ POOL_TYPE pool_type,
    _In_ size_t number_of_bytes,
    _In_ unsigned long tag)
{
    if (tag == 0) {
        KeBugCheckExCPP(BAD_POOL_CALLER, 0x9B, pool_type, number_of_bytes, 0);
    }
    return usersim_allocate_with_tag(pool_type, number_of_bytes, tag, false);
}

_Ret_maybenull_ void*
ExAllocatePoolUninitialized(
    _In_ __drv_strictTypeMatch(__drv_typeExpr) POOL_TYPE pool_type, SIZE_T number_of_bytes, ULONG tag)
{
    return ExAllocatePoolUninitializedCPP(pool_type, number_of_bytes, tag);
}

_Ret_maybenull_ void*
ExAllocatePoolWithTagCPP(
    _In_ __drv_strictTypeMatch(__drv_typeExpr) POOL_TYPE pool_type,
    SIZE_T number_of_bytes,
    ULONG tag)
{
    if (tag == 0) {
        KeBugCheckExCPP(BAD_POOL_CALLER, 0x9B, pool_type, number_of_bytes, 0);
    }
    return usersim_allocate_with_tag(pool_type, number_of_bytes, tag, true);
}

#define USERSIM_CACHE_LINE_SIZE 64

typedef struct
{
    POOL_TYPE pool_type;
    uint32_t tag;
} usersim_allocation_header_t;

__drv_allocatesMem(Mem) _Must_inspect_result_ _Ret_writes_maybenull_(size) void*
usersim_allocate_with_tag(
    _In_ __drv_strictTypeMatch(__drv_typeExpr) POOL_TYPE pool_type, size_t size, uint32_t tag, bool initialize)
{
    if (size == 0) {
        KeBugCheckEx(BAD_POOL_CALLER, 0x00, 0, 0, 0);
    }
    if (size > usersim_fuzzing_memory_limit) {
        return nullptr;
    }

    if (usersim_fault_injection_inject_fault()) {
        return nullptr;
    }

    // Allocate space with a usersim_allocation_header_t prepended.
    void* memory;
    if (pool_type == NonPagedPoolNxCacheAligned) {
        // The pointer we return has to be cache aligned so we allocate
        // enough extra space to fill a cache line, and put the
        // usersim_allocation_header_t at the end of that space.
        size_t full_size = USERSIM_CACHE_LINE_SIZE + size;
        uint8_t* pointer = (uint8_t*)_aligned_malloc(full_size, USERSIM_CACHE_LINE_SIZE);
        if (pointer == nullptr) {
            return nullptr;
        }
        memory = pointer + USERSIM_CACHE_LINE_SIZE;
    } else {
        size_t full_size = sizeof(usersim_allocation_header_t) + size;
        uint8_t* pointer = (uint8_t*)calloc(full_size, 1);
        if (pointer == nullptr) {
            return nullptr;
        }
        memory = pointer + sizeof(usersim_allocation_header_t);
    }

    // Do any initialization.
    auto header = (usersim_allocation_header_t*)((uint8_t*)memory - sizeof(usersim_allocation_header_t));
    header->pool_type = pool_type;
    header->tag = tag;
    if (!initialize) {
        // The calloc call always zero-initializes memory.  To test
        // returning uninitialized memory, we explicitly fill it with 0xcc.
        memset(memory, 0xcc, size);
    }

    if (memory && _usersim_leak_detector_ptr) {
        _usersim_leak_detector_ptr->register_allocation(reinterpret_cast<uintptr_t>(memory), size);
    }

    return memory;
}

__drv_allocatesMem(Mem) _Must_inspect_result_ _Ret_writes_maybenull_(new_size) void*
usersim_reallocate(
    _In_ _Post_invalid_ void* memory, size_t old_size, size_t new_size)
{
    return usersim_reallocate_with_tag(memory, old_size, new_size, 0);
}

__drv_allocatesMem(Mem) _Must_inspect_result_ _Ret_writes_maybenull_(new_size) void*
usersim_reallocate_with_tag(
    _In_ _Post_invalid_ void* memory, size_t old_size, size_t new_size, uint32_t tag)
{
    UNREFERENCED_PARAMETER(tag);
    UNREFERENCED_PARAMETER(old_size);

    if (new_size > usersim_fuzzing_memory_limit) {
        return nullptr;
    }

    if (usersim_fault_injection_inject_fault()) {
        return nullptr;
    }

    void* p;
    auto header = (usersim_allocation_header_t*)((uint8_t*)memory - sizeof(usersim_allocation_header_t));
    if (header->pool_type == NonPagedPoolNxCacheAligned) {
        uint8_t* pointer = ((uint8_t*)memory) - USERSIM_CACHE_LINE_SIZE;
        p = _aligned_realloc(pointer, USERSIM_CACHE_LINE_SIZE + new_size, USERSIM_CACHE_LINE_SIZE);
    } else {
        uint8_t* pointer = ((uint8_t*)memory) - sizeof(usersim_allocation_header_t);
        p = realloc(pointer, sizeof(usersim_allocation_header_t)  + new_size);
    }

    if (p && (new_size > old_size)) {
        memset(((char*)p) + old_size, 0, new_size - old_size);
    }

    if (_usersim_leak_detector_ptr) {
        _usersim_leak_detector_ptr->unregister_allocation(reinterpret_cast<uintptr_t>(memory));
        _usersim_leak_detector_ptr->register_allocation(reinterpret_cast<uintptr_t>(p), new_size);
    }

    return p;
}

__drv_allocatesMem(Mem) _Must_inspect_result_ _Ret_writes_maybenull_(size) void* usersim_allocate(size_t size)
{
    return usersim_allocate_with_tag(NonPagedPool, size, 'tset', true);
}

void
usersim_free(_Frees_ptr_opt_ void* memory)
{
    if (memory == nullptr) {
        return;
    }
    auto header = (usersim_allocation_header_t*)((uint8_t*)memory - sizeof(usersim_allocation_header_t));
    if (_usersim_leak_detector_ptr) {
        _usersim_leak_detector_ptr->unregister_allocation(reinterpret_cast<uintptr_t>(memory));
    }
    if (header->pool_type == NonPagedPoolNxCacheAligned) {
        uint8_t* pointer = ((uint8_t*)memory) - USERSIM_CACHE_LINE_SIZE;
        _aligned_free(pointer);
    } else {
        uint8_t* pointer = ((uint8_t*)memory) - sizeof(usersim_allocation_header_t);
        free(pointer);
    }
}

__drv_allocatesMem(Mem) _Must_inspect_result_
_Ret_writes_maybenull_(size) void*
usersim_allocate_cache_aligned(size_t size)
{
    if (size > usersim_fuzzing_memory_limit) {
        return nullptr;
    }

    if (usersim_fault_injection_inject_fault()) {
        return nullptr;
    }

    void* memory = _aligned_malloc(size, USERSIM_CACHE_LINE_SIZE);
    if (memory) {
        memset(memory, 0, size);
    }
    return memory;
}

__drv_allocatesMem(Mem) _Must_inspect_result_
_Ret_writes_maybenull_(size) void*
usersim_allocate_cache_aligned_with_tag(size_t size, uint32_t tag)
{
    UNREFERENCED_PARAMETER(tag);

    return usersim_allocate_cache_aligned(size);
}

void
usersim_free_cache_aligned(_Frees_ptr_opt_ void* memory)
{
    _aligned_free(memory);
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
    usersim_free(p);
}

void
ExFreePool(_Frees_ptr_ void* p)
{
    ExFreePoolCPP(p);
}

void
ExFreePoolWithTagCPP(_Frees_ptr_ void* p, ULONG tag)
{
    UNREFERENCED_PARAMETER(tag);
    if (p == nullptr) {
        KeBugCheckExCPP(BAD_POOL_CALLER, 0x46, 0, 0, 0);
    }
    usersim_free(p);
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
    if (usersim_fault_injection_inject_fault()) {
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

void
usersim_initialize_ex(bool leak_detector)
{
    if (leak_detector) {
        _usersim_leak_detector_ptr = std::make_unique<usersim_leak_detector_t>();
    }
}

void
usersim_clean_up_ex()
{
    if (_usersim_leak_detector_ptr) {
        _usersim_leak_detector_ptr->dump_leaks();
        _usersim_leak_detector_ptr.reset();
    }
}