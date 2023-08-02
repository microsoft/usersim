// Copyright (c) Microsoft Corporation
// SPDX-License-Identifier: MIT

#include "platform.h"
#include "kernel_um.h"
#include "usersim/ob.h"
#include <map>

static std::map<PVOID, ULONG> _object_references;

_IRQL_requires_max_(DISPATCH_LEVEL) USERSIM_API LONG_PTR
ObfReferenceObject(_In_ PVOID object)
{
    if (!_object_references.contains(object)) {
        _object_references[object] = 1;
    } else {
        _object_references[object]++;
    }
    return _object_references[object];
}

_IRQL_requires_max_(DISPATCH_LEVEL) USERSIM_API LONG_PTR
ObfDereferenceObject(_In_ PVOID object)
{
    ULONG remaining = --_object_references[object];
    if (remaining == 0) {
        _object_references.erase(object);
        CloseHandle(object);
    }
    return remaining;
}

_IRQL_requires_max_(PASSIVE_LEVEL) USERSIM_API NTSTATUS
ObReferenceObjectByHandle(
    _In_ HANDLE handle,
    _In_ ACCESS_MASK desired_access,
    _In_opt_ POBJECT_TYPE object_type,
    _In_ KPROCESSOR_MODE access_mode,
    _Out_ PVOID* object,
    _Out_opt_ POBJECT_HANDLE_INFORMATION handle_information)
{
    UNREFERENCED_PARAMETER(desired_access);
    UNREFERENCED_PARAMETER(object_type);
    UNREFERENCED_PARAMETER(access_mode);
    *object = handle;
    if (handle_information != nullptr) {
        return STATUS_NOT_SUPPORTED;
    }
    ObfReferenceObject(*object);
    return STATUS_SUCCESS;
}

USERSIM_API
NTSTATUS
ObCloseHandle(_In_ _Post_ptr_invalid_ HANDLE handle, _In_ KPROCESSOR_MODE previous_mode)
{
    UNREFERENCED_PARAMETER(previous_mode);
    ObfDereferenceObject(handle);
    return STATUS_SUCCESS;
}