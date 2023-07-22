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
