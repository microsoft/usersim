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
    // Try an allocation that need not be cache aligned.
    uint64_t* buffer = (uint64_t*)ExAllocatePoolUninitialized(NonPagedPoolNx, 8, 'tset');
    REQUIRE(buffer != nullptr);
#ifndef NDEBUG
    REQUIRE(*buffer != 0);
#endif
    *buffer = 0;
    ExFreePool(buffer);

    // Try an allocation that must be cache aligned.
    buffer = (uint64_t*)ExAllocatePoolUninitialized(NonPagedPoolNxCacheAligned, 8, 'tset');
    REQUIRE(buffer != nullptr);
#ifndef NDEBUG
    REQUIRE(*buffer != 0);
#endif
    REQUIRE((((uintptr_t)buffer) % 64) == 0);
    *buffer = 0;
    ExFreePool(buffer);

    buffer = (uint64_t*)ExAllocatePoolWithTag(NonPagedPoolNx, 8, 'tset');
    REQUIRE(buffer != nullptr);
    REQUIRE(*buffer == 0);
    *buffer = 42;
    ExFreePool(buffer);
}

TEST_CASE("ExAllocatePool2", "[ex]")
{
    // Try an allocation that need not be cache aligned.
    uint64_t* buffer = (uint64_t*)ExAllocatePool2(CXPLAT_POOL_FLAG_NON_PAGED | CXPLAT_POOL_FLAG_UNINITIALIZED, 8, 'tset');
    REQUIRE(buffer != nullptr);
#ifndef NDEBUG
    REQUIRE(*buffer != 0);
#endif
    *buffer = 0;
    ExFreePool(buffer);

    // Try an allocation that must be cache aligned.
    buffer = (uint64_t*)ExAllocatePool2(
        CXPLAT_POOL_FLAG_NON_PAGED | CXPLAT_POOL_FLAG_UNINITIALIZED | CXPLAT_POOL_FLAG_CACHE_ALIGNED, 8, 'tset');
    REQUIRE(buffer != nullptr);
#ifndef NDEBUG
    REQUIRE(*buffer != 0);
#endif
    REQUIRE((((uintptr_t)buffer) % 64) == 0);
    *buffer = 0;
    ExFreePool(buffer);

    buffer = (uint64_t*)ExAllocatePool2(CXPLAT_POOL_FLAG_NON_PAGED, 8, 'tset');
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

TEST_CASE("ExRaiseAccessViolation", "[ex]")
{
    try {
        ExRaiseAccessViolationCPP();
        REQUIRE(FALSE);
    } catch (std::exception e) {
        PCSTR message = e.what();
        PCSTR ex = message + strlen("Exception: ");
        int64_t code = _atoi64(ex);
        REQUIRE(code == STATUS_ACCESS_VIOLATION);
    }
}

TEST_CASE("ExRaiseDatatypeMisalignment", "[ex]")
{
    try {
        ExRaiseDatatypeMisalignmentCPP();
        REQUIRE(FALSE);
    } catch (std::exception e) {
        PCSTR message = e.what();
        PCSTR ex = message + strlen("Exception: ");
        int64_t code = _atoi64(ex);
        REQUIRE(code == STATUS_DATATYPE_MISALIGNMENT);
    }
}

TEST_CASE("LOOKASIDE_LIST_EX", "[ex]")
{
    LOOKASIDE_LIST_EX lookaside;
    REQUIRE(
        ExInitializeLookasideListEx(&lookaside, nullptr, nullptr, NonPagedPoolNx, 0, 8, 'tset', 0) == STATUS_SUCCESS);

    void* p = ExAllocateFromLookasideListEx(&lookaside);
    REQUIRE(p != nullptr);

    ExFreeToLookasideListEx(&lookaside, p);
    ExDeleteLookasideListEx(&lookaside);
}