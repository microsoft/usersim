// Copyright (c) Microsoft Corporation
// SPDX-License-Identifier: MIT

#if !defined(CMAKE_NUGET)
#include <catch2/catch_all.hpp>
#else
#include <catch2/catch.hpp>
#endif
#include "usersim/rtl.h"

TEST_CASE("RtlULongAdd", "[rtl]")
{
    ULONG result;
    REQUIRE(NT_SUCCESS(RtlULongAdd(1, 2, &result)));
    REQUIRE(result == 3);

    REQUIRE(RtlULongAdd(ULONG_MAX, 1, &result) == STATUS_INTEGER_OVERFLOW);
}