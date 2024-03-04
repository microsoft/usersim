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

TEST_CASE("RtlInitString", "[rtl]")
{
    STRING string = {0};
    RtlInitString(&string, nullptr);
    REQUIRE(string.Buffer == nullptr);
    REQUIRE(string.Length == 0);
    REQUIRE(string.MaximumLength == 0);

    PCSTR empty = "";
    RtlInitString(&string, empty);
    REQUIRE(string.Buffer == empty);
    REQUIRE(string.Length == 0);
    REQUIRE(string.MaximumLength == 1);

    PCSTR test = "test";
    RtlInitString(&string, test);
    REQUIRE(string.Buffer == test);
    REQUIRE(string.Length == 4);
    REQUIRE(string.MaximumLength == 5);

    CHAR buffer[80] = "test";
    RtlInitString(&string, buffer);
    REQUIRE(string.Buffer == buffer);
    REQUIRE(string.Length == 4);
    REQUIRE(string.MaximumLength == 5);
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

TEST_CASE("RtlUnicodeStringToUTF8String", "[rtl]")
{
    UNICODE_STRING unicode_string = {0};
    RtlInitUnicodeString(&unicode_string, L"test");
    UTF8_STRING utf8_string = {0};
    REQUIRE(RtlUnicodeStringToUTF8String(&utf8_string, &unicode_string, TRUE) == STATUS_SUCCESS);
    REQUIRE(utf8_string.Buffer != nullptr);
    REQUIRE(utf8_string.Length == 4);
    REQUIRE(utf8_string.MaximumLength == 5);
    REQUIRE(strcmp(utf8_string.Buffer, "test") == 0);
    RtlFreeUTF8String(&utf8_string);
}

TEST_CASE("RtlUTF8StringToUnicodeString", "[rtl]")
{
    UTF8_STRING utf8_string = {0};
    RtlInitUTF8String(&utf8_string, "test");
    UNICODE_STRING unicode_string = {0};
    REQUIRE(RtlUTF8StringToUnicodeString(&unicode_string, &utf8_string, TRUE) == STATUS_SUCCESS);
    REQUIRE(unicode_string.Buffer != nullptr);
    REQUIRE(unicode_string.Length == 8);
    REQUIRE(unicode_string.MaximumLength == 10);
    REQUIRE(wcscmp(unicode_string.Buffer, L"test") == 0);
    RtlFreeUnicodeString(&unicode_string);
}