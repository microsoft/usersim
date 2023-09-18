// Copyright (c) Microsoft Corporation
// SPDX-License-Identifier: MIT

#include "cxplat.h"

#include <wdm.h>

__drv_allocatesMem(Mem) _Must_inspect_result_ _Ret_writes_maybenull_(size) void* cxplat_allocate(
    cxplat_pool_flags_t pool_flags, size_t size, uint32_t tag)
{
    return ExAllocatePool2(pool_flags, size, tag);
}

__drv_allocatesMem(Mem) _Must_inspect_result_ _Ret_writes_maybenull_(new_size) void* cxplat_reallocate(
    _In_ _Post_invalid_ void* pointer, cxplat_pool_flags_t pool_flags, size_t old_size, size_t new_size, uint32_t tag)
{
    void* new_pointer = cxplat_allocate(pool_flags, new_size, tag);
    if (!new_pointer) {
        return NULL;
    }
    memcpy(new_pointer, pointer, min(old_size, new_size));
    cxplat_free(pointer, pool_flags, tag);
    return new_pointer;
}

void
cxplat_free(_Frees_ptr_opt_ void* pointer, cxplat_pool_flags_t pool_flags, uint32_t tag)
{
    UNREFERENCED_PARAMETER(pool_flags);
    if (pointer != NULL) {
        ExFreePoolWithTag(pointer, tag);
    }
}