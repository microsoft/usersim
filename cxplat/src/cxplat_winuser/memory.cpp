// Copyright (c) Microsoft Corporation
// SPDX-License-Identifier: MIT

#include "cxplat.h"
#include "leak_detector.h"
#if !defined(UNREFERENCED_PARAMETER)
#define UNREFERENCED_PARAMETER(X) (X)
#endif
#define CXPLAT_CACHE_LINE_SIZE 64
#define CXPLAT_CACHE_ALIGN_POINTER(P) \
    (void*)(((uintptr_t)P + CXPLAT_CACHE_LINE_SIZE - 1) & ~(CXPLAT_CACHE_LINE_SIZE - 1))
#define CXPLAT_PAD_CACHE(X) ((X + CXPLAT_CACHE_LINE_SIZE - 1) & ~(CXPLAT_CACHE_LINE_SIZE - 1))
#define CPLAT_PAD_8(X) ((X + 7) & ~7)

#include <windows.h>
#include <memory.h>
#include <stdint.h>
#include <stdlib.h>

extern cxplat_leak_detector_ptr _cxplat_leak_detector_ptr;

extern "C" size_t cxplat_fuzzing_memory_limit = MAXSIZE_T;

typedef struct
{
    POOL_TYPE pool_type;
    uint32_t tag;
} usersim_allocation_header_t;

__drv_allocatesMem(Mem) _Must_inspect_result_ _Ret_writes_maybenull_(size) void* usersim_allocate_with_tag(
    _In_ __drv_strictTypeMatch(__drv_typeExpr) POOL_TYPE pool_type, size_t size, uint32_t tag, bool initialize)
{
    if (size == 0) {
        KeBugCheckEx(BAD_POOL_CALLER, 0x00, 0, 0, 0);
    }
    if (size > cxplat_fuzzing_memory_limit) {
        return nullptr;
    }

    if (usersim_fault_injection_inject_fault()) {
        return nullptr;
    }

    // Allocate space with a usersim_allocation_header_t prepended.
    void* memory;
    if (pool_type == NonPagedPoolNxCacheAligned) {
        // The pointer we return has to be cache aligned so we allocate
        // enough extra space to fill a cache line, and put the
        // usersim_allocation_header_t at the end of that space.
        // TODO: move logic into usersim_allocate_cache_aligned_with_tag().
        size_t full_size = USERSIM_CACHE_LINE_SIZE + size;
        uint8_t* pointer = (uint8_t*)_aligned_malloc(full_size, USERSIM_CACHE_LINE_SIZE);
        if (pointer == nullptr) {
            return nullptr;
        }
        memory = pointer + USERSIM_CACHE_LINE_SIZE;
    } else {
        size_t full_size = sizeof(usersim_allocation_header_t) + size;
        uint8_t* pointer = (uint8_t*)calloc(full_size, 1);
        if (pointer == nullptr) {
            return nullptr;
        }
        memory = pointer + sizeof(usersim_allocation_header_t);
    }

    // Do any initialization.
    auto header = (usersim_allocation_header_t*)((uint8_t*)memory - sizeof(usersim_allocation_header_t));
    header->pool_type = pool_type;
    header->tag = tag;
    if (!initialize) {
        // The calloc call always zero-initializes memory.  To test
        // returning uninitialized memory, we explicitly fill it with 0xcc.
        memset(memory, 0xcc, size);
    }

    if (memory && _usersim_leak_detector_ptr) {
        _usersim_leak_detector_ptr->register_allocation(reinterpret_cast<uintptr_t>(memory), size);
    }

    return memory;
}


_Must_inspect_result_
_Ret_writes_maybenull_(size) void* cxplat_allocate(size_t size)
{
    return cxplat_allocate_with_tag(size, 'tset', true);
}

_Must_inspect_result_ _Ret_writes_maybenull_(new_size) void* cxplat_reallocate(
    _In_ _Post_invalid_ void* memory, size_t old_size, size_t new_size)
{
    UNREFERENCED_PARAMETER(old_size);
    if (new_size > cxplat_fuzzing_memory_limit) {
        return nullptr;
    }

    if (cxplat_fault_injection_inject_fault()) {
        return nullptr;
    }

    void* p = realloc(memory, new_size);
    if (p && (new_size > old_size)) {
        memset(((char*)p) + old_size, 0, new_size - old_size);
    }

    if (_cxplat_leak_detector_ptr) {
        _cxplat_leak_detector_ptr->unregister_allocation(reinterpret_cast<uintptr_t>(memory));
        _cxplat_leak_detector_ptr->register_allocation(reinterpret_cast<uintptr_t>(p), new_size);
    }

    return p;
}

_Must_inspect_result_ _Ret_writes_maybenull_(new_size) void* cxplat_reallocate_with_tag(
    _In_ _Post_invalid_ void* memory, size_t old_size, size_t new_size, uint32_t tag)
{
    UNREFERENCED_PARAMETER(tag);

    return cxplat_reallocate(memory, old_size, new_size);
}

void
cxplat_free(_Pre_maybenull_ _Post_ptr_invalid_ void* memory)
{
    if (_cxplat_leak_detector_ptr) {
        _cxplat_leak_detector_ptr->unregister_allocation(reinterpret_cast<uintptr_t>(memory));
    }
    free(memory);
}

_Must_inspect_result_
    _Ret_writes_maybenull_(size) void* cxplat_allocate_cache_aligned(size_t size)
{
    if (size > cxplat_fuzzing_memory_limit) {
        return nullptr;
    }

    if (cxplat_fault_injection_inject_fault()) {
        return nullptr;
    }

    void* memory = _aligned_malloc(size, CXPLAT_CACHE_LINE_SIZE);
    if (memory) {
        memset(memory, 0, size);
    }
    return memory;
}

_Must_inspect_result_
    _Ret_writes_maybenull_(size) void* cxplat_allocate_cache_aligned_with_tag(size_t size, uint32_t tag)
{
    UNREFERENCED_PARAMETER(tag);

    return cxplat_allocate_cache_aligned(size);
}

void
cxplat_free_cache_aligned(_Pre_maybenull_ _Post_ptr_invalid_ void* memory)
{
    _aligned_free(memory);
}
