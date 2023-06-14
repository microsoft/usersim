// Copyright (c) Microsoft Corporation
// SPDX-License-Identifier: MIT

#include "fault_injection.h"
#include "kernel_um.h"
#include "platform.h"
#include "usersim/se.h"

// Se* functions.

static SE_EXPORTS _SeExports = {0};
PSE_EXPORTS SeExports = &_SeExports;

BOOLEAN
SeAccessCheckFromState(
    _In_ PSECURITY_DESCRIPTOR SecurityDescriptor,
    _In_ PTOKEN_ACCESS_INFORMATION PrimaryTokenInformation,
    _In_opt_ PTOKEN_ACCESS_INFORMATION ClientTokenInformation,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ ACCESS_MASK PreviouslyGrantedAccess,
    _Outptr_opt_result_maybenull_ PPRIVILEGE_SET* Privileges,
    _In_ PGENERIC_MAPPING GenericMapping,
    _In_ KPROCESSOR_MODE AccessMode,
    _Out_ PACCESS_MASK GrantedAccess,
    _Out_ NTSTATUS* AccessStatus)
{
    if (Privileges != nullptr) {
        *Privileges = nullptr;
    }
    *GrantedAccess = DesiredAccess;

    if (usersim_fault_injection_inject_fault()) {
        *AccessStatus = STATUS_ACCESS_DENIED;
        return false;
    }

    UNREFERENCED_PARAMETER(SecurityDescriptor);
    UNREFERENCED_PARAMETER(PrimaryTokenInformation);
    UNREFERENCED_PARAMETER(ClientTokenInformation);
    UNREFERENCED_PARAMETER(PreviouslyGrantedAccess);
    UNREFERENCED_PARAMETER(GenericMapping);
    UNREFERENCED_PARAMETER(AccessMode);

    *AccessStatus = STATUS_SUCCESS;

    return true;
}