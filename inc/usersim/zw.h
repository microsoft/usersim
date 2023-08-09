// Copyright (c) Microsoft Corporation
// SPDX-License-Identifier: MIT
#pragma once
#include "usersim/rtl.h" // For UNICODE_STRING

#if defined(__cplusplus)
extern "C"
{
#endif

    USERSIM_API
    _IRQL_requires_max_(PASSIVE_LEVEL) NTSTATUS ZwCreateKey(
        _Out_ PHANDLE key_handle,
        _In_ ACCESS_MASK desired_access,
        _In_ POBJECT_ATTRIBUTES object_attributes,
        _Reserved_ ULONG title_index,
        _In_opt_ PUNICODE_STRING class_string,
        _In_ ULONG create_options,
        _Out_opt_ PULONG disposition);

    USERSIM_API NTSTATUS
    ZwDeleteKey(
        _In_ HANDLE key_handle);

#if defined(__cplusplus)
}
#endif
