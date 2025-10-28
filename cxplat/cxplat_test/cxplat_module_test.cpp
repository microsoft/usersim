// Copyright (c) Microsoft Corporation
// SPDX-License-Identifier: MIT

#if !defined(CMAKE_NUGET)
#include <catch2/catch_all.hpp>
#else
#include <catch2/catch.hpp>
#endif
#include "cxplat.h"

TEST_CASE("module_from_address_test", "[module]")
{
    cxplat_utf8_string_t path = {0};
    cxplat_status_t status =
        cxplat_get_module_path_from_address((const void*)cxplat_get_module_path_from_address, &path);
    REQUIRE(status == CXPLAT_STATUS_SUCCESS);
    std::string path_str(reinterpret_cast<char*>(path.value), path.length - 1); // Exclude null terminator
    std::string expected_path = "cxplat_test";

    // Check if the path contains the expected substring
    REQUIRE(path_str.find(expected_path) != std::string::npos);
}
