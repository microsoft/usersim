// Copyright (c) Microsoft Corporation
// SPDX-License-Identifier: MIT
#pragma once

#include "framework.h"
#include "kernel_um.h"

typedef NTSTATUS usersim_result_t;

#include <TraceLoggingProvider.h>
#include <winmeta.h>

typedef intptr_t usersim_handle_t;

#ifdef __cplusplus
extern "C"
{
#endif

#define USERSIM_COUNT_OF(arr) (sizeof(arr) / sizeof(arr[0]))
#define USERSIM_OFFSET_OF(s, m) (((size_t) & ((s*)0)->m))
#define USERSIM_FROM_FIELD(s, m, o) (s*)((uint8_t*)o - USERSIM_OFFSET_OF(s, m))

#define USERSIM_UTF8_STRING_FROM_CONST_STRING(x) \
    {                                            \
        ((uint8_t*)(x)), sizeof((x)) - 1         \
    }

#define USERSIM_CACHE_LINE_SIZE 64
#define USERSIM_CACHE_ALIGN_POINTER(P) \
    (void*)(((uintptr_t)P + USERSIM_CACHE_LINE_SIZE - 1) & ~(USERSIM_CACHE_LINE_SIZE - 1))
#define USERSIM_PAD_CACHE(X) ((X + USERSIM_CACHE_LINE_SIZE - 1) & ~(USERSIM_CACHE_LINE_SIZE - 1))
#define USERSIM_PAD_8(X) ((X + 7) & ~7)

#define USERSIM_NS_PER_FILETIME 100

// Macro locally suppresses "Unreferenced variable" warning, which in 'Release' builds is treated as an error.
#define usersim_assert_success(x)                                  \
    _Pragma("warning(push)") _Pragma("warning(disable : 4189)") do \
    {                                                              \
        usersim_result_t _result = (x);                            \
        usersim_assert(_result == STATUS_SUCCESS && #x);           \
    }                                                              \
    while (0)                                                      \
    _Pragma("warning(pop)")

    /**
     * @brief A UTF-8 encoded string.
     * Notes:
     * 1) This string is not NULL terminated, instead relies on length.
     * 2) A single UTF-8 code point (aka character) could be 1-4 bytes in
     *  length.
     *
     */
    typedef struct _usersim_utf8_string
    {
        uint8_t* value;
        size_t length;
    } usersim_utf8_string_t;

    typedef enum _usersim_code_integrity_state
    {
        USERSIM_CODE_INTEGRITY_DEFAULT = 0,
        USERSIM_CODE_INTEGRITY_HYPERVISOR_KERNEL_MODE = 1
    } usersim_code_integrity_state_t;

    typedef struct _usersim_non_preemptible_work_item usersim_non_preemptible_work_item_t;
    typedef struct _usersim_preemptible_work_item usersim_preemptible_work_item_t;
    typedef struct _usersim_timer_work_item usersim_timer_work_item_t;

    typedef struct _usersim_trampoline_table usersim_trampoline_table_t;

    typedef uintptr_t usersim_lock_t;
    typedef uint8_t usersim_lock_state_t;

    typedef struct _usersim_process_state usersim_process_state_t;

    // A self-relative security descriptor.
    typedef struct _SECURITY_DESCRIPTOR usersim_security_descriptor_t;
    typedef struct _GENERIC_MAPPING usersim_security_generic_mapping_t;
    typedef uint32_t usersim_security_access_mask_t;

    extern bool usersim_fuzzing_enabled;

    /**
     * @brief Initialize the usersim platform abstraction layer.
     * @retval USERSIM_SUCCESS The operation was successful.
     * @retval USERSIM_NO_MEMORY Unable to allocate resources for this
     *  operation.
     */
    _Must_inspect_result_ usersim_result_t
    usersim_platform_initiate();

    /**
     * @brief Terminate the usersim platform abstraction layer.
     */
    void
    usersim_platform_terminate();

    /**
     * @brief Allocate memory.
     * @param[in] size Size of memory to allocate.
     * @returns Pointer to memory block allocated, or null on failure.
     */
    __drv_allocatesMem(Mem) _Must_inspect_result_ _Ret_writes_maybenull_(size) void* usersim_allocate(size_t size);

    /**
     * @brief Allocate memory.
     * @param[in] size Size of memory to allocate.
     * @param[in] tag Pool tag to use.
     * @param[in] initialize False to return "uninitialized" memory.
     * @returns Pointer to memory block allocated, or null on failure.
     */
    __drv_allocatesMem(Mem) _Must_inspect_result_
        _Ret_writes_maybenull_(size) void* usersim_allocate_with_tag(size_t size, uint32_t tag, bool initialize);

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

    typedef enum _usersim_page_protection
    {
        USERSIM_PAGE_PROTECT_READ_ONLY,
        USERSIM_PAGE_PROTECT_READ_WRITE,
        USERSIM_PAGE_PROTECT_READ_EXECUTE,
    } usersim_page_protection_t;

    typedef struct _usersim_ring_descriptor usersim_ring_descriptor_t;

    /**
     * @brief Allocate pages from physical memory and create a mapping into the
     * system address space with the same pages mapped twice.
     *
     * @param[in] length Size of memory to allocate (internally this gets rounded
     * up to a page boundary).
     * @return Pointer to an usersim_memory_descriptor_t on success, NULL on failure.
     */
    _Ret_maybenull_ usersim_ring_descriptor_t*
    usersim_allocate_ring_buffer_memory(size_t length);

    /**
     * @brief Release physical memory previously allocated via usersim_allocate_ring_buffer_memory.
     *
     * @param[in] memory_descriptor Pointer to usersim_ring_descriptor_t describing
     * allocated pages.
     */
    void
    usersim_free_ring_buffer_memory(_Frees_ptr_opt_ usersim_ring_descriptor_t* ring);

    /**
     * @brief Given an usersim_ring_descriptor_t allocated via usersim_allocate_ring_buffer_memory
     * obtain the base virtual address.
     *
     * @param[in] memory_descriptor Pointer to an usersim_ring_descriptor_t
     * describing allocated pages.
     * @return Base virtual address of pages that have been allocated.
     */
    void*
    usersim_ring_descriptor_get_base_address(_In_ const usersim_ring_descriptor_t* ring);

    /**
     * @brief Create a read-only mapping in the calling process of the ring buffer.
     *
     * @param[in] ring Ring buffer to map.
     * @return Pointer to the base of the ring buffer.
     */
    _Ret_maybenull_ void*
    usersim_ring_map_readonly_user(_In_ const usersim_ring_descriptor_t* ring);

    /**
     * @brief Allocate and copy a UTF-8 string.
     *
     * @param[out] destination Pointer to memory where the new UTF-8 character
     * sequence will be allocated.
     * @param[in] source UTF-8 string that will be copied.
     * @retval USERSIM_SUCCESS The operation was successful.
     * @retval USERSIM_NO_MEMORY Unable to allocate resources for this
     *  UTF-8 string.
     */
    _Must_inspect_result_ usersim_result_t
    usersim_duplicate_utf8_string(_Out_ usersim_utf8_string_t* destination, _In_ const usersim_utf8_string_t* source);

    /**
     * @brief Free a UTF-8 string allocated by usersim_duplicate_utf8_string.
     *
     * @param[in,out] string The string to free.
     */
    void
    usersim_utf8_string_free(_Inout_ usersim_utf8_string_t* string);

    /**
     * @brief Duplicate a null-terminated string.
     *
     * @param[in] source String to duplicate.
     * @return Pointer to the duplicated string or NULL if out of memory.
     */
    _Must_inspect_result_ char*
    usersim_duplicate_string(_In_z_ const char* source);

    /**
     * @brief Get the code integrity state from the platform.
     * @param[out] state The code integrity state being enforced.
     * @retval USERSIM_SUCCESS The operation was successful.
     * @retval USERSIM_NOT_SUPPORTED Unable to obtain state from platform.
     */
    _Must_inspect_result_ usersim_result_t
    usersim_get_code_integrity_state(_Out_ usersim_code_integrity_state_t* state);

    /**
     * @brief Multiplies one value of type size_t by another and check for
     *   overflow.
     * @param[in] multiplicand The value to be multiplied by multiplier.
     * @param[in] multiplier The value by which to multiply multiplicand.
     * @param[out] result A pointer to the result.
     * @retval USERSIM_SUCCESS The operation was successful.
     * @retval USERSIM_ERROR_ARITHMETIC_OVERFLOW Multiplication overflowed.
     */
    _Must_inspect_result_ usersim_result_t
    usesim_safe_size_t_multiply(
        size_t multiplicand, size_t multiplier, _Out_ _Deref_out_range_(==, multiplicand* multiplier) size_t* result);

    /**
     * @brief Add one value of type size_t by another and check for
     *   overflow.
     * @param[in] augend The value to be added by addend.
     * @param[in] addend The value add to augend.
     * @param[out] result A pointer to the result.
     * @retval USERSIM_SUCCESS The operation was successful.
     * @retval USERSIM_ERROR_ARITHMETIC_OVERFLOW Addition overflowed.
     */
    _Must_inspect_result_ usersim_result_t
    usersim_safe_size_t_add(size_t augend, size_t addend, _Out_ _Deref_out_range_(==, augend + addend) size_t* result);

    /**
     * @brief Subtract one value of type size_t from another and check for
     *   overflow or underflow.
     * @param[in] minuend The value from which subtrahend is subtracted.
     * @param[in] subtrahend The value subtract from minuend.
     * @param[out] result A pointer to the result.
     * @retval USERSIM_SUCCESS The operation was successful.
     * @retval USERSIM_ERROR_ARITHMETIC_OVERFLOW Addition overflowed or underflowed.
     */
    _Must_inspect_result_ usersim_result_t
    usersim_safe_size_t_subtract(
        size_t minuend, size_t subtrahend, _Out_ _Deref_out_range_(==, minuend - subtrahend) size_t* result);

    /**
     * @brief Create an instance of a lock.
     * @param[out] lock Pointer to memory location that will contain the lock.
     */
    void
    usersim_lock_create(_Out_ usersim_lock_t* lock);

    /**
     * @brief Destroy an instance of a lock.
     * @param[in] lock Pointer to memory location that contains the lock.
     */
    void
    usersim_lock_destroy(_In_ _Post_invalid_ usersim_lock_t* lock);

    /**
     * @brief Acquire exclusive access to the lock.
     * @param[in, out] lock Pointer to memory location that contains the lock.
     * @returns The previous lock_state required for unlock.
     */
    _Requires_lock_not_held_(*lock) _Acquires_lock_(*lock) _IRQL_requires_max_(DISPATCH_LEVEL) _IRQL_saves_
        _IRQL_raises_(DISPATCH_LEVEL) usersim_lock_state_t usersim_lock_lock(_Inout_ usersim_lock_t* lock);

    /**
     * @brief Release exclusive access to the lock.
     * @param[in, out] lock Pointer to memory location that contains the lock.
     * @param[in] state The state returned from usersim_lock_lock.
     */
    _Requires_lock_held_(*lock) _Releases_lock_(*lock) _IRQL_requires_(DISPATCH_LEVEL) void usersim_lock_unlock(
        _Inout_ usersim_lock_t* lock, _IRQL_restores_ usersim_lock_state_t state);

    /**
     * @brief Raise the IRQL to new_irql.
     *
     * @param[in] new_irql The new IRQL.
     * @return The previous IRQL.
     */
    _IRQL_requires_max_(HIGH_LEVEL) _IRQL_raises_(new_irql) _IRQL_saves_ uint8_t usersim_raise_irql(uint8_t new_irql);

    /**
     * @brief Lower the IRQL to old_irql.
     *
     * @param[in] old_irql The old IRQL.
     */
    _IRQL_requires_max_(HIGH_LEVEL) void usersim_lower_irql(_In_ _Notliteral_ _IRQL_restores_ uint8_t old_irql);

    /**
     * @brief Query the platform for the total number of CPUs.
     * @return The count of logical cores in the system.
     */
    _Ret_range_(>, 0) uint32_t usersim_get_cpu_count();

    /**
     * @brief Query the platform to determine if the current execution can
     *    be preempted by other execution.
     * @retval True if this execution can be preempted.
     */
    bool
    usersim_is_preemptible();

    ULONG
    KeGetCurrentProcessorNumberEx(_Out_opt_ PPROCESSOR_NUMBER ProcNumber);

    /**
     * @brief Query the platform to determine an opaque identifier for the
     *   current thread. Only valid if usersim_is_preemptible() == false.
     * @return Opaque identifier for the current thread.
     */
    uint64_t
    usersim_get_current_thread_id();

    /**
     * @brief Query the platform to determine if non-preemptible work items are
     *   supported.
     *
     * @retval true Non-preemptible work items are supported.
     * @retval false Non-preemptible work items are not supported.
     */
    bool
    usersim_is_non_preemptible_work_item_supported();

    /**
     * @brief Create a non-preemptible work item.
     *
     * @param[out] work_item Pointer to memory that will contain the pointer to
     *  the non-preemptible work item on success.
     * @param[in] cpu_id Associate the work item with this CPU.
     * @param[in] work_item_routine Routine to execute as a work item.
     * @param[in, out] work_item_context Context to pass to the routine.
     * @retval USERSIM_SUCCESS The operation was successful.
     * @retval USERSIM_NO_MEMORY Unable to allocate resources for this
     *  work item.
     */
    _Must_inspect_result_ usersim_result_t
    usersim_allocate_non_preemptible_work_item(
        _Outptr_ usersim_non_preemptible_work_item_t** work_item,
        uint32_t cpu_id,
        _In_ void (*work_item_routine)(_Inout_opt_ void* work_item_context, _Inout_opt_ void* parameter_1),
        _Inout_opt_ void* work_item_context);

    /**
     * @brief Free a non-preemptible work item.
     *
     * @param[in] work_item Pointer to the work item to free.
     */
    void
    usersim_free_non_preemptible_work_item(_Frees_ptr_opt_ usersim_non_preemptible_work_item_t* work_item);

    /**
     * @brief Schedule a non-preemptible work item to run.
     *
     * @param[in, out] work_item Work item to schedule.
     * @param[in, out] parameter_1 Parameter to pass to work item.
     * @retval true Work item was queued.
     * @retval false Work item is already queued.
     */
    bool
    usersim_queue_non_preemptible_work_item(
        _Inout_ usersim_non_preemptible_work_item_t* work_item, _Inout_opt_ void* parameter_1);

    /**
     * @brief Create a preemptible work item.
     *
     * @param[out] work_item Pointer to memory that will contain the pointer to
     *  the preemptible work item on success.
     * @param[in] work_item_routine Routine to execute as a work item.
     * @param[in, out] work_item_context Context to pass to the routine.
     * @retval USERSIM_SUCCESS The operation was successful.
     * @retval USERSIM_NO_MEMORY Unable to allocate resources for this
     *  work item.
     */
    _Must_inspect_result_ usersim_result_t
    usersim_allocate_preemptible_work_item(
        _Outptr_ usersim_preemptible_work_item_t** work_item,
        _In_ void (*work_item_routine)(_Inout_opt_ void* work_item_context),
        _Inout_opt_ void* work_item_context);

    /**
     * @brief Free a preemptible work item.
     *
     * @param[in] work_item Pointer to the work item to free.
     */
    void
    usersim_free_preemptible_work_item(_Frees_ptr_opt_ usersim_preemptible_work_item_t* work_item);

    /**
     * @brief Schedule a preemptible work item to run.
     *
     * @param[in, out] work_item Work item to schedule.
     */
    void
    usersim_queue_preemptible_work_item(_Inout_ usersim_preemptible_work_item_t* work_item);

    /**
     * @brief Allocate a timer to run a non-preemptible work item.
     *
     * @param[out] timer Pointer to memory that will contain timer on success.
     * @param[in] work_item_routine Routine to execute when time expires.
     * @param[in, out] work_item_context Context to pass to routine.
     * @retval USERSIM_SUCCESS The operation was successful.
     * @retval USERSIM_NO_MEMORY Unable to allocate resources for this
     *  timer.
     */
    _Must_inspect_result_ usersim_result_t
    usersim_allocate_timer_work_item(
        _Outptr_ usersim_timer_work_item_t** timer,
        _In_ void (*work_item_routine)(_Inout_opt_ void* work_item_context),
        _Inout_opt_ void* work_item_context);

    /**
     * @brief Schedule a work item to be executed after elapsed_microseconds.
     *
     * @param[in, out] timer Pointer to timer to schedule.
     * @param[in] elapsed_microseconds Microseconds to delay before executing
     *   work item.
     */
    void
    usersim_schedule_timer_work_item(_Inout_ usersim_timer_work_item_t* timer, uint32_t elapsed_microseconds);

    /**
     * @brief Free a timer.
     *
     * @param[in] timer Timer to be freed.
     */
    void
    usersim_free_timer_work_item(_Frees_ptr_opt_ usersim_timer_work_item_t* timer);

    /**
     * @brief Atomically increase the value of addend by 1 and return the new
     *  value.
     *
     * @param[in, out] addend Value to increase by 1.
     * @return The new value.
     */
    int32_t
    usersim_interlocked_increment_int32(_Inout_ volatile int32_t* addend);

    /**
     * @brief Atomically decrease the value of addend by 1 and return the new
     *  value.
     *
     * @param[in, out] addend Value to decrease by 1.
     * @return The new value.
     */
    int32_t
    usersim_interlocked_decrement_int32(_Inout_ volatile int32_t* addend);

    /**
     * @brief Atomically increase the value of addend by 1 and return the new
     *  value.
     *
     * @param[in, out] addend Value to increase by 1.
     * @return The new value.
     */
    int64_t
    usersim_interlocked_increment_int64(_Inout_ volatile int64_t* addend);

    /**
     * @brief Atomically decrease the value of addend by 1 and return the new
     *  value.
     *
     * @param[in, out] addend Value to increase by 1.
     * @return The new value.
     */
    int64_t
    usersim_interlocked_decrement_int64(_Inout_ volatile int64_t* addend);

    /**
     * @brief Performs an atomic operation that compares the input value pointed
     *  to by destination with the value of comparand and replaces it with
     *  exchange.
     *
     * @param[in, out] destination A pointer to the input value that is compared
     *  with the value of comparand.
     * @param[in] exchange Specifies the output value pointed to by destination
     *  if the input value pointed to by destination equals the value of
     *  comparand.
     * @param[in] comparand Specifies the value that is compared with the input
     *  value pointed to by destination.
     * @return Returns the original value of memory pointed to by
     *  destination.
     */
    int32_t
    usersim_interlocked_compare_exchange_int32(
        _Inout_ volatile int32_t* destination, int32_t exchange, int32_t comparand);

    /**
     * @brief Performs an atomic operation that compares the input value pointed
     *  to by destination with the value of comparand and replaces it with
     *  exchange.
     *
     * @param[in, out] destination A pointer to the input value that is compared
     *  with the value of comparand.
     * @param[in] exchange Specifies the output value pointed to by destination
     *  if the input value pointed to by destination equals the value of
     *  comparand.
     * @param[in] comparand Specifies the value that is compared with the input
     *  value pointed to by destination.
     * @return Returns the original value of memory pointed to by
     *  destination.
     */
    int64_t
    usersim_interlocked_compare_exchange_int64(
        _Inout_ volatile int64_t* destination, int64_t exchange, int64_t comparand);

    /**
     * @brief Performs an atomic operation that compares the input value pointed
     *  to by destination with the value of comparand and replaces it with
     *  exchange.
     *
     * @param[in, out] destination A pointer to the input value that is compared
     *  with the value of comparand.
     * @param[in] exchange Specifies the output value pointed to by destination
     *  if the input value pointed to by destination equals the value of
     *  comparand.
     * @param[in] comparand Specifies the value that is compared with the input
     *  value pointed to by destination.
     * @return Returns the original value of memory pointed to by
     *  destination.
     */
    void*
    usersim_interlocked_compare_exchange_pointer(
        _Inout_ void* volatile* destination, _In_opt_ const void* exchange, _In_opt_ const void* comparand);

    /**
     * @brief Performs an atomic OR of the value stored at destination with mask and stores the result in destination.
     *
     * @param[in, out] destination A pointer to the memory for this operation to be applied to.
     * @param[in] mask Value to be applied to the value stored at the destination.
     * @return The original value stored at destination.
     */
    int32_t
    usersim_interlocked_or_int32(_Inout_ volatile int32_t* destination, int32_t mask);

    /**
     * @brief Performs an atomic AND of the value stored at destination with mask and stores the result in destination.
     *
     * @param[in, out] destination A pointer to the memory for this operation to be applied to.
     * @param[in] mask Value to be applied to the value stored at the destination.
     * @return The original value stored at destination.
     */
    int32_t
    usersim_interlocked_and_int32(_Inout_ volatile int32_t* destination, int32_t mask);

    /**
     * @brief Performs an atomic XOR of the value stored at destination with mask and stores the result in destination.
     *
     * @param[in, out] destination A pointer to the memory for this operation to be applied to.
     * @param[in] mask Value to be applied to the value stored at the destination.
     * @return The original value stored at destination.
     */
    int32_t
    usersim_interlocked_xor_int32(_Inout_ volatile int32_t* destination, int32_t mask);

    /**
     * @brief Performs an atomic OR of the value stored at destination with mask and stores the result in destination.
     *
     * @param[in, out] destination A pointer to the memory for this operation to be applied to.
     * @param[in] mask Value to be applied to the value stored at the destination.
     * @return The original value stored at destination.
     */
    int64_t
    usersim_interlocked_or_int64(_Inout_ volatile int64_t* destination, int64_t mask);

    /**
     * @brief Performs an atomic AND of the value stored at destination with mask and stores the result in destination.
     *
     * @param[in, out] destination A pointer to the memory for this operation to be applied to.
     * @param[in] mask Value to be applied to the value stored at the destination.
     * @return The original value stored at destination.
     */
    int64_t
    usersim_interlocked_and_int64(_Inout_ volatile int64_t* destination, int64_t mask);

    /**
     * @brief Performs an atomic XOR of the value stored at destination with mask and stores the result in destination.
     *
     * @param[in, out] destination A pointer to the memory for this operation to be applied to.
     * @param[in] mask Value to be applied to the value stored at the destination.
     * @return The original value stored at destination.
     */
    int64_t
    usersim_interlocked_xor_int64(_Inout_ volatile int64_t* destination, int64_t mask);

    int32_t
    usersim_log_function(_In_ void* context, _In_z_ const char* format_string, ...);

    /**
     * @brief Check if the user associated with the current thread is granted
     *  the rights requested.
     *
     * @param[in] security_descriptor Security descriptor representing the
     *  security policy.
     * @param[in] request_access Access the caller is requesting.
     * @param[in] generic_mapping Mappings for generic read/write/execute to
     *  specific rights.
     * @retval USERSIM_SUCCESS Requested access is granted.
     * @retval USERSIM_ACCESS_DENIED Requested access is denied.
     */
    _Must_inspect_result_ usersim_result_t
    usersim_access_check(
        _In_ const usersim_security_descriptor_t* security_descriptor,
        usersim_security_access_mask_t request_access,
        _In_ const usersim_security_generic_mapping_t* generic_mapping);

    /**
     * @brief Check the validity of the provided security descriptor.
     *
     * @param[in] security_descriptor Security descriptor to verify.
     * @param[in] security_descriptor_length Length of security descriptor.
     * @retval USERSIM_SUCCESS Security descriptor is well formed.
     * @retval USERSIM_INVALID_ARGUMENT Security descriptor is malformed.
     */
    _Must_inspect_result_ usersim_result_t
    usersim_validate_security_descriptor(
        _In_ const usersim_security_descriptor_t* security_descriptor, size_t security_descriptor_length);

    /**
     * @brief Return a pseudorandom number.
     *
     * @return A pseudorandom number.
     */
    uint32_t
    usersim_random_uint32();

    /**
     * @brief Return time elapsed since boot in units of 100 nanoseconds.
     *
     * @param[in] include_suspended_time Include time the system spent in a suspended state.
     * @return Time elapsed since boot in 100 nanosecond units.
     */
    uint64_t
    usersim_query_time_since_boot(bool include_suspended_time);

    _Must_inspect_result_ usersim_result_t
    usersim_set_current_thread_affinity(uintptr_t new_thread_affinity_mask, _Out_ uintptr_t* old_thread_affinity_mask);

    void
    usersim_restore_current_thread_affinity(uintptr_t old_thread_affinity_mask);

    typedef _Return_type_success_(return >= 0) LONG NTSTATUS;

    /**
     * @brief Map a usersim_result_t to a generic Win32 error code.
     *
     * @param[in] result usersim_result_t to map.
     * @return The generic Win32 error code.
     */
    uint32_t
    usersim_result_to_win32_error_code(usersim_result_t result);

    /**
     * @brief Output a debug message.
     *
     * @param[in] format Format string.
     * @param[in] arg_list Argument list.
     *
     * @returns Number of bytes written, or -1 on error.
     */
    long
    usersim_platform_printk(_In_z_ const char* format, va_list arg_list);

    /**
     * @brief Get a handle to the current process.
     *
     * @return Handle to the current process.
     */
    intptr_t
    usersim_platform_reference_process();

    /**
     * @brief Dereference a handle to a process.
     *
     * @param[in] process_handle to the process.
     */
    void
    usersim_platform_dereference_process(intptr_t process_handle);

    /**
     * @brief Attach to the specified process.
     *
     * @param[in] handle to the process.
     * @param[in,out] state Pointer to the process state.
     */
    void
    usersim_platform_attach_process(intptr_t process_handle, _Inout_ usersim_process_state_t* state);

    /**
     * @brief Detach from the current process.
     *
     * @param[in] state Pointer to the process state.
     */
    void
    usersim_platform_detach_process(_In_ usersim_process_state_t* state);

    TRACELOGGING_DECLARE_PROVIDER(usersim_tracelog_provider);

    _Must_inspect_result_ usersim_result_t
    usersim_trace_initiate();

    void
    usersim_trace_terminate();

    typedef struct _usersim_cryptographic_hash usersim_cryptographic_hash_t;

    /**
     * @brief Create a cryptographic hash object.
     *
     * @param[in] algorithm The algorithm to use. Recommended value is "SHA256".
     *  The CNG algorithm name to use is listed in
     *  https://learn.microsoft.com/en-us/windows/win32/seccng/cng-algorithm-identifiers
     * @param[out] hash The hash object.
     * @return USERSIM_SUCCESS The hash object was created.
     * @return USERSIM_NO_MEMORY Unable to allocate memory for the hash object.
     * @return USERSIM_INVALID_ARGUMENT The algorithm is not supported.
     */
    _Must_inspect_result_ usersim_result_t
    usersim_cryptographic_hash_create(
        _In_ const usersim_utf8_string_t* algorithm, _Outptr_ usersim_cryptographic_hash_t** hash);

    /**
     * @brief Destroy a cryptographic hash object.
     *
     * @param[in] hash The hash object to destroy.
     */
    void
    usersim_cryptographic_hash_destroy(_In_opt_ _Frees_ptr_opt_ usersim_cryptographic_hash_t* hash);

    /**
     * @brief Append data to a cryptographic hash object.
     *
     * @param[in] hash The hash object to update.
     * @param[in] buffer The data to append.
     * @param[in] length The length of the data to append.
     * @return USERSIM_SUCCESS The hash object was created.
     * @return USERSIM_INVALID_ARGUMENT An error occurred while computing the hash.
     */
    _Must_inspect_result_ usersim_result_t
    usersim_cryptographic_hash_append(
        _Inout_ usersim_cryptographic_hash_t* hash, _In_reads_bytes_(length) const uint8_t* buffer, size_t length);

    /**
     * @brief Finalize the hash and return the hash value.
     *
     * @param[in, out] hash The hash object to finalize.
     * @param[out] buffer The buffer to receive the hash value.
     * @param[in] input_length The length of the buffer.
     * @param[out] output_length The length of the hash value.
     * @return USERSIM_SUCCESS The hash object was created.
     * @return USERSIM_INVALID_ARGUMENT An error occurred while computing the hash.
     * @return USERSIM_INSUFFICIENT_BUFFER The buffer is not large enough to receive the hash value.
     */
    _Must_inspect_result_ usersim_result_t
    usersim_cryptographic_hash_get_hash(
        _Inout_ usersim_cryptographic_hash_t* hash,
        _Out_writes_to_(input_length, *output_length) uint8_t* buffer,
        size_t input_length,
        _Out_ size_t* output_length);

    _Must_inspect_result_ usersim_result_t
    usersim_cryptographic_hash_get_hash_length(_In_ const usersim_cryptographic_hash_t* hash, _Out_ size_t* length);

    /**
     * @brief Should the current thread yield the processor?
     *
     * @retval true Thread should yield the processor.
     * @retval false Thread should not yield the processor.
     */
    bool
    usersim_should_yield_processor();

/**
 * @brief Append a value to a cryptographic hash object.
 * @param[in] hash The hash object to update.
 * @param[in] value The value to append. Size is determined by the type of the value.
 */
#define USERSIM_CRYPTOGRAPHIC_HASH_APPEND_VALUE(hash, value) \
    usersim_cryptographic_hash_append(hash, (const uint8_t*)&(value), sizeof((value)))

/**
 * @brief Append a value to a cryptographic hash object.
 * @param[in] hash The hash object to update.
 * @param[in] string The string to append. Size is determined by the length of the string.
 */
#define USERSIM_CRYPTOGRAPHIC_HASH_APPEND_STR(hash, string) \
    usersim_cryptographic_hash_append(hash, (const uint8_t*)(string), strlen(string))

    /**
     * @brief Get 64-bit Authentication ID for the current user.
     *
     * @param[out] authentication_id The authentication ID.
     *
     * @return result of the operation.
     */
    _IRQL_requires_max_(PASSIVE_LEVEL) _Must_inspect_result_ usersim_result_t
        usersim_platform_get_authentication_id(_Out_ uint64_t* authentication_id);

    typedef struct _usersim_semaphore usersim_semaphore_t;

    /**
     * @brief Create a semaphore.
     *
     * @param[out] semaphore Pointer to the memory that contains the semaphore.
     * @param[in] initial_count Initial count of the semaphore.
     * @param[in] maximum_count Maximum count of the semaphore.
     * @retval USERSIM_SUCCESS The hash object was created.
     * @retval USERSIM_NO_MEMORY Unable to allocate resources for the semaphore.
     */
    _Must_inspect_result_ usersim_result_t
    usersim_semaphore_create(_Outptr_ usersim_semaphore_t** semaphore, int initial_count, int maximum_count);

    /**
     * @brief Destroy a semaphore.
     *
     * @param[in] semaphore Semaphore to destroy.
     */
    void
    usersim_semaphore_destroy(_Frees_ptr_opt_ usersim_semaphore_t* semaphore);

    /**
     * @brief Wait on a semaphore.
     *
     * @param[in] semaphore Semaphore to wait on.
     */
    void
    usersim_semaphore_wait(_In_ usersim_semaphore_t* semaphore);

    /**
     * @brief Release a semaphore.
     *
     * @param[in] semaphore Semaphore to release.
     */
    void
    usersim_semaphore_release(_In_ usersim_semaphore_t* semaphore);

    /**
     * @brief Enter a critical region. This will defer execution of kernel APCs
     * until usersim_leave_critical_region is called.
     */
    void
    usersim_enter_critical_region();

    /**
     * @brief Leave a critical region. This will resume execution of kernel APCs.
     */
    void
    usersim_leave_critical_region();

    /**
     * @brief Convert the provided UTF-8 string into a UTF-16LE string.
     *
     * @param[in] input UTF-8 string to convert.
     * @param[out] output Converted UTF-16LE string.
     * @retval USERSIM_SUCCESS The conversion was successful.
     * @retval USERSIM_NO_MEMORY Unable to allocate resources for the conversion.
     * @retval USERSIM_INVALID_ARGUMENT Unable to convert the string.
     */
    usersim_result_t
    usersim_utf8_string_to_unicode(_In_ const usersim_utf8_string_t* input, _Outptr_ wchar_t** output);

#ifdef __cplusplus
}
#endif

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
