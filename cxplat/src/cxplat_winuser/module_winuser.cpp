// Copyright (c) Microsoft Corporation
// SPDX-License-Identifier: MIT
#include "..\tags.h"
#include "cxplat.h"

#include <windows.h>
#include <psapi.h>

#pragma comment(lib, "Mincore.lib")

cxplat_status_t
cxplat_get_module_path_from_address(_In_ const void* address, _Out_ cxplat_utf8_string_t* utf8_path)
{
    HMODULE hModule = NULL;
    wchar_t path[MAX_PATH] = {0};
    size_t path_length = sizeof(path);
    cxplat_status_t status;
    size_t utf8_length = 0;

    if (GetModuleHandleExW(
            GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            (LPCWSTR)address,
            &hModule) == 0) {
        status = CXPLAT_STATUS_NOT_FOUND;
        goto Done;
    }

    // Note: The path buffer may or may not be null-terminated, depending on the length of the module name.
    if (GetModuleFileNameW(hModule, path, (DWORD)path_length) == 0) {
        return CXPLAT_STATUS_NOT_FOUND;
    }

    // Convert the wide string to a UTF-8 string.
    utf8_length = WideCharToMultiByte(CP_UTF8, 0, path, wcslen(path), NULL, 0, NULL, NULL);

    if (utf8_length == 0) {
        status = CXPLAT_STATUS_NO_MEMORY;
        goto Done;
    }

    // Allocate the UTF-8 string large enough to hold the converted string and a null terminator.
    utf8_path->length = utf8_length + 1; // +1 for null terminator
    utf8_path->value = (uint8_t*)cxplat_allocate(CXPLAT_POOL_FLAG_NON_PAGED, utf8_path->length, CXPLAT_TAG_STRING);
    if (utf8_path->value == NULL) {
        status = CXPLAT_STATUS_NO_MEMORY;
        goto Done;
    }

    // Convert the wide string to UTF-8.
    if (WideCharToMultiByte(CP_UTF8, 0, path, wcslen(path), (LPSTR)utf8_path->value, (int)utf8_length, NULL, NULL) ==
        0) {
        status = CXPLAT_STATUS_NO_MEMORY;
        goto Done;
    }

    // Add the null terminator to the end of the string.
    // Note: WideCharToMultiByte does not guarantee null termination, so we add it manually.
    utf8_path->value[utf8_path->length - 1] = '\0';

    status = CXPLAT_STATUS_SUCCESS;

Done:
    if (status != CXPLAT_STATUS_SUCCESS) {
        if (utf8_path->value != NULL) {
            cxplat_free(utf8_path->value, CXPLAT_POOL_FLAG_NON_PAGED, CXPLAT_TAG_STRING);
            utf8_path->value = NULL;
        }
    }

    return status;
}
