// Copyright (c) Microsoft Corporation
// SPDX-License-Identifier: MIT

#pragma once
#include <stdint.h>

#if defined(__cplusplus)
extern "C"
{
#endif

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

    void
    MmBuildMdlForNonPagedPool(_Inout_ MDL* memory_descriptor_list);

    unsigned long
    MmGetMdlByteCount(_In_ MDL* mdl);

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

#if defined(__cplusplus)
}
#endif
