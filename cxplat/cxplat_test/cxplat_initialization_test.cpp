// Copyright (c) Microsoft Corporation
// SPDX-License-Identifier: MIT

#if !defined(CMAKE_NUGET)
#include <catch2/catch_all.hpp>
#else
#include <catch2/catch.hpp>
#endif
#include "cxplat.h"
#include "cxplat_passed_test_log.h"
CATCH_REGISTER_LISTENER(_passed_test_log)

TEST_CASE("initialize", "[initialization]")
{
    REQUIRE(cxplat_initialize() == CXPLAT_STATUS_SUCCESS);
    REQUIRE(cxplat_initialize() == CXPLAT_STATUS_SUCCESS);
    cxplat_cleanup();
    cxplat_cleanup();
}