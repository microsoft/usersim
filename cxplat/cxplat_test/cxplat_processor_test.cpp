// Copyright (c) Microsoft Corporation
// SPDX-License-Identifier: MIT

#if !defined(CMAKE_NUGET)
#include <catch2/catch_all.hpp>
#else
#include <catch2/catch.hpp>
#endif
#include "cxplat.h"

TEST_CASE("processor", "[processor]")
{
    REQUIRE(cxplat_initialize() == CXPLAT_STATUS_SUCCESS);
    uint32_t maximum = cxplat_get_maximum_processor_count();
    REQUIRE(maximum > 0);
    uint32_t current = cxplat_get_current_processor_number();
    REQUIRE(current >= 0);
    REQUIRE(current < maximum);
    cxplat_cleanup();
}
