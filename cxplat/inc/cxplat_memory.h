// Copyright (c) Microsoft Corporation
// SPDX-License-Identifier: MIT
#pragma once

#pragma warning(push)
#pragma warning(disable : 4005 4083 4616)
#include <driverspecs.h>
#include <stdint.h>
#pragma warning(pop)

CXPLAT_EXTERN_C_BEGIN

// Values in this enum line up with POOL_TYPE in Windows,
// and other values are legal.
typedef enum
{
    CxPlatNonPagedPoolNx = 512,
    CxPlatNonPagedPoolNxCacheAligned = CxPlatNonPagedPoolNx + 4,
} cxplat_pool_type_t;

/**
 * @brief A UTF-8 encoded string.
 * Notes:
 * 1) This string is not NULL terminated, instead relies on length.
 * 2) A single UTF-8 code point (aka character) could be 1-4 bytes in
 *  length.
 *
 */
typedef struct _cxplat_utf8_string
{
    uint8_t* value;
    size_t length;
} cxplat_utf8_string_t;

#define CXPLAT_UTF8_STRING_FROM_CONST_STRING(x) \
    {                                           \
        ((uint8_t*)(x)), sizeof((x)) - 1        \
    }

/**
 * @brief Allocate memory. cxplat_allocate_with_tag() should normally be used instead of this API.
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
    _In_ cxplat_pool_type_t pool_type, size_t size, uint32_t tag, bool initialize);

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
 * @param[in] size Size of memory to allocate.
 * @param[in] tag Pool tag to use.
 * @returns Pointer to zero-initialized memory block allocated, or null on failure.
 */
__drv_allocatesMem(Mem) _Must_inspect_result_
    _Ret_writes_maybenull_(size) void* cxplat_allocate_cache_aligned_with_tag(size_t size, uint32_t tag);

/**
 * @brief Free memory that has a starting address that is cache aligned.
 * @param[in] memory Allocation to be freed.
 */
void
cxplat_free_cache_aligned(_Frees_ptr_opt_ void* memory);

/**
 * @brief Allocate and copy a UTF-8 string.
 *
 * @param[out] destination Pointer to memory where the new UTF-8 character
 * sequence will be allocated.
 * @param[in] source UTF-8 string that will be copied.
 * @retval CXPLAT_STATUS_SUCCESS The operation was successful.
 * @retval CXPLAT_STATUS_NO_MEMORY Unable to allocate resources for this
 *  UTF-8 string.
 */
_Must_inspect_result_ cxplat_status_t
cxplat_duplicate_utf8_string(_Out_ cxplat_utf8_string_t* destination, _In_ const cxplat_utf8_string_t* source);

/**
 * @brief Free a UTF-8 string allocated by cxplat_duplicate_utf8_string.
 *
 * @param[in,out] string The string to free.
 */
void
cxplat_utf8_string_free(_Inout_ cxplat_utf8_string_t* string);

/**
 * @brief Duplicate a null-terminated string.
 *
 * @param[in] source String to duplicate.
 * @return Pointer to the duplicated string or NULL if out of memory.
 */
_Must_inspect_result_ _Ret_maybenull_z_ char*
cxplat_duplicate_string(_In_z_ const char* source);

CXPLAT_EXTERN_C_END
