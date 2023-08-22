// Copyright (c) Microsoft Corporation
// SPDX-License-Identifier: MIT

#if !defined(CMAKE_NUGET)
#include <catch2/catch_all.hpp>
#else
#include <catch2/catch.hpp>
#endif
#include "cxplat.h"

TEST_CASE("cxplat_safe_size_t_multiply", "[size]")
{
    size_t result;
    REQUIRE(cxplat_safe_size_t_multiply(3, 5, &result) == CXPLAT_STATUS_SUCCESS);
    REQUIRE(result == 15);

    REQUIRE(cxplat_safe_size_t_multiply(SIZE_MAX, 2, &result) == CXPLAT_STATUS_ARITHMETIC_OVERFLOW);
}

TEST_CASE("cxplat_safe_size_t_add", "[size]")
{
    size_t result;
    REQUIRE(cxplat_safe_size_t_add(3, 5, &result) == CXPLAT_STATUS_SUCCESS);
    REQUIRE(result == 8);

    REQUIRE(cxplat_safe_size_t_add(SIZE_MAX, 2, &result) == CXPLAT_STATUS_ARITHMETIC_OVERFLOW);
}

TEST_CASE("cxplat_safe_size_t_subtract", "[size]")
{
    size_t result;
    REQUIRE(cxplat_safe_size_t_subtract(5, 3, &result) == CXPLAT_STATUS_SUCCESS);
    REQUIRE(result == 2);

    REQUIRE(cxplat_safe_size_t_subtract(3, 5, &result) == CXPLAT_STATUS_ARITHMETIC_OVERFLOW);
}