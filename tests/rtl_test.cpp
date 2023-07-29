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

TEST_CASE("RtlAssert", "[rtl]")
{
    try {
        RtlAssertCPP((PVOID) "0 == 1", (PVOID) "filename.c", 17, nullptr);
        REQUIRE(FALSE);
    } catch (std::exception e) {
        REQUIRE(
            strcmp(
                e.what(),
                "*** STOP 0x00000000 (0x0000000000000011,0x0000000000000000,0x0000000000000000,0x0000000000000000)") ==
            0);
    }
}

TEST_CASE("RtlInitUnicodeString", "[rtl]")
{
    UNICODE_STRING unicode_string = {0};
    RtlInitUnicodeString(&unicode_string, nullptr);
    REQUIRE(unicode_string.Buffer == nullptr);
    REQUIRE(unicode_string.Length == 0);
    REQUIRE(unicode_string.MaximumLength == 0);

    PCWSTR empty = L"";
    RtlInitUnicodeString(&unicode_string, empty);
    REQUIRE(unicode_string.Buffer == empty);
    REQUIRE(unicode_string.Length == 0);
    REQUIRE(unicode_string.MaximumLength == 2);

    PCWSTR test = L"test";
    RtlInitUnicodeString(&unicode_string, test);
    REQUIRE(unicode_string.Buffer == test);
    REQUIRE(unicode_string.Length == 8);
    REQUIRE(unicode_string.MaximumLength == 10);

    WCHAR buffer[80] = L"test";
    RtlInitUnicodeString(&unicode_string, buffer);
    REQUIRE(unicode_string.Buffer == buffer);
    REQUIRE(unicode_string.Length == 8);
    REQUIRE(unicode_string.MaximumLength == 10);
}