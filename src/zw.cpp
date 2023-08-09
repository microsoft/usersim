// Copyright (c) Microsoft Corporation
// SPDX-License-Identifier: MIT
//
// This file contains Usersim overrides of some Zw* APIs
// exposed by ntdll.dll, where the behavior of such APIs
// differs between kernel mode and unprivileged user
// mode, so that we can emulate kernel mode behavior to
// unprivileged user mode test processes.
#include "fault_injection.h"
#include "usersim/zw.h"
#include "utilities.h"
#include <string>

_IRQL_requires_max_(PASSIVE_LEVEL) NTSTATUS ZwCreateKey(
    _Out_ PHANDLE key_handle,
    _In_ ACCESS_MASK desired_access,
    _In_ POBJECT_ATTRIBUTES object_attributes,
    _Reserved_ ULONG title_index,
    _In_opt_ PUNICODE_STRING class_string,
    _In_ ULONG create_options,
    _Out_opt_ PULONG disposition)
{
    if (usersim_fault_injection_inject_fault()) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    if (class_string != nullptr || title_index != 0) {
        return STATUS_NOT_SUPPORTED;
    }

    UNREFERENCED_PARAMETER(create_options);

    HKEY root_key = (HKEY)object_attributes->RootDirectory;
    std::wstring relative_path = object_attributes->ObjectName->Buffer;
    PCWSTR hklm_path = L"\\Registry\\Machine\\";
    PCWSTR hkcu_path = L"\\Registry\\User\\";
    if (relative_path.starts_with(hklm_path)) {
        root_key = HKEY_LOCAL_MACHINE;
        relative_path = relative_path.substr(wcslen(hklm_path));
    } else if (relative_path.starts_with(hkcu_path)) {
        root_key = HKEY_CURRENT_USER;
        relative_path = relative_path.substr(wcslen(hkcu_path));
    }

    ULONG result = RegCreateKeyExW(
        root_key,
        relative_path.c_str(),
        0, // Reserved
        (class_string ? class_string->Buffer : nullptr),
        REG_OPTION_VOLATILE,
        desired_access,
        nullptr,
        (PHKEY)key_handle,
        disposition);
    if (result == ERROR_ACCESS_DENIED && root_key == HKEY_LOCAL_MACHINE) {
        // Try again with HKCU, so access to \Registry\Machine will succeed
        // like it should when called from "kernel-mode" code.
        //
        // Note that HKLM will fail with access denied, even when the desired
        // access is only read access, unless the process is running as admin,
        // since we use RegCreateKey instead of RegOpenKey.  This ensures that
        // the same registry key will be accessed across multiple calls
        // regardless of desired access.

        root_key = HKEY_CURRENT_USER;
        result = RegCreateKeyExW(
            root_key,
            relative_path.c_str(),
            0, // Reserved
            (class_string ? class_string->Buffer : nullptr),
            REG_OPTION_VOLATILE,
            desired_access,
            nullptr,
            (PHKEY)key_handle,
            disposition);
    }

    return win32_error_code_to_usersim_result(result);
}

NTSTATUS
ZwDeleteKey(_In_ HANDLE key_handle)
{
    ULONG result = RegDeleteKey((HKEY)key_handle, L"");
    return win32_error_code_to_usersim_result(result);
}