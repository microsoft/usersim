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
    char path[MAX_PATH];
    size_t path_length_out = 0;
    cxplat_status_t status = cxplat_get_module_path_from_address(
        (const void*)cxplat_get_module_path_from_address, path, sizeof(path), &path_length_out);
    REQUIRE(status == CXPLAT_STATUS_SUCCESS);
    REQUIRE(path_length_out > 0);
    REQUIRE(path[0] != '\0');
    REQUIRE(path);
    std::string path_str(path, path_length_out);
    std::string expected_path = "cxplat_test";

    // Check if the path contains the expected substring
    REQUIRE(path_str.find(expected_path) != std::string::npos);
}
