// Copyright (c) Microsoft Corporation
// SPDX-License-Identifier: MIT

#if !defined(CMAKE_NUGET)
#include <catch2/catch_all.hpp>
#else
#include <catch2/catch.hpp>
#endif
#include "usersim/etw.h"

TEST_CASE("EtwRegister", "[etw]")
{
    GUID guid = {};
    REGHANDLE reg_handle;
    REQUIRE(EtwRegister(&guid, nullptr, nullptr, &reg_handle) == STATUS_SUCCESS);
    REQUIRE(EtwUnregister(reg_handle) == STATUS_SUCCESS);
}