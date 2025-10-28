// Copyright (c) Microsoft Corporation
// SPDX-License-Identifier: MIT
#include "../tags.h"
#include "cxplat.h"
#include "wdm.h"

#include <aux_klib.h>

static cxplat_status_t
cxplat_convert_ansi_to_utf8_string(_In_ ANSI_STRING* ansi_string, _Out_ cxplat_utf8_string_t* utf8)
{
    NTSTATUS status;
    cxplat_status_t cxplat_status;
    UNICODE_STRING unicode_str = {0};
    UTF8_STRING utf8_str = {0};

    // First convert the ANSI string to a Unicode string.
    status = RtlAnsiStringToUnicodeString(&unicode_str, ansi_string, TRUE);
    if (!NT_SUCCESS(status)) {
        cxplat_status = CXPLAT_STATUS_NO_MEMORY;
        goto Exit;
    }

    // Convert the unicode string to a UTF-8 string.
    status = RtlUnicodeStringToUTF8String(&utf8_str, &unicode_str, TRUE);
    if (!NT_SUCCESS(status)) {
        cxplat_status = CXPLAT_STATUS_NO_MEMORY;
        goto Exit;
    }

    // Allocate the UTF-8 string large enough to hold the converted string and a null terminator.
    utf8->length = utf8_str.Length + 1; // +1 for null terminator
    utf8->value = cxplat_allocate(CXPLAT_POOL_FLAG_NON_PAGED, utf8->length, CXPLAT_TAG_UTF8_STRING);
    if (utf8->value == NULL) {
        cxplat_status = CXPLAT_STATUS_NO_MEMORY;
        goto Exit;
    }

    // Copy the converted string to the output buffer.
    RtlCopyMemory(utf8->value, utf8_str.Buffer, utf8_str.Length);
    utf8->value[utf8_str.Length] = '\0'; // Null-terminate the string

    cxplat_status = CXPLAT_STATUS_SUCCESS;

Exit:
    if (unicode_str.Buffer) {
        RtlFreeUnicodeString(&unicode_str);
    }

    if (utf8_str.Buffer) {
        RtlFreeUTF8String(&utf8_str);
    }

    return cxplat_status;
}

cxplat_status_t
cxplat_get_module_path_from_address(_In_ const void* address, _Out_ cxplat_utf8_string_t* path)
{
    NTSTATUS status;
    cxplat_status_t cxplat_status;
    ULONG buffer_size = 0;
    AUX_MODULE_EXTENDED_INFO* module_info = NULL;

    status = AuxKlibInitialize();
    if (!NT_SUCCESS(status)) {
        cxplat_status = CXPLAT_STATUS_NO_MEMORY;
        goto Exit;
    }

    status = AuxKlibQueryModuleInformation(&buffer_size, sizeof(AUX_MODULE_EXTENDED_INFO), NULL);

    if (buffer_size == 0) {
        cxplat_status = CXPLAT_STATUS_NOT_FOUND;
        goto Exit;
    }

    module_info =
        (AUX_MODULE_EXTENDED_INFO*)cxplat_allocate(CXPLAT_POOL_FLAG_PAGED, buffer_size, CXPLAT_TAG_MODULE_INFO);
    if (module_info == NULL) {
        cxplat_status = CXPLAT_STATUS_NO_MEMORY;
        goto Exit;
    }

    status = AuxKlibQueryModuleInformation(&buffer_size, sizeof(AUX_MODULE_EXTENDED_INFO), module_info);

    if (!NT_SUCCESS(status)) {
        cxplat_status = CXPLAT_STATUS_NOT_FOUND;
        goto Exit;
    }

    for (ULONG i = 0; i < buffer_size / sizeof(AUX_MODULE_EXTENDED_INFO); i++) {
        uintptr_t module_base = (uintptr_t)module_info[i].BasicInfo.ImageBase;
        uintptr_t module_end = module_base + module_info[i].ImageSize;
        // Found the module that contains the address.
        if ((uintptr_t)address >= module_base && (uintptr_t)address < module_end) {
            ANSI_STRING ansi_path;
            ansi_path.Buffer = (PCHAR)module_info[i].FullPathName;
            ansi_path.Length = (USHORT)strlen((const char*)module_info[i].FullPathName);
            ansi_path.MaximumLength = sizeof(module_info[i].FullPathName);
            cxplat_status = cxplat_convert_ansi_to_utf8_string(&ansi_path, path);

            goto Exit;
        }
    }
    cxplat_status = CXPLAT_STATUS_NOT_FOUND;

Exit:
    if (module_info) {
        cxplat_free(module_info, CXPLAT_POOL_FLAG_PAGED, CXPLAT_TAG_MODULE_INFO);
    }
    return cxplat_status;
}
