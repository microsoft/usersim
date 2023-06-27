// Copyright (c) Microsoft Corporation
// SPDX-License-Identifier: MIT

#include "fault_injection.h"
#include "kernel_um.h"
#include "platform.h"
#include "tracelog.h"
#include "usersim/ex.h"
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
    _In_ unsigned long page_priority) // MM_PAGE_PRIORITY logically OR'd with MdlMapping*
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

MDL*
MmAllocatePagesForMdlEx(
    PHYSICAL_ADDRESS low_address,
    PHYSICAL_ADDRESS high_address,
    PHYSICAL_ADDRESS skip_bytes,
    SIZE_T total_bytes,
    MEMORY_CACHING_TYPE cache_type,
    ULONG flags)
{
    UNREFERENCED_PARAMETER(low_address);
    UNREFERENCED_PARAMETER(high_address);
    UNREFERENCED_PARAMETER(skip_bytes);
    UNREFERENCED_PARAMETER(cache_type);
    UNREFERENCED_PARAMETER(flags);

    if (total_bytes > ULONG_MAX) {
        return nullptr;
    }

    // Skip fault injection for this VirtualAlloc OS API, as ebpf_allocate already does that.
    MDL* descriptor = (MDL*)ExAllocatePoolUninitialized(NonPagedPool, sizeof(MDL), 'PAmM');
    if (!descriptor) {
        return nullptr;
    }
    memset(descriptor, 0, sizeof(descriptor));

    descriptor->start_va = VirtualAlloc(0, total_bytes, MEM_COMMIT, PAGE_EXECUTE_READWRITE);
    descriptor->byte_count = (ULONG)total_bytes;

    if (!descriptor->start_va) {
        USERSIM_LOG_WIN32_API_FAILURE(USERSIM_TRACELOG_KEYWORD_BASE, VirtualAlloc);
        ExFreePool(descriptor);
        descriptor = nullptr;
    }
    return descriptor;
}

void
MmFreePagesFromMdl(_Inout_ PMDL memory_descriptor)
{
    if (!VirtualFree(memory_descriptor->start_va, 0, MEM_RELEASE)) {
        USERSIM_LOG_WIN32_API_FAILURE(USERSIM_TRACELOG_KEYWORD_BASE, VirtualFree);
    } else {
        memory_descriptor->start_va = nullptr;
        memory_descriptor->byte_count = 0;
    }
}

void*
MmMapLockedPagesSpecifyCache(
    MDL* memory_descriptor_list,
    __drv_strictType(KPROCESSOR_MODE / enum _MODE, __drv_typeConst) KPROCESSOR_MODE access_mode,
    __drv_strictTypeMatch(__drv_typeCond) MEMORY_CACHING_TYPE cache_type,
    _In_opt_ void* requested_address,
    ULONG bug_check_on_failure,
    ULONG priority)
{
    UNREFERENCED_PARAMETER(access_mode);
    UNREFERENCED_PARAMETER(cache_type);
    UNREFERENCED_PARAMETER(requested_address);
    UNREFERENCED_PARAMETER(bug_check_on_failure);
    UNREFERENCED_PARAMETER(priority);
    return memory_descriptor_list->start_va;
}

void
MmUnmapLockedPages(_In_ void* base_address, _In_ MDL* memory_descriptor_list)
{
    UNREFERENCED_PARAMETER(base_address);
    UNREFERENCED_PARAMETER(memory_descriptor_list);
}

NTSTATUS
MmProtectMdlSystemAddress(_In_ MDL* memory_descriptor_list, ULONG new_protect)
{
    DWORD old_protect;
    if (!VirtualProtect(
            memory_descriptor_list->start_va,
            memory_descriptor_list->byte_count,
            new_protect,
            &old_protect)) {
        USERSIM_LOG_WIN32_API_FAILURE(USERSIM_TRACELOG_KEYWORD_BASE, VirtualProtect);
        USERSIM_RETURN_RESULT(STATUS_INVALID_PARAMETER);
    }
    USERSIM_RETURN_RESULT(STATUS_SUCCESS);
}