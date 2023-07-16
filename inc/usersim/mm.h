// Copyright (c) Microsoft Corporation
// SPDX-License-Identifier: MIT

#pragma once
#include <stdint.h>

#if defined(__cplusplus)
extern "C"
{
#endif

#define PAGE_SIZE 0x1000
#define MM_DONT_ZERO_ALLOCATION 0x00000001
#define MM_ALLOCATE_FROM_LOCAL_NODE_ONLY 0x00000002
#define MM_ALLOCATE_FULLY_REQUIRED 0x00000004
#define MM_ALLOCATE_NO_WAIT 0x00000008
#define MM_ALLOCATE_PREFER_CONTIGUOUS 0x00000010
#define MM_ALLOCATE_REQUIRE_CONTIGUOUS_CHUNKS 0x00000020
#define MM_ALLOCATE_FAST_LARGE_PAGES 0x00000040
#define MM_ALLOCATE_TRIM_IF_NECESSARY 0x00000080
#define MM_ALLOCATE_AND_HOT_REMOVE 0x00000100

    typedef enum _MM_PAGE_PRIORITY
    {
        LowPagePriority,
        NormalPagePriority = 16,
        HighPagePriority = 32
    } MM_PAGE_PRIORITY;

    typedef struct _MDL
    {
        struct _MDL* next;
        size_t size;
        uint64_t flags;
        void* start_va;
        unsigned long byte_offset;
        unsigned long byte_count;
    } MDL, *PMDL;

    USERSIM_API
    void
    MmBuildMdlForNonPagedPool(_Inout_ MDL* memory_descriptor_list);

    USERSIM_API
    unsigned long
    MmGetMdlByteCount(_In_ MDL* mdl);

    USERSIM_API
    void*
    MmGetSystemAddressForMdlSafe(
        _Inout_ MDL* mdl,
        _In_ unsigned long page_priority // MM_PAGE_PRIORITY logically OR'd with MdlMapping*
    );

#define MmInitializeMdl(mdl, base_va, length)                                                                     \
    {                                                                                                             \
        (mdl)->next = (PMDL)NULL;                                                                                 \
        (mdl)->size =                                                                                             \
            (uint16_t)(sizeof(MDL) + (sizeof(PFN_NUMBER) * ADDRESS_AND_SIZE_TO_SPAN_PAGES((base_va), (length)))); \
        (mdl)->flags = 0;                                                                                         \
        (mdl)->start_va = (void*)PAGE_ALIGN((base_va));                                                           \
        (mdl)->byte_offset = BYTE_OFFSET((base_va));                                                              \
        (mdl)->byte_count = (ULONG)(length);                                                                      \
    }

#define MmGetMdlByteOffset(mdl) ((mdl)->byte_offset)
#define MmGetMdlBaseVa(mdl) ((mdl)->start_va)
#define MmGetMdlVirtualAddress(mdl) ((void*)((PCHAR)((mdl)->start_va) + (mdl)->byte_offset))

    USERSIM_API
    void
    MmFreePagesFromMdl(_Inout_ PMDL memory_descriptor);

    typedef LARGE_INTEGER PHYSICAL_ADDRESS;

    typedef enum _MEMORY_CACHING_TYPE
    {
        MmNonCached = FALSE,
        MmCached = TRUE,
        MmWriteCombined,
        MmHardwareCoherentCached,
        MmNonCachedUnordered,
        MmUSWCCached,
        MmMaximumCacheType,
        MmNotMapped = -1
    } MEMORY_CACHING_TYPE;

    USERSIM_API
    MDL*
    MmAllocatePagesForMdlEx(
        PHYSICAL_ADDRESS low_address,
        PHYSICAL_ADDRESS high_address,
        PHYSICAL_ADDRESS skip_bytes,
        SIZE_T total_bytes,
        MEMORY_CACHING_TYPE cache_type,
        ULONG flags);

    typedef enum _MODE
    {
        KernelMode,
        UserMode,
        MaximumMode
    } MODE;

    USERSIM_API
    void*
    MmMapLockedPagesSpecifyCache(
        MDL* memory_descriptor_list,
        __drv_strictType(KPROCESSOR_MODE / enum _MODE, __drv_typeConst) KPROCESSOR_MODE access_mode,
        __drv_strictTypeMatch(__drv_typeCond) MEMORY_CACHING_TYPE cache_type,
        _In_opt_ void* requested_address,
        ULONG bug_check_on_failure,
        ULONG priority);

    USERSIM_API
    void
    MmUnmapLockedPages(_In_ void* base_address, _In_ MDL* memory_descriptor_list);

    USERSIM_API
    NTSTATUS
    MmProtectMdlSystemAddress(_In_ MDL* memory_descriptor_list, ULONG new_protect);

#if defined(__cplusplus)
}

// The functions below throw C++ exceptions so tests can catch them to verify error behavior.

void
MmUnmapLockedPagesCPP(_In_ void* base_address, _In_ MDL* memory_descriptor_list);

#endif
