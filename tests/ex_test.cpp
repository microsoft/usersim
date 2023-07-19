// Copyright (c) Microsoft Corporation
// SPDX-License-Identifier: MIT

#if !defined(CMAKE_NUGET)
#include <catch2/catch_all.hpp>
#else
#include <catch2/catch.hpp>
#endif
#include "usersim/ex.h"

TEST_CASE("ExAllocatePool", "[ex]")
{
    uint64_t* buffer = (uint64_t*)ExAllocatePoolUninitialized(NonPagedPool, 8, 'tset');
    REQUIRE(buffer != nullptr);
    REQUIRE(*buffer != 0);
    *buffer = 0;
    ExFreePool(buffer);

    buffer = (uint64_t*)ExAllocatePoolWithTag(NonPagedPool, 8, 'tset');
    REQUIRE(buffer != nullptr);
    REQUIRE(*buffer == 0);
    *buffer = 42;
    ExFreePool(buffer);
}

TEST_CASE("ExFreePool null", "[ex]")
{
    try {
        ExFreePoolCPP(nullptr);
        REQUIRE(FALSE);
    } catch (std::exception e) {
        REQUIRE(
            strcmp(
                e.what(),
                "*** STOP 0x000000c2 (0x0000000000000046,0x0000000000000000,0x0000000000000000,0x0000000000000000)") ==
            0);
    } catch (...) {
        REQUIRE(FALSE);
    }

    try {
        ExFreePoolWithTagCPP(nullptr, 'tset');
        REQUIRE(FALSE);
    } catch (std::exception e) {
        REQUIRE(
            strcmp(
                e.what(),
                "*** STOP 0x000000c2 (0x0000000000000046,0x0000000000000000,0x0000000000000000,0x0000000000000000)") ==
            0);
    }
}