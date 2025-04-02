// Copyright (c) Microsoft Corporation
// SPDX-License-Identifier: MIT
#include "../tags.h"
#include "cxplat.h"
#include "wdm.h"

#include <aux_klib.h>

cxplat_status_t
cxplat_get_module_path_from_address(
    _In_ const void* address,
    _Out_writes_z_(path_length) char* path,
    _In_ size_t path_length,
    _Out_opt_ size_t* path_length_out)
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
            size_t path_length_needed = strlen((const char*)module_info[i].FullPathName) + 1;

            if (path_length < path_length_needed) {
                cxplat_status = CXPLAT_STATUS_NO_MEMORY;
                goto Exit;
            }
            RtlCopyMemory(path, module_info[i].FullPathName, path_length_needed);
            path[path_length_needed - 1] = '\0';
            if (path_length_out) {
                *path_length_out = path_length_needed;
            }
            cxplat_status = CXPLAT_STATUS_SUCCESS;
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
