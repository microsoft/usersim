// Copyright (c) Microsoft Corporation
// SPDX-License-Identifier: MIT

#if !defined(CMAKE_NUGET)
#include <catch2/catch_all.hpp>
#else
#include <catch2/catch.hpp>
#endif
#include "usersim/ex.h"
#include "usersim/ps.h"
#include "usersim/rtl.h"
#include "usersim/se.h"

#include <memory>

struct _ex_pool_free_functor
{
    void
    operator()(void* buffer)
    {
        ExFreePool(buffer);
    }
};

struct _ps_deref_token_functor
{
    void
    operator()(void* token)
    {
        PsDereferencePrimaryToken((PACCESS_TOKEN)token);
    }
};

static void
_verify_sid(_In_ PSID sid)
{
    REQUIRE(RtlValidSid(sid));
    REQUIRE(RtlLengthSid(sid) > 0);
}

TEST_CASE("SeExports", "[se]")
{
    PSE_EXPORTS exports = SeExports;
    _verify_sid(exports->SeNullSid);
    _verify_sid(exports->SeWorldSid);
    _verify_sid(exports->SeLocalSid);
    _verify_sid(exports->SeCreatorOwnerSid);
    _verify_sid(exports->SeCreatorGroupSid);
    _verify_sid(exports->SeNtAuthoritySid);
    _verify_sid(exports->SeDialupSid);
    _verify_sid(exports->SeNetworkSid);
    _verify_sid(exports->SeBatchSid);
    _verify_sid(exports->SeInteractiveSid);
    _verify_sid(exports->SeLocalSystemSid);
    _verify_sid(exports->SeAliasAdminsSid);
    _verify_sid(exports->SeAliasUsersSid);
    _verify_sid(exports->SeAliasGuestsSid);
    _verify_sid(exports->SeAliasPowerUsersSid);
    _verify_sid(exports->SeAliasAccountOpsSid);
    _verify_sid(exports->SeAliasSystemOpsSid);
    _verify_sid(exports->SeAliasPrintOpsSid);
    _verify_sid(exports->SeAliasBackupOpsSid);
    _verify_sid(exports->SeAuthenticatedUsersSid);
    _verify_sid(exports->SeRestrictedSid);
    _verify_sid(exports->SeAnonymousLogonSid);
    _verify_sid(exports->SeLocalServiceSid);
    _verify_sid(exports->SeNetworkServiceSid);
    _verify_sid(exports->SeIUserSid);
    _verify_sid(exports->SeUntrustedMandatorySid);
    _verify_sid(exports->SeLowMandatorySid);
    _verify_sid(exports->SeMediumMandatorySid);
    _verify_sid(exports->SeHighMandatorySid);
    _verify_sid(exports->SeSystemMandatorySid);
    _verify_sid(exports->SeOwnerRightsSid);
    _verify_sid(exports->SeAllAppPackagesSid);
    _verify_sid(exports->SeUserModeDriversSid);
}

TEST_CASE("SeLockSubjectContext", "[se]")
{
    SECURITY_SUBJECT_CONTEXT security_subject_context = {0};
    SeCaptureSubjectContext(&security_subject_context);
    REQUIRE(security_subject_context.process_token != 0);
    REQUIRE(security_subject_context.thread_token != 0);
    REQUIRE(security_subject_context.lock_count == 0);

    SeLockSubjectContext(&security_subject_context);
    REQUIRE(security_subject_context.lock_count == 1);
    SeLockSubjectContext(&security_subject_context);
    REQUIRE(security_subject_context.lock_count == 2);

    SeUnlockSubjectContext(&security_subject_context);
    REQUIRE(security_subject_context.lock_count == 1);
    SeUnlockSubjectContext(&security_subject_context);
    REQUIRE(security_subject_context.lock_count == 0);

    LUID authentication_id = {0};
    REQUIRE(
        SeQueryAuthenticationIdToken((PACCESS_TOKEN)security_subject_context.process_token, &authentication_id) ==
        STATUS_SUCCESS);
    REQUIRE(authentication_id.LowPart != 0);
}

TEST_CASE("SeAccessCheck", "[se]")
{
    SECURITY_DESCRIPTOR security_descriptor = {0};
    SECURITY_SUBJECT_CONTEXT security_subject_context = {0};
    SeCaptureSubjectContext(&security_subject_context);

    ACCESS_MASK granted_access = 0;
    NTSTATUS access_status = 0;
    GENERIC_MAPPING generic_mapping = {0};
    KPROCESSOR_MODE processor_mode = 0;
    PPRIVILEGE_SET privileges = nullptr;
    BOOLEAN result = SeAccessCheck(
        &security_descriptor,
        &security_subject_context,
        FALSE, // subject_context_locked
        STANDARD_RIGHTS_WRITE,
        0,
        &privileges,
        &generic_mapping,
        processor_mode,
        &granted_access,
        &access_status);
    REQUIRE(result == TRUE);
    REQUIRE(granted_access == STANDARD_RIGHTS_WRITE);
    REQUIRE(access_status == STATUS_SUCCESS);

    TOKEN_ACCESS_INFORMATION token_access_information = {0};
    result = SeAccessCheckFromState(
        &security_descriptor,
        &token_access_information,
        nullptr,
        STANDARD_RIGHTS_READ,
        0,
        &privileges,
        &generic_mapping,
        processor_mode,
        &granted_access,
        &access_status);
    REQUIRE(result == TRUE);
    REQUIRE(granted_access == STANDARD_RIGHTS_READ);
    REQUIRE(access_status == STATUS_SUCCESS);
}

TEST_CASE("SeQueryInformationToken", "[se]")
{
    // Get a token handle for the current process.
    PACCESS_TOKEN token = PsReferencePrimaryToken(nullptr);
    REQUIRE(token != nullptr);
    std::unique_ptr<void, _ps_deref_token_functor> token_guard(token);

    // Query TokenUser information.
    PVOID token_info = nullptr;
    NTSTATUS status = SeQueryInformationToken(token, TokenUser, &token_info);
    REQUIRE(status == STATUS_SUCCESS);
    REQUIRE(token_info != nullptr);
    std::unique_ptr<void, _ex_pool_free_functor> token_info_guard(token_info);

    // Validate the returned SID.
    PTOKEN_USER token_user = (PTOKEN_USER)token_info;
    REQUIRE(RtlValidSid(token_user->User.Sid));
    REQUIRE(RtlLengthSid(token_user->User.Sid) > 0);
}

TEST_CASE("SecLookupAccountSid", "[se]")
{
    // Get the current process token's user SID.
    PACCESS_TOKEN token = PsReferencePrimaryToken(nullptr);
    REQUIRE(token != nullptr);
    std::unique_ptr<void, _ps_deref_token_functor> token_guard(token);

    PVOID token_info = nullptr;
    NTSTATUS status = SeQueryInformationToken(token, TokenUser, &token_info);
    REQUIRE(status == STATUS_SUCCESS);
    std::unique_ptr<void, _ex_pool_free_functor> token_info_guard(token_info);

    PTOKEN_USER token_user = (PTOKEN_USER)token_info;
    PSID sid = token_user->User.Sid;

    // First call: sizing query with NULL name/domain buffers.
    ULONG name_size = 0;
    ULONG domain_size = 0;
    SID_NAME_USE name_use;
    status = SecLookupAccountSid(sid, &name_size, NULL, &domain_size, NULL, &name_use);
    REQUIRE(status == STATUS_BUFFER_TOO_SMALL);
    REQUIRE(name_size > 0);
    REQUIRE(domain_size > 0);

    // Allocate UNICODE_STRING buffers for name and domain.
    UNICODE_STRING name_str = {0};
    name_str.Buffer = (PWSTR)ExAllocatePoolUninitialized(NonPagedPoolNx, name_size, 'tset');
    REQUIRE(name_str.Buffer != nullptr);
    std::unique_ptr<void, _ex_pool_free_functor> name_buf_guard(name_str.Buffer);
    name_str.MaximumLength = (USHORT)name_size;
    name_str.Length = 0;

    UNICODE_STRING domain_str = {0};
    domain_str.Buffer = (PWSTR)ExAllocatePoolUninitialized(NonPagedPoolNx, domain_size, 'tset');
    REQUIRE(domain_str.Buffer != nullptr);
    std::unique_ptr<void, _ex_pool_free_functor> domain_buf_guard(domain_str.Buffer);
    domain_str.MaximumLength = (USHORT)domain_size;
    domain_str.Length = 0;

    // Second call: actual lookup.
    status = SecLookupAccountSid(sid, &name_size, &name_str, &domain_size, &domain_str, &name_use);
    REQUIRE(status == STATUS_SUCCESS);
    REQUIRE(name_str.Length > 0);
    REQUIRE(domain_str.Length > 0);
}

TEST_CASE("SecLookupAccountSid_invalid_params", "[se]")
{
    ULONG name_size = 0;
    ULONG domain_size = 0;
    SID_NAME_USE name_use;

    // NULL SID should return STATUS_INVALID_PARAMETER.
    NTSTATUS status = SecLookupAccountSid(nullptr, &name_size, NULL, &domain_size, NULL, &name_use);
    REQUIRE(status == STATUS_INVALID_PARAMETER);
}
