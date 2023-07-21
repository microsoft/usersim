// Copyright (c) Microsoft Corporation
// SPDX-License-Identifier: MIT

#include "fault_injection.h"
#include "kernel_um.h"
#include "platform.h"
#include "usersim/se.h"

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
    if (privileges != nullptr) {
        *privileges = nullptr;
    }
    *granted_access = desired_access;

    if (usersim_fault_injection_inject_fault()) {
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
