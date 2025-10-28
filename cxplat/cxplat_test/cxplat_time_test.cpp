// Copyright (c) Microsoft Corporation
// SPDX-License-Identifier: MIT

#if !defined(CMAKE_NUGET)
#include <catch2/catch_all.hpp>
#else
#include <catch2/catch.hpp>
#endif
#include "cxplat.h"

#include <Windows.h>

TEST_CASE("query_time_precise_include_suspend", "[time]")
{
    uint64_t time1 = cxplat_query_time_since_boot_precise(true);
    Sleep(2);
    uint64_t time2 = cxplat_query_time_since_boot_precise(true);

    // The time difference should be at least 10000 filetime units (1ms)
    REQUIRE(time2 - time1 >= 10000);
}

TEST_CASE("query_time_precise", "[time]")
{
    uint64_t time1 = cxplat_query_time_since_boot_precise(false);
    Sleep(2);
    uint64_t time2 = cxplat_query_time_since_boot_precise(false);

    // The time difference should be at least 10000 filetime units (1ms)
    REQUIRE(time2 - time1 >= 10000);
}

TEST_CASE("query_time_approximate_include_suspend", "[time]")
{
    uint64_t time1 = cxplat_query_time_since_boot_approximate(true);
    Sleep(2);
    uint64_t time2 = cxplat_query_time_since_boot_approximate(true);

    // The time difference should be at least 10000 filetime units (1ms)
    REQUIRE(time2 - time1 >= 10000);
}

TEST_CASE("query_time_approximate", "[time]")
{
    uint64_t time1 = cxplat_query_time_since_boot_approximate(false);
    Sleep(2);
    uint64_t time2 = cxplat_query_time_since_boot_approximate(false);

    // The time difference should be at least 10000 filetime units (1ms)
    REQUIRE(time2 - time1 >= 10000);
}
