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

#define CXPLAT_DEFAULT_TAG 'lpxc'

typedef struct
{
    cxplat_pool_type_t pool_type;
    uint32_t tag;
} cxplat_allocation_header_t;

static inline cxplat_allocation_header_t*
_header_from_pointer(const void* memory)
{
    return (cxplat_allocation_header_t*)((uint8_t*)memory - sizeof(cxplat_allocation_header_t));
}

static inline uint8_t*
_memory_block_from_aligned_pointer(const void* pointer)
{
    return ((uint8_t*)pointer) - CXPLAT_CACHE_LINE_SIZE;
}

static inline uint8_t*
_memory_block_from_unaligned_pointer(const void* pointer)
{
    return ((uint8_t*)pointer) - sizeof(cxplat_allocation_header_t);
}

static inline uint8_t*
_aligned_pointer_from_memory_block(const void* memory)
{
    return ((uint8_t*)memory) + CXPLAT_CACHE_LINE_SIZE;
}

static inline uint8_t*
_unaligned_pointer_from_memory_block(const void* memory)
{
    return ((uint8_t*)memory) + sizeof(cxplat_allocation_header_t);
}

__drv_allocatesMem(Mem) _Must_inspect_result_ _Ret_writes_maybenull_(size) void* cxplat_allocate_with_tag(
    _In_ cxplat_pool_type_t pool_type, size_t size, uint32_t tag, bool initialize)
{
    CXPLAT_RUNTIME_ASSERT(size > 0);
    if (size > cxplat_fuzzing_memory_limit) {
        return nullptr;
    }

    if (cxplat_fault_injection_inject_fault()) {
        return nullptr;
    }

    // Allocate space with a cxplat_allocation_header_t prepended.
    void* memory;
    if (pool_type == CxPlatNonPagedPoolNxCacheAligned) {
        // The pointer we return has to be cache aligned so we allocate
        // enough extra space to fill a cache line, and put the
        // cxplat_allocation_header_t at the end of that space.
        // TODO: move logic into cxplat_allocate_cache_aligned_with_tag().
        size_t full_size = CXPLAT_CACHE_LINE_SIZE + size;
        uint8_t* pointer = (uint8_t*)_aligned_malloc(full_size, CXPLAT_CACHE_LINE_SIZE);
        if (pointer == nullptr) {
            return nullptr;
        }
        memory = _aligned_pointer_from_memory_block(pointer);
    } else {
        size_t full_size = sizeof(cxplat_allocation_header_t) + size;
        uint8_t* pointer = (uint8_t*)calloc(full_size, 1);
        if (pointer == nullptr) {
            return nullptr;
        }
        memory = _unaligned_pointer_from_memory_block(pointer);
    }

    // Do any initialization.
    auto header = (cxplat_allocation_header_t*)((uint8_t*)memory - sizeof(cxplat_allocation_header_t));
    header->pool_type = pool_type;
    header->tag = tag;
    if (!initialize) {
        // The calloc call always zero-initializes memory.  To test
        // returning uninitialized memory, we explicitly fill it with 0xcc.
        memset(memory, 0xcc, size);
    }

    if (memory && _cxplat_leak_detector_ptr) {
        _cxplat_leak_detector_ptr->register_allocation(reinterpret_cast<uintptr_t>(memory), size);
    }

    return memory;
}

__drv_allocatesMem(Mem) _Must_inspect_result_ _Ret_writes_maybenull_(size) void* cxplat_allocate(size_t size)
{
    return cxplat_allocate_with_tag(CxPlatNonPagedPoolNx, size, CXPLAT_DEFAULT_TAG, true);
}

__drv_allocatesMem(Mem) _Must_inspect_result_ _Ret_writes_maybenull_(new_size) void* cxplat_reallocate_with_tag(
    _In_ _Post_invalid_ void* pointer, size_t old_size, size_t new_size, uint32_t tag)
{
    UNREFERENCED_PARAMETER(tag);
    UNREFERENCED_PARAMETER(old_size);

    if (new_size > cxplat_fuzzing_memory_limit) {
        return nullptr;
    }

    if (cxplat_fault_injection_inject_fault()) {
        return nullptr;
    }

    cxplat_allocation_header_t* header = _header_from_pointer(pointer);
    void* p;
    if (header->pool_type == CxPlatNonPagedPoolNxCacheAligned) {
        uint8_t* old_memory_block = _memory_block_from_aligned_pointer(pointer);
        void* new_memory_block = _aligned_realloc(old_memory_block, new_size, CXPLAT_CACHE_LINE_SIZE);
        p = (new_memory_block) ? _aligned_pointer_from_memory_block(new_memory_block) : nullptr;
    } else {
        uint8_t* old_memory_block = _memory_block_from_unaligned_pointer(pointer);
        size_t full_size = sizeof(cxplat_allocation_header_t) + new_size;
        void* new_memory_block = realloc(old_memory_block, new_size);
        p = (new_memory_block) ? _unaligned_pointer_from_memory_block(new_memory_block) : nullptr;
    }

    if (_cxplat_leak_detector_ptr) {
        _cxplat_leak_detector_ptr->unregister_allocation(reinterpret_cast<uintptr_t>(pointer));
    }
    if (p) {
        if (new_size > old_size) {
            memset(((char*)p) + old_size, 0, new_size - old_size);
        }

        if (_cxplat_leak_detector_ptr) {
            _cxplat_leak_detector_ptr->register_allocation(reinterpret_cast<uintptr_t>(p), new_size);
        }
    }

    return p;
}

__drv_allocatesMem(Mem) _Must_inspect_result_ _Ret_writes_maybenull_(new_size) void* cxplat_reallocate(
    _In_ _Post_invalid_ void* memory, size_t old_size, size_t new_size)
{
    return cxplat_reallocate_with_tag(memory, old_size, new_size, CXPLAT_DEFAULT_TAG);
}

void
cxplat_free(_Frees_ptr_opt_ void* pointer)
{
    if (pointer == nullptr)
    {
        return;
    }
    cxplat_allocation_header_t* header = _header_from_pointer(pointer);
    if (header->pool_type == CxPlatNonPagedPoolNxCacheAligned)
    {
        uint8_t* memory_block = _memory_block_from_aligned_pointer(pointer);
        _aligned_free(memory_block);
    }
    else
    {
        uint8_t* memory_block = _memory_block_from_unaligned_pointer(pointer);
        free(memory_block);
    }
    if (_cxplat_leak_detector_ptr) {
        _cxplat_leak_detector_ptr->unregister_allocation(reinterpret_cast<uintptr_t>(pointer));
    }
}

__drv_allocatesMem(Mem) _Must_inspect_result_
_Ret_writes_maybenull_(size) void*
cxplat_allocate_cache_aligned(size_t size)
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
