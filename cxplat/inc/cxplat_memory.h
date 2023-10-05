// Copyright (c) Microsoft Corporation
// SPDX-License-Identifier: MIT
#pragma once

#pragma warning(push)
#pragma warning(disable : 4005 4083 4616)
#include <driverspecs.h>
#include <stdint.h>
#pragma warning(pop)

CXPLAT_EXTERN_C_BEGIN

// Values in this enum line up with POOL_FLAGS in Windows,
// and other values are legal.
typedef _Enum_is_bitflag_ enum
{
    CXPLAT_POOL_FLAG_NONE          = 0x00000000,
    CXPLAT_POOL_FLAG_CACHE_ALIGNED = 0x00000008,
    CXPLAT_POOL_FLAG_UNINITIALIZED = 0x00000002,
    CXPLAT_POOL_FLAG_NON_PAGED     = 0x00000040,
    CXPLAT_POOL_FLAG_PAGED         = 0x00000100,
} cxplat_pool_flags_t;

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
 * @brief Allocate memory.
 * @param[in] pool_flags Pool flags to use.
 * @param[in] size Size of memory to allocate.
 * @param[in] tag Pool tag to use.
 * @returns Pointer to memory block allocated, or null on failure.
 */
__drv_allocatesMem(Mem) _Must_inspect_result_ _Ret_writes_maybenull_(size) void* cxplat_allocate(
    cxplat_pool_flags_t pool_flags, size_t size, uint32_t tag);

/**
 * @brief Reallocate memory.
 * @param[in] memory Allocation to be reallocated.
 * @param[in] pool_flags Pool flags to use.
 * @param[in] old_size Old size of memory to reallocate.
 * @param[in] new_size New size of memory to reallocate.
 * @param[in] tag Pool tag to use.
 * @returns Pointer to memory block allocated, or null on failure.
 */
__drv_allocatesMem(Mem) _Must_inspect_result_ _Ret_writes_maybenull_(new_size) void* cxplat_reallocate(
    _In_ _Post_invalid_ void* memory,
    cxplat_pool_flags_t pool_flags,
    size_t old_size,
    size_t new_size,
    uint32_t tag);

/**
 * @brief Free memory.
 * @param[in] memory Allocation to be freed.
 * @param[in] pool_flags Pool flags to use.
 * @param[in] tag Pool tag to use, or 0 for any.
 */
void
cxplat_free(_Frees_ptr_opt_ void* memory, cxplat_pool_flags_t pool_flags, uint32_t tag);

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
cxplat_free_utf8_string(_Inout_ cxplat_utf8_string_t* string);

/**
 * @brief Duplicate a null-terminated string.
 *
 * @param[in] source String to duplicate.
 * @return Pointer to the duplicated string or NULL if out of memory.
 */
_Must_inspect_result_ _Ret_maybenull_z_ char*
cxplat_duplicate_string(_In_z_ const char* source);

/**
 * @brief Free a null-terminated string allocated by cxplat_duplicate_string.
 *
 * @param[in,out] string The string to free.
 */
void
cxplat_free_string(_Frees_ptr_opt_ _In_z_ const char* string);

/**
 * @brief Initialize a lookaside list.
 *
 * @param[out] lookaside The initialized lookaside list.
 * @param[in] size Allocation size of each entry in the lookaside list.
 * @param[in] tag Tag to use for the lookaside list.
 * @param[in] pool_flags Flags to use for the lookaside list.
 * @param[in] allocate Optional allocation callback.
 * @param[in] free Optional free callback.
 * @retval CXPLAT_STATUS_SUCCESS The operation was successful.
 * @retval CXPLAT_STATUS_NO_MEMORY Unable to allocate resources for this lookaside list.
 */
_Must_inspect_result_ cxplat_status_t
cxplat_initialize_lookaside_list(
    _Out_ cxplat_lookaside_list_t* lookaside, _In_ size_t size, _In_ uint32_t tag, _In_ cxplat_pool_flags_t pool_flags);

/**
 * @brief Free a lookaside list.
 *
 * @param[in] lookaside Lookaside list to free.
 */
void
cxplat_uninitialize_lookaside_list(_Inout_ cxplat_lookaside_list_t* lookaside);

/**
 * @brief Allocate an entry from a lookaside list.
 *
 * @param[in,out] lookaside Lookaside list to allocate from.
 * @return Allocation from the lookaside list, or NULL if the allocation failed.
 */
_Must_inspect_result_ void*
cxplat_lookaside_list_alloc(_Inout_ cxplat_lookaside_list_t* lookaside);

/**
 * @brief Free an entry to a lookaside list.
 *
 * @param[in,out] lookaside Lookaside list to free to.
 * @param[in] entry Entry to free.
 */
void
cxplat_lookaside_list_free(_Inout_ cxplat_lookaside_list_t* lookaside, _In_ _Post_invalid_ void* entry);

CXPLAT_EXTERN_C_END
