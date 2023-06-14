// Copyright (c) Microsoft Corporation
// SPDX-License-Identifier: MIT

#include "fault_injection.h"
#include "kernel_um.h"
#include "platform.h"
#include "usersim/mm.h"

// Mm* functions.

void
MmBuildMdlForNonPagedPool(_Inout_ MDL* memory_descriptor_list)
{
    UNREFERENCED_PARAMETER(memory_descriptor_list);
}

void*
MmGetSystemAddressForMdlSafe(
    _Inout_ MDL* mdl,
    _In_ unsigned long page_priority // MM_PAGE_PRIORITY logically OR'd with MdlMapping*
)
{
    if (usersim_fault_injection_inject_fault()) {
        return nullptr;
    }

    UNREFERENCED_PARAMETER(page_priority);
    return ((void*)((PUCHAR)(mdl)->start_va + (mdl)->byte_offset));
}

unsigned long
MmGetMdlByteCount(_In_ MDL* mdl)
{
    return mdl->byte_count;
}

#define MmGetMdlByteOffset(mdl) ((mdl)->byte_offset)
#define MmGetMdlBaseVa(mdl) ((mdl)->start_va)
#define MmGetMdlVirtualAddress(mdl) ((void*)((PCHAR)((mdl)->start_va) + (mdl)->byte_offset))