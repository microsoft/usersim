// Copyright (c) Microsoft Corporation
// SPDX-License-Identifier: MIT
#pragma once
#include "usersim/rtl.h" // For UNICODE_STRING

CXPLAT_EXTERN_C_BEGIN

USERSIM_API _IRQL_requires_max_(PASSIVE_LEVEL) NTSTATUS ZwCreateKey(
    _Out_ PHANDLE key_handle,
    _In_ ACCESS_MASK desired_access,
    _In_ POBJECT_ATTRIBUTES object_attributes,
    _Reserved_ ULONG title_index,
    _In_opt_ PUNICODE_STRING class_string,
    _In_ ULONG create_options,
    _Out_opt_ PULONG disposition);

// The following APIs are exported by ntdll.dll but the prototypes
// are not in system headers, so define them here.

NTSTATUS
ZwDeleteKey(_In_ HANDLE key_handle);

NTSTATUS
ZwClose(_In_ HANDLE handle);

CXPLAT_EXTERN_C_END
