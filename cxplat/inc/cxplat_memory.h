// Copyright (c) Microsoft Corporation
// SPDX-License-Identifier: MIT
#pragma once

#include <stdint.h>
#include <driverspecs.h>

#ifdef __cplusplus
extern "C"
{
#endif

    // Values in this enum line up with POOL_TYPE in Windows,
    // and other values are legal.
    typedef enum
    {
        CxPlatNonPagedPoolNx = 512,
        CxPlatNonPagedPoolNxCacheAligned = CxPlatNonPagedPoolNx + 4,
    } cxplat_pool_type_t;

    /**
     * @brief Allocate memory.
     * @param[in] size Size of memory to allocate.
     * @returns Pointer to memory block allocated, or null on failure.
     */
    __drv_allocatesMem(Mem) _Must_inspect_result_ _Ret_writes_maybenull_(size) void* cxplat_allocate(size_t size);

    /**
     * @brief Allocate memory.
     * @param[in] pool_type Type of pool to use.
     * @param[in] size Size of memory to allocate.
     * @param[in] tag Pool tag to use.
     * @param[in] initialize False to return "uninitialized" memory.
     * @returns Pointer to memory block allocated, or null on failure.
     */
    __drv_allocatesMem(Mem) _Must_inspect_result_ _Ret_writes_maybenull_(size) void* cxplat_allocate_with_tag(
        _In_ cxplat_pool_type_t pool_type,
        size_t size,
        uint32_t tag,
        bool initialize);

    /**
     * @brief Reallocate memory.
     * @param[in] memory Allocation to be reallocated.
     * @param[in] old_size Old size of memory to reallocate.
     * @param[in] new_size New size of memory to reallocate.
     * @returns Pointer to memory block allocated, or null on failure.
     */
    __drv_allocatesMem(Mem) _Must_inspect_result_ _Ret_writes_maybenull_(new_size) void* cxplat_reallocate(
        _In_ _Post_invalid_ void* memory, size_t old_size, size_t new_size);

    /**
     * @brief Reallocate memory with tag.
     * @param[in] memory Allocation to be reallocated.
     * @param[in] old_size Old size of memory to reallocate.
     * @param[in] new_size New size of memory to reallocate.
     * @param[in] tag Pool tag to use.
     * @returns Pointer to memory block allocated, or null on failure.
     */
    __drv_allocatesMem(Mem) _Must_inspect_result_ _Ret_writes_maybenull_(new_size) void* cxplat_reallocate_with_tag(
        _In_ _Post_invalid_ void* memory, size_t old_size, size_t new_size, uint32_t tag);

    /**
     * @brief Free memory.
     * @param[in] memory Allocation to be freed.
     */
    void
    cxplat_free(_Frees_ptr_opt_ void* memory);

    /**
     * @brief Allocate memory that has a starting address that is cache aligned.
     * @param[in] size Size of memory to allocate
     * @returns Pointer to memory block allocated, or null on failure.
     */
    __drv_allocatesMem(Mem) _Must_inspect_result_
        _Ret_writes_maybenull_(size) void* cxplat_allocate_cache_aligned(size_t size);

    /**
     * @brief Allocate memory that has a starting address that is cache aligned with tag.
     * @param[in] size Size of memory to allocate
     * @param[in] tag Pool tag to use.
     * @returns Pointer to memory block allocated, or null on failure.
     */
    __drv_allocatesMem(Mem) _Must_inspect_result_
        _Ret_writes_maybenull_(size) void* cxplat_allocate_cache_aligned_with_tag(size_t size, uint32_t tag);

    /**
     * @brief Free memory that has a starting address that is cache aligned.
     * @param[in] memory Allocation to be freed.
     */
    void
    cxplat_free_cache_aligned(_Frees_ptr_opt_ void* memory);

#ifdef __cplusplus
}
#endif