// Copyright (c) Microsoft Corporation
// SPDX-License-Identifier: MIT
#pragma once
#include "usersim/ke.h"

CXPLAT_EXTERN_C_BEGIN

_IRQL_requires_max_(DISPATCH_LEVEL) USERSIM_API LONG_PTR ObfReferenceObject(_In_ PVOID object);

_IRQL_requires_max_(DISPATCH_LEVEL) USERSIM_API LONG_PTR ObfDereferenceObject(_In_ PVOID object);

typedef struct _OBJECT_TYPE* POBJECT_TYPE;

USERSIM_API extern POBJECT_TYPE* ExEventObjectType;

typedef struct _OBJECT_HANDLE_INFORMATION
{
    ULONG HandleAttributes;
    ACCESS_MASK GrantedAccess;
} OBJECT_HANDLE_INFORMATION, *POBJECT_HANDLE_INFORMATION;

_IRQL_requires_max_(PASSIVE_LEVEL) USERSIM_API NTSTATUS ObReferenceObjectByHandle(
    _In_ HANDLE handle,
    _In_ ACCESS_MASK desired_access,
    _In_opt_ POBJECT_TYPE object_type,
    _In_ KPROCESSOR_MODE access_mode,
    _Out_ PVOID* object,
    _Out_opt_ POBJECT_HANDLE_INFORMATION handle_information);

USERSIM_API
NTSTATUS
ObCloseHandle(_In_ _Post_ptr_invalid_ HANDLE handle, _In_ KPROCESSOR_MODE previous_mode);

CXPLAT_EXTERN_C_END
