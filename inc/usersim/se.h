// Copyright (c) Microsoft Corporation
// SPDX-License-Identifier: MIT

#pragma once

#include "..\src\platform.h"
#include "ke.h"

CXPLAT_EXTERN_C_BEGIN

typedef struct _SE_EXPORTS
{
    // Privilege values
    LUID SeCreateTokenPrivilege;
    LUID SeAssignPrimaryTokenPrivilege;
    LUID SeLockMemoryPrivilege;
    LUID SeIncreaseQuotaPrivilege;
    LUID SeUnsolicitedInputPrivilege;
    LUID SeTcbPrivilege;
    LUID SeSecurityPrivilege;
    LUID SeTakeOwnershipPrivilege;
    LUID SeLoadDriverPrivilege;
    LUID SeCreatePagefilePrivilege;
    LUID SeIncreaseBasePriorityPrivilege;
    LUID SeSystemProfilePrivilege;
    LUID SeSystemtimePrivilege;
    LUID SeProfileSingleProcessPrivilege;
    LUID SeCreatePermanentPrivilege;
    LUID SeBackupPrivilege;
    LUID SeRestorePrivilege;
    LUID SeShutdownPrivilege;
    LUID SeDebugPrivilege;
    LUID SeAuditPrivilege;
    LUID SeSystemEnvironmentPrivilege;
    LUID SeChangeNotifyPrivilege;
    LUID SeRemoteShutdownPrivilege;

    // Universally defined Sids
    PSID SeNullSid;
    PSID SeWorldSid;
    PSID SeLocalSid;
    PSID SeCreatorOwnerSid;
    PSID SeCreatorGroupSid;

    // Nt defined Sids
    PSID SeNtAuthoritySid;
    PSID SeDialupSid;
    PSID SeNetworkSid;
    PSID SeBatchSid;
    PSID SeInteractiveSid;
    PSID SeLocalSystemSid;
    PSID SeAliasAdminsSid;
    PSID SeAliasUsersSid;
    PSID SeAliasGuestsSid;
    PSID SeAliasPowerUsersSid;
    PSID SeAliasAccountOpsSid;
    PSID SeAliasSystemOpsSid;
    PSID SeAliasPrintOpsSid;
    PSID SeAliasBackupOpsSid;

    // New Sids defined for NT5
    PSID SeAuthenticatedUsersSid;

    PSID SeRestrictedSid;
    PSID SeAnonymousLogonSid;

    // New Privileges defined for NT5
    LUID SeUndockPrivilege;
    LUID SeSyncAgentPrivilege;
    LUID SeEnableDelegationPrivilege;

    // New Sids defined for post-Windows 2000
    PSID SeLocalServiceSid;
    PSID SeNetworkServiceSid;

    // New Privileges defined for post-Windows 2000
    LUID SeManageVolumePrivilege;
    LUID SeImpersonatePrivilege;
    LUID SeCreateGlobalPrivilege;

    // New Privileges defined for post Windows Server 2003
    LUID SeTrustedCredManAccessPrivilege;
    LUID SeRelabelPrivilege;
    LUID SeIncreaseWorkingSetPrivilege;

    LUID SeTimeZonePrivilege;
    LUID SeCreateSymbolicLinkPrivilege;

    // New Sids defined for post Windows Server 2003
    PSID SeIUserSid;

    // Mandatory Sids, ordered lowest to highest.
    PSID SeUntrustedMandatorySid;
    PSID SeLowMandatorySid;
    PSID SeMediumMandatorySid;
    PSID SeHighMandatorySid;
    PSID SeSystemMandatorySid;

    PSID SeOwnerRightsSid;

    // Package/Capability Sids.
    PSID SeAllAppPackagesSid;
    PSID SeUserModeDriversSid;

    // Process Trust Sids.
    PSID SeProcTrustWinTcbSid;

    // Trusted Installer SID.
    PSID SeTrustedInstallerSid;

    // New Privileges defined for Windows 10
    LUID SeDelegateSessionUserImpersonatePrivilege;

    // App Silo SID
    PSID SeAppSiloSid;

    // App Silo Volume Root Minimal Capability SID
    PSID SeAppSiloVolumeRootMinimalCapabilitySid;

    // App Silo Users Minimal Capability SID
    PSID SeAppSiloProfilesRootMinimalCapabilitySid;
} SE_EXPORTS, *PSE_EXPORTS;

USERSIM_API
extern PSE_EXPORTS SeExports;

typedef struct _SECURITY_SUBJECT_CONTEXT
{
    // These field names intentionally differ from the ones in the DDK
    // since the DDK header contains a prominent notice that drivers
    // should treat this struct as opaque.
    HANDLE thread_token;
    HANDLE process_token;
    uint64_t lock_count;
} SECURITY_SUBJECT_CONTEXT, *PSECURITY_SUBJECT_CONTEXT;

USERSIM_API
VOID
SeCaptureSubjectContext(_Out_ PSECURITY_SUBJECT_CONTEXT subject_context);

USERSIM_API
VOID
SeReleaseSubjectContext(_Inout_ PSECURITY_SUBJECT_CONTEXT SubjectContext);

USERSIM_API
VOID
SeLockSubjectContext(_In_ PSECURITY_SUBJECT_CONTEXT subject_context);

USERSIM_API
VOID
SeUnlockSubjectContext(_In_ PSECURITY_SUBJECT_CONTEXT subject_context);

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
    _Out_ PNTSTATUS access_status);

USERSIM_API
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
    _Out_ NTSTATUS* access_status);

USERSIM_API
NTSTATUS
SeQueryAuthenticationIdToken(_In_ PACCESS_TOKEN token, _Out_ PLUID authentication_id);

void
usersim_initialize_se();

CXPLAT_EXTERN_C_END
