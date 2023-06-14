// Copyright (c) Microsoft Corporation
// SPDX-License-Identifier: MIT

#pragma once
#include "../src/platform.h"

#if defined(__cplusplus)
extern "C"
{
#endif

    NTSTATUS
    RtlULongAdd(
        _In_ unsigned long augend,
        _In_ unsigned long addend,
        _Out_ _Deref_out_range_(==, augend + addend) unsigned long* result);

    NTSTATUS
    RtlCreateAcl(_Out_ PACL Acl, unsigned long AclLength, unsigned long AclRevision);

    VOID
    RtlMapGenericMask(_Inout_ PACCESS_MASK AccessMask, _In_ const GENERIC_MAPPING* GenericMapping);

    unsigned long
    RtlLengthSid(_In_ PSID Sid);

    NTSTATUS
    NTAPI
    RtlAddAccessAllowedAce(
        _Inout_ PACL Acl, _In_ unsigned long AceRevision, _In_ ACCESS_MASK AccessMask, _In_ PSID Sid);

    NTSTATUS
    NTAPI
    RtlCreateSecurityDescriptor(_Out_ PSECURITY_DESCRIPTOR SecurityDescriptor, _In_ unsigned long Revision);

    NTSTATUS
    NTAPI
    RtlSetDaclSecurityDescriptor(
        _Inout_ PSECURITY_DESCRIPTOR SecurityDescriptor,
        _In_ BOOLEAN DaclPresent,
        _In_opt_ PACL Dacl,
        _In_ BOOLEAN DaclDefaulted);

#if defined(__cplusplus)
}
#endif
