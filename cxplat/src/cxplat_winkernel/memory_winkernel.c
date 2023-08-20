// Copyright (c) Microsoft Corporation
// SPDX-License-Identifier: MIT

#include "cxplat.h"
#include <wdm.h>

__drv_allocatesMem(Mem) _Must_inspect_result_ _Ret_writes_maybenull_(size) void* cxplat_allocate_with_tag(
    _In_ cxplat_pool_type_t pool_type, size_t size, uint32_t tag, bool initialize)
{
    if (initialize) {
#pragma warning(suppress : 4996)
        return ExAllocatePoolWithTag(pool_type, size, tag);
    } else {
        return ExAllocatePoolUninitialized(pool_type, size, tag);
    }
}

__drv_allocatesMem(Mem) _Must_inspect_result_ _Ret_writes_maybenull_(new_size) void* cxplat_reallocate_with_tag(
    _In_ _Post_invalid_ void* pointer, size_t old_size, size_t new_size, uint32_t tag)
{
    void* new_pointer = cxplat_allocate_with_tag(CxPlatNonPagedPoolNx, new_size, tag, true);
    if (!new_pointer) {
        return NULL;
    }
    memcpy(new_pointer, pointer, min(old_size, new_size));
    cxplat_free(pointer);
    return new_pointer;
}

void
cxplat_free(_Frees_ptr_opt_ void* pointer)
{
    if (pointer != NULL)
    {
        ExFreePool(pointer);
    }
}

__drv_allocatesMem(Mem) _Must_inspect_result_ _Ret_writes_maybenull_(size) void*
cxplat_allocate_cache_aligned_with_tag(size_t size, uint32_t tag)
{
    return cxplat_allocate_with_tag(CxPlatNonPagedPoolNxCacheAligned, size, tag, false);
}

void
cxplat_free_cache_aligned(_Frees_ptr_opt_ void* memory)
{
    cxplat_free(memory);
}
