// Copyright (c) Microsoft Corporation
// SPDX-License-Identifier: MIT

#if !defined(CMAKE_NUGET)
#include <catch2/catch_all.hpp>
#else
#include <catch2/catch.hpp>
#endif
#include "usersim/se.h"

TEST_CASE("SeExports", "[se]")
{
    PSE_EXPORTS exports = SeExports;
    REQUIRE(exports->SeLocalSystemSid != nullptr);
    REQUIRE(exports->SeAliasAdminsSid != nullptr);
}