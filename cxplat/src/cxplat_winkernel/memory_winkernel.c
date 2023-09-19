// Copyright (c) Microsoft Corporation
// SPDX-License-Identifier: MIT

#include "cxplat.h"

#include <wdm.h>

__forceinline POOL_TYPE
_pool_flags_to_type(cxplat_pool_flags_t pool_flags)
{
    POOL_TYPE pool_type = NonPagedPool;
    if (pool_flags & CXPLAT_POOL_FLAG_PAGED) {
        pool_type |= PagedPool;
    }
    if (pool_flags & CXPLAT_POOL_FLAG_CACHE_ALIGNED) {
        pool_type |= NonPagedPoolCacheAligned;
    }
    if (pool_flags & CXPLAT_POOL_FLAG_NON_PAGED) {
        pool_type |= NonPagedPoolNx;
    }
    return pool_type;
}

__drv_allocatesMem(Mem) _Must_inspect_result_ _Ret_writes_maybenull_(size) void* cxplat_allocate(
    cxplat_pool_flags_t pool_flags, size_t size, uint32_t tag)
{
    // ExAllocatePool2 would be perfect here, but it doesn't exist prior to Windows 10 version 2004
    // or Windows Server 2022.  So we need to wrap ExAllocatePoolUninitialized instead by converting
    // pool flags to pool type.
    POOL_TYPE pool_type = _pool_flags_to_type(pool_flags);
#pragma warning(suppress: 4996) // ExAllocatePoolUninitialized is deprecated, use ExAllocatePool2
    void* memory = ExAllocatePoolUninitialized(pool_type, size, tag);
    if (memory && !(pool_flags & CXPLAT_POOL_FLAG_UNINITIALIZED)) {
        RtlZeroMemory(memory, size);
    }
    return memory;
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
