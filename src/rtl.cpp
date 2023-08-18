// Copyright (c) Microsoft Corporation
// SPDX-License-Identifier: MIT

#include "cxplat_fault_injection.h"
#include "platform.h"
#include "kernel_um.h"
#include "usersim/ke.h"
#include "usersim/rtl.h"
#include "utilities.h"
#include <intsafe.h>
#include <random>
#include <windows.h>

// Rtl* functions.

NTSTATUS
RtlULongAdd(
    _In_ unsigned long augend,
    _In_ unsigned long addend,
    _Out_ _Deref_out_range_(==, augend + addend) unsigned long* result)
{
    // Skip Fault Injection.
    return SUCCEEDED(ULongAdd(augend, addend, result)) ? STATUS_SUCCESS : STATUS_INTEGER_OVERFLOW;
}

NTSTATUS
RtlCreateAcl(_Out_ PACL Acl, unsigned long AclLength, unsigned long AclRevision)
{
    // Skip Fault Injection.
    UNREFERENCED_PARAMETER(Acl);
    UNREFERENCED_PARAMETER(AclRevision);

    if (AclLength < sizeof(ACL)) {
        return STATUS_BUFFER_TOO_SMALL;
    }

    memset(Acl, 0, AclLength);

    return STATUS_SUCCESS;
}

VOID
RtlMapGenericMask(_Inout_ PACCESS_MASK AccessMask, _In_ const GENERIC_MAPPING* GenericMapping)
{
    UNREFERENCED_PARAMETER(AccessMask);
    UNREFERENCED_PARAMETER(GenericMapping);
}

unsigned long
RtlLengthSid(_In_ PSID sid)
{
    SID* sid_structure = (SID*)sid;
    return FIELD_OFFSET(SID, SubAuthority[sid_structure->SubAuthorityCount]);
}

BOOLEAN
RtlValidSid(_In_ PSID sid)
{
    return (sid != nullptr);
}

NTSTATUS
RtlAddAccessAllowedAce(_Inout_ PACL Acl, _In_ unsigned long AceRevision, _In_ ACCESS_MASK AccessMask, _In_ PSID Sid)
{
    if (cxplat_fault_injection_inject_fault()) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    UNREFERENCED_PARAMETER(Acl);
    UNREFERENCED_PARAMETER(AceRevision);
    UNREFERENCED_PARAMETER(AccessMask);
    UNREFERENCED_PARAMETER(Sid);

    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
RtlSetDaclSecurityDescriptor(
    _Inout_ PSECURITY_DESCRIPTOR SecurityDescriptor,
    _In_ BOOLEAN DaclPresent,
    _In_opt_ PACL Dacl,
    _In_ BOOLEAN DaclDefaulted)
{
    // Skip Fault Injection.
    UNREFERENCED_PARAMETER(SecurityDescriptor);
    UNREFERENCED_PARAMETER(DaclPresent);
    UNREFERENCED_PARAMETER(Dacl);
    UNREFERENCED_PARAMETER(DaclDefaulted);

    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
RtlCreateSecurityDescriptor(_Out_ PSECURITY_DESCRIPTOR SecurityDescriptor, _In_ unsigned long Revision)
{
    // Skip Fault Injection.
    UNREFERENCED_PARAMETER(Revision);
    memset(SecurityDescriptor, 0, sizeof(SECURITY_DESCRIPTOR));

    return STATUS_SUCCESS;
}

_Must_inspect_result_ NTSTATUS
RtlSizeTMult(size_t multiplicand, size_t multiplier, _Out_ size_t* result)
{
    return SUCCEEDED(SizeTMult(multiplicand, multiplier, result)) ? STATUS_SUCCESS : STATUS_INTEGER_OVERFLOW;
}

_Must_inspect_result_ NTSTATUS
RtlSizeTAdd(size_t augend, size_t addend, _Out_ _Deref_out_range_(==, augend + addend) size_t* result)
{
    return SUCCEEDED(SizeTAdd(augend, addend, result)) ? STATUS_SUCCESS : STATUS_INTEGER_OVERFLOW;
}

_Must_inspect_result_ NTSTATUS
RtlSizeTSub(
    size_t minuend, size_t subtrahend, _Out_ _Deref_out_range_(==, minuend - subtrahend) size_t* result)
{
    return SUCCEEDED(SizeTSub(minuend, subtrahend, result)) ? STATUS_SUCCESS : STATUS_INTEGER_OVERFLOW;
}

__analysis_noreturn VOID NTAPI
RtlAssertCPP(
    _In_ PVOID void_failed_assertion,
    _In_ PVOID void_file_name,
    _In_ ULONG line_number,
    _In_opt_ PSTR mutable_message)
{
    UNREFERENCED_PARAMETER(void_failed_assertion);
    UNREFERENCED_PARAMETER(void_file_name);
    UNREFERENCED_PARAMETER(mutable_message);
    KeBugCheckEx(0, line_number, NULL, NULL, NULL);
}

__analysis_noreturn VOID NTAPI
RtlAssert(
    _In_ PVOID void_failed_assertion,
    _In_ PVOID void_file_name,
    _In_ ULONG line_number,
    _In_opt_ PSTR mutable_message)
{
    RtlAssertCPP(void_failed_assertion, void_file_name, line_number, mutable_message);
}