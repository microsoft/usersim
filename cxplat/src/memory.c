// Copyright (c) Microsoft Corporation
// SPDX-License-Identifier: MIT

#include "cxplat.h"
#include "tags.h"

#include <memory.h>
#include <string.h>

__drv_allocatesMem(Mem) _Must_inspect_result_ _Ret_writes_maybenull_(size) void* cxplat_allocate(size_t size)
{
    return cxplat_allocate_with_tag(CxPlatNonPagedPoolNx, size, CXPLAT_DEFAULT_TAG, true);
}

__drv_allocatesMem(Mem) _Must_inspect_result_ _Ret_writes_maybenull_(new_size) void* cxplat_reallocate(
    _In_ _Post_invalid_ void* memory, size_t old_size, size_t new_size)
{
    return cxplat_reallocate_with_tag(memory, old_size, new_size, CXPLAT_DEFAULT_TAG);
}

__drv_allocatesMem(Mem) _Must_inspect_result_
    _Ret_writes_maybenull_(size) void* cxplat_allocate_cache_aligned(size_t size)
{
    return cxplat_allocate_cache_aligned_with_tag(size, CXPLAT_DEFAULT_TAG);
}

_Must_inspect_result_ _Ret_maybenull_z_ char*
cxplat_duplicate_string(_In_z_ const char* source)
{
    size_t size = strlen(source) + 1;
    char* destination = (char*)cxplat_allocate_with_tag(CxPlatNonPagedPoolNx, size, CXPLAT_STRING_TAG, true);
    if (destination) {
        memcpy(destination, source, size);
    }
    return destination;
}

_Must_inspect_result_ cxplat_status_t
cxplat_duplicate_utf8_string(_Out_ cxplat_utf8_string_t* destination, _In_ const cxplat_utf8_string_t* source)
{
    if (!source->value || !source->length) {
        destination->value = NULL;
        destination->length = 0;
        return CXPLAT_STATUS_SUCCESS;
    } else {
        destination->value =
            (uint8_t*)cxplat_allocate_with_tag(CxPlatNonPagedPoolNx, source->length, CXPLAT_UTF8_STRING_TAG, true);
        if (!destination->value) {
            return CXPLAT_STATUS_NO_MEMORY;
        }
        memcpy(destination->value, source->value, source->length);
        destination->length = source->length;
        return CXPLAT_STATUS_SUCCESS;
    }
}

void
cxplat_utf8_string_free(_Inout_ cxplat_utf8_string_t* string)
{
    cxplat_free(string->value);
    string->value = NULL;
    string->length = 0;
}
