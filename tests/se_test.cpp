// Copyright (c) Microsoft Corporation
// SPDX-License-Identifier: MIT

#if !defined(CMAKE_NUGET)
#include <catch2/catch_all.hpp>
#else
#include <catch2/catch.hpp>
#endif
#include "usersim/rtl.h"
#include "usersim/se.h"

static void _verify_sid(_In_ PSID sid)
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
