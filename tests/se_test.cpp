// Copyright (c) Microsoft Corporation
// SPDX-License-Identifier: MIT

#if !defined(CMAKE_NUGET)
#include <catch2/catch_all.hpp>
#else
#include <catch2/catch.hpp>
#endif
#include "usersim/rtl.h"
#include "usersim/se.h"

TEST_CASE("SeExports", "[se]")
{
    PSE_EXPORTS exports = SeExports;
    REQUIRE(RtlValidSid(exports->SeLocalSystemSid));
    REQUIRE(RtlLengthSid(exports->SeLocalSystemSid) > 0);

    REQUIRE(RtlValidSid(exports->SeAliasAdminsSid));
    REQUIRE(RtlLengthSid(exports->SeAliasAdminsSid) > 0);
}