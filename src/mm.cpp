// Copyright (c) Microsoft Corporation
// SPDX-License-Identifier: MIT

#include "cxplat_fault_injection.h"
#include "kernel_um.h"
#include "platform.h"
#include "tracelog.h"
#include "usersim/ex.h"
#include "usersim/ke.h"
#include "usersim/mm.h"

// Mm* functions.

// Internal MDL flags.
#define MDL_FLAG_MAPPED 0x01

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
    if (!(mdl->flags & MDL_FLAG_MAPPED)) {
        if (MmMapLockedPagesSpecifyCache(mdl, KernelMode, MmCached, nullptr, FALSE, page_priority) == nullptr) {
            return nullptr;
        }
    }

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
    memset(descriptor, 0, sizeof(*descriptor));

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
    UNREFERENCED_PARAMETER(cache_type);
    UNREFERENCED_PARAMETER(requested_address);
    UNREFERENCED_PARAMETER(priority);

    if (memory_descriptor_list->flags & MDL_FLAG_MAPPED) {
        // Already mapped.
        KeBugCheck(0);
    }

    if (cxplat_fault_injection_inject_fault()) {
        if (bug_check_on_failure && (access_mode == KernelMode)) {
            KeBugCheck(0);
        }
        return nullptr;
    }

    memory_descriptor_list->flags |= MDL_FLAG_MAPPED;
    return memory_descriptor_list->start_va;
}

void
MmUnmapLockedPagesCPP(_In_ void* base_address, _In_ MDL* memory_descriptor_list)
{
    UNREFERENCED_PARAMETER(base_address);
    if (!(memory_descriptor_list->flags & MDL_FLAG_MAPPED)) {
        KeBugCheckEx(DRIVER_UNMAPPING_INVALID_VIEW, (uintptr_t)memory_descriptor_list, 1, 0, 0);
    }
    memory_descriptor_list->flags &= ~MDL_FLAG_MAPPED;
}

void
MmUnmapLockedPages(_In_ void* base_address, _In_ MDL* memory_descriptor_list)
{
    return MmUnmapLockedPagesCPP(base_address, memory_descriptor_list);
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

void
ProbeForReadCPP(_In_ const volatile void* address, SIZE_T length, ULONG alignment)
{
    if ((((uintptr_t)address) % alignment) != 0) {
        ExRaiseDatatypeMisalignment();
    }
    MEMORY_BASIC_INFORMATION mbi;
    mbi.Protect = 0;
    ::VirtualQuery(((LPCSTR)address) + length - 1, &mbi, sizeof(mbi));
    if ((mbi.Protect & 0xE6) != 0 && (mbi.Protect & PAGE_GUARD) == 0) {
        // Probe is ok.
        return;
    }

    ExRaiseAccessViolation();
}

void
ProbeForRead(_In_ const volatile void* address, SIZE_T length, ULONG alignment)
{
    ProbeForReadCPP(address, length, alignment);
}

void
ProbeForWriteCPP(_Inout_ volatile void* address, SIZE_T length, ULONG alignment)
{
    ProbeForReadCPP(address, length, alignment);
}

void
ProbeForWrite(_Inout_ volatile void* address, SIZE_T length, ULONG alignment)
{
    ProbeForWriteCPP(address, length, alignment);
}