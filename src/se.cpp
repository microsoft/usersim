// Copyright (c) Microsoft Corporation
// SPDX-License-Identifier: MIT

#include "cxplat_fault_injection.h"
#include "kernel_um.h"
#include "platform.h"
#include "usersim/ex.h"
#include "usersim/se.h"
#include "utilities.h"

#include <processthreadsapi.h>

// Se* functions.

static SE_EXPORTS _SeExports = {0};
__declspec(dllexport) PSE_EXPORTS SeExports = &_SeExports;

static void
_initialize_sid(WELL_KNOWN_SID_TYPE type, _Outptr_result_maybenull_ PSID* psid)
{
    *psid = nullptr;

    // Get the SID size.
    DWORD sid_size = 0;
    (void)CreateWellKnownSid(type, nullptr, nullptr, &sid_size);
    DWORD error = GetLastError();
    if (error != ERROR_INSUFFICIENT_BUFFER) {
        return;
    }

    // Get the actual SID.
    PSID actual_sid = malloc(sid_size);
    if (!actual_sid) {
        return;
    }
    if (!CreateWellKnownSid(type, nullptr, actual_sid, &sid_size)) {
        free(actual_sid);
        return;
    }
    *psid = actual_sid;
}

void
usersim_initialize_se()
{
    // Initialize SeExports.
    _initialize_sid(WinNullSid, &_SeExports.SeNullSid);
    _initialize_sid(WinWorldSid, &_SeExports.SeWorldSid);
    _initialize_sid(WinLocalSid, &_SeExports.SeLocalSid);
    _initialize_sid(WinCreatorOwnerSid, &_SeExports.SeCreatorOwnerSid);
    _initialize_sid(WinCreatorGroupSid, &_SeExports.SeCreatorGroupSid);
    _initialize_sid(WinNtAuthoritySid, &_SeExports.SeNtAuthoritySid);
    _initialize_sid(WinDialupSid, &_SeExports.SeDialupSid);
    _initialize_sid(WinNetworkSid, &_SeExports.SeNetworkSid);
    _initialize_sid(WinBatchSid, &_SeExports.SeBatchSid);
    _initialize_sid(WinInteractiveSid, &_SeExports.SeInteractiveSid);
    _initialize_sid(WinLocalSystemSid, &_SeExports.SeLocalSystemSid);
    _initialize_sid(WinBuiltinAdministratorsSid, &_SeExports.SeAliasAdminsSid);
    _initialize_sid(WinBuiltinUsersSid, &_SeExports.SeAliasUsersSid);
    _initialize_sid(WinBuiltinGuestsSid, &_SeExports.SeAliasGuestsSid);
    _initialize_sid(WinBuiltinPowerUsersSid, &_SeExports.SeAliasPowerUsersSid);
    _initialize_sid(WinBuiltinAccountOperatorsSid, &_SeExports.SeAliasAccountOpsSid);
    _initialize_sid(WinBuiltinSystemOperatorsSid, &_SeExports.SeAliasSystemOpsSid);
    _initialize_sid(WinBuiltinPrintOperatorsSid, &_SeExports.SeAliasPrintOpsSid);
    _initialize_sid(WinBuiltinBackupOperatorsSid, &_SeExports.SeAliasBackupOpsSid);
    _initialize_sid(WinAuthenticatedUserSid, &_SeExports.SeAuthenticatedUsersSid);
    _initialize_sid(WinRestrictedCodeSid, &_SeExports.SeRestrictedSid);
    _initialize_sid(WinAnonymousSid, &_SeExports.SeAnonymousLogonSid);
    _initialize_sid(WinLocalServiceSid, &_SeExports.SeLocalServiceSid);
    _initialize_sid(WinNetworkServiceSid, &_SeExports.SeNetworkServiceSid);
    _initialize_sid(WinIUserSid, &_SeExports.SeIUserSid);
    _initialize_sid(WinUntrustedLabelSid, &_SeExports.SeUntrustedMandatorySid);
    _initialize_sid(WinLowLabelSid, &_SeExports.SeLowMandatorySid);
    _initialize_sid(WinMediumLabelSid, &_SeExports.SeMediumMandatorySid);
    _initialize_sid(WinHighLabelSid, &_SeExports.SeHighMandatorySid);
    _initialize_sid(WinSystemLabelSid, &_SeExports.SeSystemMandatorySid);
    _initialize_sid(WinCreatorOwnerRightsSid, &_SeExports.SeOwnerRightsSid);
    _initialize_sid(WinBuiltinAnyPackageSid, &_SeExports.SeAllAppPackagesSid);
    _initialize_sid(WinUserModeDriversSid, &_SeExports.SeUserModeDriversSid);

    // The following don't seem to have WELL_KNOWN_SID_TYPE enum values:
    //    SeProcTrustWinTcbSid
    //    SeTrustedInstallerSid
    //    SeAppSiloSid
    //    SeAppSiloVolumeRootMinimalCapabilitySid
    //    SeAppSiloProfilesRootMinimalCapabilitySid
}

VOID
SeCaptureSubjectContext(_Out_ PSECURITY_SUBJECT_CONTEXT subject_context)
{
    subject_context->lock_count = 0;
    subject_context->process_token = GetCurrentProcessToken();
    subject_context->thread_token = GetCurrentThreadEffectiveToken();
}

VOID
SeLockSubjectContext(_In_ PSECURITY_SUBJECT_CONTEXT subject_context)
{
    InterlockedIncrement(&subject_context->lock_count);
}

VOID
SeUnlockSubjectContext(_In_ PSECURITY_SUBJECT_CONTEXT subject_context)
{
    uint64_t new_value = InterlockedDecrement(&subject_context->lock_count);
    if (new_value == UINT64_MAX) {
        // Underflow.
        KeBugCheck(0);
    }
}

VOID
SeReleaseSubjectContext(_In_ PSECURITY_SUBJECT_CONTEXT subject_context)
{
    UNREFERENCED_PARAMETER(subject_context);
    // This function is a no-op in usersim.
}

_IRQL_requires_max_(PASSIVE_LEVEL) USERSIM_API BOOLEAN SeAccessCheck(
    _In_ PSECURITY_DESCRIPTOR security_descriptor,
    _In_ PSECURITY_SUBJECT_CONTEXT subject_security_context,
    _In_ BOOLEAN subject_context_locked,
    _In_ ACCESS_MASK desired_access,
    _In_ ACCESS_MASK previously_granted_access,
    _Out_opt_ PPRIVILEGE_SET* privileges,
    _In_ PGENERIC_MAPPING generic_mapping,
    _In_ KPROCESSOR_MODE access_mode,
    _Out_ PACCESS_MASK granted_access,
    _Out_ PNTSTATUS access_status)
{
    TOKEN_ACCESS_INFORMATION* token_access_information = nullptr;

    *granted_access = 0;
    if (privileges) {
        *privileges = nullptr;
    }
    if (!subject_context_locked) {
        SeLockSubjectContext(subject_security_context);
    }

    // Get needed buffer size.
    DWORD length = 0;
    BOOLEAN result = !!GetTokenInformation(
        subject_security_context->thread_token, TokenAccessInformation, token_access_information, length, &length);
    int error = GetLastError();
    if (error != ERROR_INSUFFICIENT_BUFFER || length == 0) {
        *access_status = win32_error_to_usersim_error(error);
        goto Done;
    }

    // Allocate buffer.
    token_access_information = (TOKEN_ACCESS_INFORMATION*)cxplat_allocate(
        CXPLAT_POOL_FLAG_NON_PAGED, length, USERSIM_TAG_TOKEN_ACCESS_INFORMATION);
    if (token_access_information == nullptr) {
        *access_status = STATUS_NO_MEMORY;
        goto Done;
    }

    result = SeAccessCheckFromState(
        security_descriptor,
        token_access_information,
        nullptr,
        desired_access,
        previously_granted_access,
        privileges,
        generic_mapping,
        access_mode,
        granted_access,
        access_status);

Done:
    cxplat_free(token_access_information, CXPLAT_POOL_FLAG_NON_PAGED, USERSIM_TAG_TOKEN_ACCESS_INFORMATION);
    if (!subject_context_locked) {
        SeUnlockSubjectContext(subject_security_context);
    }

    return result;
}

BOOLEAN
SeAccessCheckFromState(
    _In_ PSECURITY_DESCRIPTOR security_descriptor,
    _In_ PTOKEN_ACCESS_INFORMATION primary_token_information,
    _In_opt_ PTOKEN_ACCESS_INFORMATION client_token_information,
    _In_ ACCESS_MASK desired_access,
    _In_ ACCESS_MASK previously_granted_access,
    _Outptr_opt_result_maybenull_ PPRIVILEGE_SET* privileges,
    _In_ PGENERIC_MAPPING generic_mapping,
    _In_ KPROCESSOR_MODE access_mode,
    _Out_ PACCESS_MASK granted_access,
    _Out_ NTSTATUS* access_status)
{
    // For now we just grant the requested access, except as part of
    // fault injection.

    if (privileges != nullptr) {
        *privileges = nullptr;
    }
    *granted_access = desired_access;

    if (cxplat_fault_injection_inject_fault()) {
        *access_status = STATUS_ACCESS_DENIED;
        return false;
    }

    UNREFERENCED_PARAMETER(security_descriptor);
    UNREFERENCED_PARAMETER(primary_token_information);
    UNREFERENCED_PARAMETER(client_token_information);
    UNREFERENCED_PARAMETER(previously_granted_access);
    UNREFERENCED_PARAMETER(generic_mapping);
    UNREFERENCED_PARAMETER(access_mode);

    *access_status = STATUS_SUCCESS;

    return true;
}

// Convert a variable-length SID to some 64-bit unique id.
// Currently we just do this with a simple hash.
void
usersim_convert_sid_to_luid(_In_ const PSID psid, _Out_ LUID* luid)
{
    const struct _SID* sid = (const struct _SID*)psid;
    size_t size = FIELD_OFFSET(SID, SubAuthority[sid->SubAuthorityCount]);
    uint64_t hash = 0;
    for (size_t i = 0; i < size; i += sizeof(DWORD)) {
        DWORD value = *(const DWORD*)(((const uint8_t*)psid) + i);
        hash += value;
    }
    *(uint64_t*)luid = hash;
}

NTSTATUS
SeQueryAuthenticationIdToken(_In_ PACCESS_TOKEN token, _Out_ PLUID authentication_id)
{
    HANDLE token_handle = token;
    char token_owner_buffer[TOKEN_OWNER_MAX_SIZE];
    DWORD return_length;

    if (cxplat_fault_injection_inject_fault()) {
        return STATUS_UNSUCCESSFUL;
    }

    if (!GetTokenInformation(
            token_handle, TokenOwner, token_owner_buffer, sizeof(token_owner_buffer), &return_length)) {
        return win32_error_to_usersim_error(GetLastError());
    }

    usersim_convert_sid_to_luid(((PTOKEN_OWNER)token_owner_buffer)->Owner, authentication_id);
    return STATUS_SUCCESS;
}

NTSTATUS
SeQueryInformationToken(
    _In_ PACCESS_TOKEN token, _In_ TOKEN_INFORMATION_CLASS token_information_class, _Out_ PVOID* token_information)
{
    *token_information = nullptr;

    if (cxplat_fault_injection_inject_fault()) {
        return STATUS_UNSUCCESSFUL;
    }

    HANDLE token_handle = (HANDLE)token;
    DWORD needed = 0;

    // Get required buffer size.
    (void)GetTokenInformation(token_handle, token_information_class, nullptr, 0, &needed);
    DWORD error = GetLastError();
    if (error != ERROR_INSUFFICIENT_BUFFER || needed == 0) {
        return win32_error_to_usersim_error(error);
    }

    void* buffer = ExAllocatePoolUninitialized(NonPagedPoolNx, needed, USERSIM_TAG_TOKEN_ACCESS_INFORMATION);
    if (buffer == nullptr) {
        return STATUS_NO_MEMORY;
    }

    if (!GetTokenInformation(token_handle, token_information_class, buffer, needed, &needed)) {
        ExFreePool(buffer);
        return win32_error_to_usersim_error(GetLastError());
    }

    *token_information = buffer;
    return STATUS_SUCCESS;
}

NTSTATUS
SecLookupAccountSid(
    _In_ PSID Sid,
    _Inout_ PULONG NameSize,
    _Out_opt_ PUNICODE_STRING Name,
    _Inout_ PULONG DomainSize,
    _Out_opt_ PUNICODE_STRING Domain,
    _Out_ PSID_NAME_USE SidNameUse)
{
    if (cxplat_fault_injection_inject_fault()) {
        return STATUS_UNSUCCESSFUL;
    }

    if (Sid == nullptr || NameSize == nullptr || DomainSize == nullptr || SidNameUse == nullptr) {
        return STATUS_INVALID_PARAMETER;
    }

    // Use LookupAccountSidW to resolve the SID in user mode.
    DWORD name_chars = 0;
    DWORD domain_chars = 0;
    SID_NAME_USE name_use;

    // First call to get required buffer sizes (in characters including null terminator).
    (void)LookupAccountSidW(nullptr, Sid, nullptr, &name_chars, nullptr, &domain_chars, &name_use);
    DWORD error = GetLastError();
    if (error != ERROR_INSUFFICIENT_BUFFER) {
        return win32_error_to_usersim_error(error);
    }

    // If caller just wants sizes (Name and Domain are NULL), return sizes in bytes.
    if (Name == nullptr && Domain == nullptr) {
        // Return sizes as byte counts (without null terminator) to match kernel SecLookupAccountSid behavior.
        *NameSize = (name_chars > 0) ? (ULONG)((name_chars - 1) * sizeof(WCHAR)) : 0;
        *DomainSize = (domain_chars > 0) ? (ULONG)((domain_chars - 1) * sizeof(WCHAR)) : 0;
        *SidNameUse = name_use;
        return STATUS_BUFFER_TOO_SMALL;
    }

    // Allocate temporary buffers for the LookupAccountSidW call.
    WCHAR* name_buf = nullptr;
    WCHAR* domain_buf = nullptr;

    if (name_chars > 0) {
        name_buf =
            (WCHAR*)ExAllocatePoolUninitialized(NonPagedPoolNx, name_chars * sizeof(WCHAR), USERSIM_TAG_ACCOUNT_NAME);
        if (name_buf == nullptr) {
            return STATUS_NO_MEMORY;
        }
    }
    if (domain_chars > 0) {
        domain_buf =
            (WCHAR*)ExAllocatePoolUninitialized(NonPagedPoolNx, domain_chars * sizeof(WCHAR), USERSIM_TAG_ACCOUNT_NAME);
        if (domain_buf == nullptr) {
            ExFreePool(name_buf);
            return STATUS_NO_MEMORY;
        }
    }

    if (!LookupAccountSidW(nullptr, Sid, name_buf, &name_chars, domain_buf, &domain_chars, &name_use)) {
        ExFreePool(name_buf);
        ExFreePool(domain_buf);
        return win32_error_to_usersim_error(GetLastError());
    }

    // Copy results into caller-provided UNICODE_STRING buffers.
    // The kernel SecLookupAccountSid returns Length in bytes (without null terminator).
    if (Name != nullptr && Name->Buffer != nullptr && name_chars > 0) {
        USHORT byte_len = (USHORT)((name_chars) * sizeof(WCHAR));
        if (byte_len > Name->MaximumLength) {
            byte_len = Name->MaximumLength;
        }
        memcpy(Name->Buffer, name_buf, byte_len);
        Name->Length = byte_len;
        *NameSize = byte_len;
    } else {
        *NameSize = 0;
    }

    if (Domain != nullptr && Domain->Buffer != nullptr && domain_chars > 0) {
        USHORT byte_len = (USHORT)((domain_chars) * sizeof(WCHAR));
        if (byte_len > Domain->MaximumLength) {
            byte_len = Domain->MaximumLength;
        }
        memcpy(Domain->Buffer, domain_buf, byte_len);
        Domain->Length = byte_len;
        *DomainSize = byte_len;
    } else {
        *DomainSize = 0;
    }

    *SidNameUse = name_use;

    ExFreePool(name_buf);
    ExFreePool(domain_buf);
    return STATUS_SUCCESS;
}
