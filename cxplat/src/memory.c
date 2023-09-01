// Copyright (c) Microsoft Corporation
// SPDX-License-Identifier: MIT

#include "cxplat.h"
#include "tags.h"

#include <memory.h>
#include <string.h>

_Must_inspect_result_ _Ret_maybenull_z_ char*
cxplat_duplicate_string(_In_z_ const char* source)
{
    size_t size = strlen(source) + 1;
    char* destination = (char*)cxplat_allocate(CxPlatNonPagedPoolNx, size, CXPLAT_TAG_STRING, true);
    if (destination) {
        memcpy(destination, source, size);
    }
    return destination;
}

void
cxplat_free_string(_Frees_ptr_opt_ _In_z_ const char* source)
{
    cxplat_free((void*)source, CXPLAT_TAG_STRING);
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
            (uint8_t*)cxplat_allocate(CxPlatNonPagedPoolNx, source->length, CXPLAT_TAG_UTF8_STRING, true);
        if (!destination->value) {
            return CXPLAT_STATUS_NO_MEMORY;
        }
        memcpy(destination->value, source->value, source->length);
        destination->length = source->length;
        return CXPLAT_STATUS_SUCCESS;
    }
}

void
cxplat_free_utf8_string(_Inout_ cxplat_utf8_string_t* string)
{
    cxplat_free(string->value, CXPLAT_TAG_UTF8_STRING);
    string->value = NULL;
    string->length = 0;
}
