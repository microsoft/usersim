// Copyright (c) Microsoft Corporation
// SPDX-License-Identifier: MIT
#include "cxplat.h"

#include <windows.h>
#include <psapi.h>

#pragma comment(lib, "Mincore.lib")

cxplat_status_t
cxplat_get_module_path_from_address(
    _In_ const void* address,
    _Out_writes_z_(path_length) char* path,
    _In_ size_t path_length,
    _Out_opt_ size_t* path_length_out)
{
    HMODULE hModule = NULL;

    if (GetModuleHandleExA(
            GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            (LPCSTR)address,
            &hModule) == 0) {
        return CXPLAT_STATUS_NOT_FOUND;
    }

    if (GetModuleFileNameA(hModule, path, (DWORD)path_length) == 0) {
        return CXPLAT_STATUS_NOT_FOUND;
    }
    if (path_length_out) {
        *path_length_out = strlen(path) + 1;
    }

    return CXPLAT_STATUS_SUCCESS;
}
