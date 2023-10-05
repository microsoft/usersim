// Copyright (c) Microsoft Corporation
// SPDX-License-Identifier: MIT

#include "../tags.h"
#include "cxplat.h"
#include "cxplat_fault_injection.h"
#ifdef CXPLAT_DEBUGGING_FEATURES_ENABLED
#include "leak_detector.h"
#endif
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

#ifdef CXPLAT_DEBUGGING_FEATURES_ENABLED
extern cxplat_leak_detector_ptr _cxplat_leak_detector_ptr;
extern "C" size_t cxplat_fuzzing_memory_limit = MAXSIZE_T;
#endif

#ifndef NDEBUG
typedef struct
{
    cxplat_pool_flags_t pool_flags;
    union
    {
        uint32_t tag;
        char tag_string[4];
    };
    size_t size;
} cxplat_allocation_header_t;

static_assert(sizeof(cxplat_allocation_header_t) <= CXPLAT_CACHE_LINE_SIZE);

static inline cxplat_allocation_header_t*
_header_from_pointer(const void* memory)
{
    return (cxplat_allocation_header_t*)((uint8_t*)memory - sizeof(cxplat_allocation_header_t));
}

#define UNALIGNED_POINTER_OFFSET sizeof(cxplat_allocation_header_t)
#else
#define UNALIGNED_POINTER_OFFSET 0
#endif

#define ALIGNED_POINTER_OFFSET CXPLAT_CACHE_LINE_SIZE

static inline uint8_t*
_memory_block_from_aligned_pointer(const void* pointer)
{
    return ((uint8_t*)pointer) - ALIGNED_POINTER_OFFSET;
}

static inline uint8_t*
_memory_block_from_unaligned_pointer(const void* pointer)
{
    return ((uint8_t*)pointer) - UNALIGNED_POINTER_OFFSET;
}

static inline uint8_t*
_aligned_pointer_from_memory_block(const void* memory)
{
    return ((uint8_t*)memory) + ALIGNED_POINTER_OFFSET;
}

static inline uint8_t*
_unaligned_pointer_from_memory_block(const void* memory)
{
    return ((uint8_t*)memory) + UNALIGNED_POINTER_OFFSET;
}

__drv_allocatesMem(Mem) _Must_inspect_result_ _Ret_writes_maybenull_(size) void* cxplat_allocate(
    cxplat_pool_flags_t pool_flags, size_t size, uint32_t tag)
{
    CXPLAT_RUNTIME_ASSERT(size > 0);
#ifdef CXPLAT_DEBUGGING_FEATURES_ENABLED
    if (size > cxplat_fuzzing_memory_limit) {
        return nullptr;
    }

    if (cxplat_fault_injection_inject_fault()) {
        return nullptr;
    }
#endif

    // Allocate space with a cxplat_allocation_header_t prepended.
    void* memory;
    if (pool_flags & CXPLAT_POOL_FLAG_CACHE_ALIGNED) {
        // The pointer we return has to be cache aligned so we allocate
        // enough extra space to fill a cache line, and put the
        // cxplat_allocation_header_t at the end of that space.
        size_t full_size = ALIGNED_POINTER_OFFSET + size;
        uint8_t* pointer = (uint8_t*)_aligned_malloc(full_size, CXPLAT_CACHE_LINE_SIZE);
        if (pointer == nullptr) {
            return nullptr;
        }
        memory = _aligned_pointer_from_memory_block(pointer);
    } else {
        size_t full_size = UNALIGNED_POINTER_OFFSET + size;
        uint8_t* pointer = (uint8_t*)malloc(full_size);
        if (pointer == nullptr) {
            return nullptr;
        }
        memory = _unaligned_pointer_from_memory_block(pointer);
    }
    if (!(pool_flags & CXPLAT_POOL_FLAG_UNINITIALIZED)) {
        memset(memory, 0, size);
    }
#ifndef NDEBUG
    if (pool_flags & CXPLAT_POOL_FLAG_UNINITIALIZED) {
        // The calloc call always zero-initializes memory.  To test
        // returning uninitialized memory, we explicitly fill it with 0xcc.
        memset(memory, 0xcc, size);
    }

    // Do any initialization.
    auto header = (cxplat_allocation_header_t*)((uint8_t*)memory - sizeof(cxplat_allocation_header_t));
    header->pool_flags = pool_flags;
    header->tag = tag;
    header->size = size;

    if (memory && _cxplat_leak_detector_ptr) {
        _cxplat_leak_detector_ptr->register_allocation(reinterpret_cast<uintptr_t>(memory), size);
    }
#endif

    return memory;
}

__drv_allocatesMem(Mem) _Must_inspect_result_ _Ret_writes_maybenull_(new_size) void* cxplat_reallocate(
    _In_ _Post_invalid_ void* pointer, cxplat_pool_flags_t pool_flags, size_t old_size, size_t new_size, uint32_t tag)
{
#ifdef CXPLAT_DEBUGGING_FEATURES_ENABLED
    if (new_size > cxplat_fuzzing_memory_limit) {
        return nullptr;
    }

    if (cxplat_fault_injection_inject_fault()) {
        return nullptr;
    }
#endif

#ifndef NDEBUG
    cxplat_allocation_header_t* header = _header_from_pointer(pointer);
    CXPLAT_DEBUG_ASSERT(header->size == old_size);
    CXPLAT_DEBUG_ASSERT(!tag || header->tag == tag);
    CXPLAT_DEBUG_ASSERT(header->pool_flags == pool_flags);
#endif
    void* p;
    if (pool_flags & CXPLAT_POOL_FLAG_CACHE_ALIGNED) {
        uint8_t* old_memory_block = _memory_block_from_aligned_pointer(pointer);
        size_t full_size = ALIGNED_POINTER_OFFSET + new_size;
        void* new_memory_block = _aligned_realloc(old_memory_block, full_size, CXPLAT_CACHE_LINE_SIZE);
        p = (new_memory_block) ? _aligned_pointer_from_memory_block(new_memory_block) : nullptr;
    } else {
        uint8_t* old_memory_block = _memory_block_from_unaligned_pointer(pointer);
        size_t full_size = UNALIGNED_POINTER_OFFSET + new_size;
        void* new_memory_block = realloc(old_memory_block, full_size);
        p = (new_memory_block) ? _unaligned_pointer_from_memory_block(new_memory_block) : nullptr;
    }

#ifdef CXPLAT_DEBUGGING_FEATURES_ENABLED
    if (_cxplat_leak_detector_ptr) {
        _cxplat_leak_detector_ptr->unregister_allocation(reinterpret_cast<uintptr_t>(pointer));
    }
#endif
    if (p) {
        if (new_size > old_size) {
            if (!(pool_flags & CXPLAT_POOL_FLAG_UNINITIALIZED)) {
                memset(((char*)p) + old_size, 0, new_size - old_size);
            } else {
#ifndef NDEBUG
                memset(((char*)p) + old_size, 0xcc, new_size - old_size);
#endif
            }
        }

#ifdef CXPLAT_DEBUGGING_FEATURES_ENABLED
        if (_cxplat_leak_detector_ptr) {
            _cxplat_leak_detector_ptr->register_allocation(reinterpret_cast<uintptr_t>(p), new_size);
        }
#endif
    }

    return p;
}

void
cxplat_free(_Frees_ptr_opt_ void* pointer, cxplat_pool_flags_t pool_flags, uint32_t tag)
{
    if (pointer == nullptr) {
        return;
    }
#ifndef NDEBUG
    cxplat_allocation_header_t* header = _header_from_pointer(pointer);
    CXPLAT_DEBUG_ASSERT(!tag || header->tag == tag);
    CXPLAT_DEBUG_ASSERT(header->pool_flags == pool_flags);
#endif
#ifdef CXPLAT_DEBUGGING_FEATURES_ENABLED
    if (_cxplat_leak_detector_ptr) {
        _cxplat_leak_detector_ptr->unregister_allocation(reinterpret_cast<uintptr_t>(pointer));
    }
#endif
    if (pool_flags & CXPLAT_POOL_FLAG_CACHE_ALIGNED) {
        uint8_t* memory_block = _memory_block_from_aligned_pointer(pointer);
        _aligned_free(memory_block);
    } else {
        uint8_t* memory_block = _memory_block_from_unaligned_pointer(pointer);
        free(memory_block);
    }
}

_Must_inspect_result_ cxplat_status_t
cxplat_initialize_lookaside_list(
    _Out_ cxplat_lookaside_list_t* lookaside, _In_ size_t size, _In_ uint32_t tag, _In_ cxplat_pool_flags_t pool_flags)
{

    lookaside->tag = tag;
    lookaside->pool_flags = pool_flags;
    lookaside->size = static_cast<uint32_t>(size);

    return CXPLAT_STATUS_SUCCESS;
}

void
cxplat_uninitialize_lookaside_list(_Inout_ cxplat_lookaside_list_t* lookaside)
{}

_Must_inspect_result_ void*
cxplat_lookaside_list_alloc(_Inout_ cxplat_lookaside_list_t* lookaside)
{
    cxplat_lookaside_list_t* lookaside_list = reinterpret_cast<cxplat_lookaside_list_t*>(lookaside);

    return cxplat_allocate((cxplat_pool_flags_t)lookaside_list->pool_flags, lookaside_list->size, lookaside_list->tag);
}

void
cxplat_lookaside_list_free(_Inout_ cxplat_lookaside_list_t* lookaside, _In_ _Post_invalid_ void* entry)
{
    cxplat_lookaside_list_t* lookaside_list = reinterpret_cast<cxplat_lookaside_list_t*>(lookaside);
    cxplat_free(entry, (cxplat_pool_flags_t)lookaside_list->pool_flags, lookaside_list->tag);
}
