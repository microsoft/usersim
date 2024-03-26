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

static RTL_GENERIC_COMPARE_RESULTS
_test_avl_compare_routine(_In_ RTL_AVL_TABLE* table, _In_ PVOID first_struct, _In_ PVOID second_struct)
{
    int first = *(reinterpret_cast<int*>(first_struct));
    int second = *(reinterpret_cast<int*>(second_struct));

    if (first < second) {
        return GenericLessThan;
    } else if (first > second) {
        return GenericGreaterThan;
    } else {
        return GenericEqual;
    }
}

static PVOID
_test_avl_allocate_routine(_In_ struct _RTL_AVL_TABLE* Table, _In_ CLONG ByteSize)
{
    return malloc(ByteSize);
}

static VOID
_test_avl_free_routine(_In_ struct _RTL_AVL_TABLE* Table, _In_ PVOID Buffer)
{
    free(Buffer);
}

TEST_CASE("RtlInitializeGenericTableAvl", "[rtl]")
{
    RTL_AVL_TABLE table = {0};
    int context = 0;

    // Invoke without context
    RtlInitializeGenericTableAvl(
        &table, _test_avl_compare_routine, _test_avl_allocate_routine, _test_avl_free_routine, nullptr);

    // Invoke with context
    RtlInitializeGenericTableAvl(
        &table, _test_avl_compare_routine, _test_avl_allocate_routine, _test_avl_free_routine, &context);
}

TEST_CASE("RtlInsertElementGenericTableAvl", "[rtl]")
{
    RTL_AVL_TABLE table = {0};
    int entry = 0;
    BOOLEAN new_element = FALSE;

    RtlInitializeGenericTableAvl(
        &table, _test_avl_compare_routine, _test_avl_allocate_routine, _test_avl_free_routine, nullptr);

    // Insert a new entry.
    REQUIRE(RtlInsertElementGenericTableAvl(&table, &entry, sizeof(entry), &new_element) != nullptr);
    REQUIRE(new_element == TRUE);

    // Re-insert the same entry.
    REQUIRE(RtlInsertElementGenericTableAvl(&table, &entry, sizeof(entry), &new_element) != nullptr);
    REQUIRE(new_element == FALSE);

    // Insert the another new entry.
    entry = 1;
    REQUIRE(RtlInsertElementGenericTableAvl(&table, &entry, sizeof(entry), &new_element) != nullptr);
    REQUIRE(new_element == TRUE);

    // Remove the entries
    entry = 0;
    REQUIRE(RtlDeleteElementGenericTableAvl(&table, &entry) == TRUE);
    entry = 1;
    REQUIRE(RtlDeleteElementGenericTableAvl(&table, &entry) == TRUE);
}

TEST_CASE("RtlInsertElementGenericTableFullAvl", "[rtl]")
{
    RTL_AVL_TABLE table = {0};
    int entry = 0;
    BOOLEAN new_element = FALSE;
    PVOID node_or_parent = nullptr;
    TABLE_SEARCH_RESULT search_result;

    RtlInitializeGenericTableAvl(
        &table, _test_avl_compare_routine, _test_avl_allocate_routine, _test_avl_free_routine, nullptr);

    // Lookup entry
    REQUIRE(RtlLookupElementGenericTableFullAvl(&table, &entry, &node_or_parent, &search_result) == nullptr);

    // Insert entry
    REQUIRE(
        RtlInsertElementGenericTableFullAvl(&table, &entry, sizeof(entry), nullptr, node_or_parent, search_result) !=
        nullptr);

    // Search for entry while table is populated
    entry = 1;
    REQUIRE(RtlLookupElementGenericTableFullAvl(&table, &entry, &node_or_parent, &search_result) == nullptr);

    // Insert entry
    REQUIRE(
        RtlInsertElementGenericTableFullAvl(&table, &entry, sizeof(entry), nullptr, node_or_parent, search_result) !=
        nullptr);

    // Delete added entries
    entry = 0;
    REQUIRE(RtlDeleteElementGenericTableAvl(&table, &entry) == TRUE);
    entry = 1;
    REQUIRE(RtlDeleteElementGenericTableAvl(&table, &entry) == TRUE);
}

TEST_CASE("RtlDeleteElementGenericTableAvl", "[rtl]")
{
    RTL_AVL_TABLE table = {0};
    int entry = 0;

    RtlInitializeGenericTableAvl(
        &table, _test_avl_compare_routine, _test_avl_allocate_routine, _test_avl_free_routine, nullptr);

    // Deleting an entry which does not exist should fail.
    REQUIRE(RtlDeleteElementGenericTableAvl(&table, &entry) == FALSE);

    // Insert and remove the entry.
    REQUIRE(RtlInsertElementGenericTableAvl(&table, &entry, sizeof(entry), nullptr) != nullptr);
    REQUIRE(RtlDeleteElementGenericTableAvl(&table, &entry) == TRUE);

    // Deleting an already deleted enry should fail.
    REQUIRE(RtlDeleteElementGenericTableAvl(&table, &entry) == FALSE);
}

TEST_CASE("RtlGetElementGenericTableAvl", "[rtl]")
{
    RTL_AVL_TABLE table = {0};
    int entry = 0;

    RtlInitializeGenericTableAvl(
        &table, _test_avl_compare_routine, _test_avl_allocate_routine, _test_avl_free_routine, nullptr);

    for (int i = 0; i < 100; ++i) {
        // Get on element i prior to insertion. This should fail.
        REQUIRE(RtlGetElementGenericTableAvl(&table, i) == nullptr);

        // Get on element should succeed after insertions.
        REQUIRE(RtlInsertElementGenericTableAvl(&table, &i, sizeof(i), nullptr) != nullptr);
        REQUIRE(RtlGetElementGenericTableAvl(&table, i) != nullptr);
    }

    // Remove all elements.
    for (int i = 0; i < 100; ++i) {
        REQUIRE(RtlDeleteElementGenericTableAvl(&table, &i));
    }
}

TEST_CASE("RtlLookupElementGenericTableAvl", "[rtl]")
{
    RTL_AVL_TABLE table = {0};
    int entry = 0;

    RtlInitializeGenericTableAvl(
        &table, _test_avl_compare_routine, _test_avl_allocate_routine, _test_avl_free_routine, nullptr);

    // Lookup on an empty table should return nullptr.
    REQUIRE(RtlLookupElementGenericTableAvl(&table, &entry) == nullptr);

    // Lookup should succeed after inserting an element.
    REQUIRE(RtlInsertElementGenericTableAvl(&table, &entry, sizeof(entry), nullptr) != nullptr);
    PVOID buffer = RtlLookupElementGenericTableAvl(&table, &entry);
    REQUIRE(buffer != nullptr);
    REQUIRE(entry == *(reinterpret_cast<int*>(buffer)));

    // Lookup should fail after removingthe element.
    REQUIRE(RtlDeleteElementGenericTableAvl(&table, &entry) == TRUE);
    REQUIRE(RtlLookupElementGenericTableAvl(&table, &entry) == nullptr);
}

TEST_CASE("RtlLookupElementGenericTableFullAvl", "[rtl]")
{
    RTL_AVL_TABLE table = {0};
    int entry = 0;
    PVOID node_or_parent = nullptr;
    TABLE_SEARCH_RESULT result;

    RtlInitializeGenericTableAvl(
        &table, _test_avl_compare_routine, _test_avl_allocate_routine, _test_avl_free_routine, nullptr);

    // Lookup on an empty table should return nullptr.
    REQUIRE(RtlLookupElementGenericTableFullAvl(&table, &entry, &node_or_parent, &result) == nullptr);
    REQUIRE(result == TableEmptyTree);

    // Lookup should succeed after inserting an element.
    REQUIRE(RtlInsertElementGenericTableAvl(&table, &entry, sizeof(entry), nullptr) != nullptr);
    REQUIRE(RtlLookupElementGenericTableFullAvl(&table, &entry, &node_or_parent, &result) != nullptr);
    REQUIRE(result == TableFoundNode);

    // Search for an entry greater than the inserted entry.
    entry = 1;
    REQUIRE(RtlLookupElementGenericTableFullAvl(&table, &entry, &node_or_parent, &result) == nullptr);
    REQUIRE(result == TableInsertAsRight);

    // Search for an entry less than the inserted entry.
    entry = -1;
    REQUIRE(RtlLookupElementGenericTableFullAvl(&table, &entry, &node_or_parent, &result) == nullptr);
    REQUIRE(result == TableInsertAsLeft);

    // Delete the entry
    entry = 0;
    REQUIRE(RtlDeleteElementGenericTableAvl(&table, &entry) == TRUE);
}

TEST_CASE("RtlLookupFirstMatchingElementGenericTableAvl", "[rtl]")
{
    RTL_AVL_TABLE table = {0};
    int entry = 0;
    PVOID restart_key = nullptr;

    RtlInitializeGenericTableAvl(
        &table, _test_avl_compare_routine, _test_avl_allocate_routine, _test_avl_free_routine, nullptr);

    // Lookup on an empty table should return nullptr.
    REQUIRE(RtlLookupFirstMatchingElementGenericTableAvl(&table, &entry, &restart_key) == nullptr);
    REQUIRE(restart_key == nullptr);

    // Lookup should succeed after inserting an element.
    REQUIRE(RtlInsertElementGenericTableAvl(&table, &entry, sizeof(entry), nullptr) != nullptr);
    REQUIRE(RtlLookupFirstMatchingElementGenericTableAvl(&table, &entry, &restart_key) != nullptr);
    REQUIRE(restart_key != nullptr);

    // Delete the entry
    entry = 0;
    REQUIRE(RtlDeleteElementGenericTableAvl(&table, &entry) == TRUE);
}

TEST_CASE("RtlEnumerateGenericTableAvl", "[rtl]")
{
    RTL_AVL_TABLE table = {0};
    int entry = 0;

    RtlInitializeGenericTableAvl(
        &table, _test_avl_compare_routine, _test_avl_allocate_routine, _test_avl_free_routine, nullptr);

    // Populate the table
    for (int i = 0; i < 100; ++i) {
        REQUIRE(RtlInsertElementGenericTableAvl(&table, &i, sizeof(i), nullptr) != nullptr);
    }

    int expected_entry = 0;
    PVOID enumerated_entry = nullptr;
    for (enumerated_entry = RtlEnumerateGenericTableAvl(&table, TRUE); enumerated_entry != nullptr;
         enumerated_entry = RtlEnumerateGenericTableAvl(&table, FALSE)) {

        REQUIRE(expected_entry++ == *(reinterpret_cast<int*>(enumerated_entry)));
    }

    // Remove all elements.
    for (int i = 0; i < 100; ++i) {
        REQUIRE(RtlDeleteElementGenericTableAvl(&table, &i));
    }
}

TEST_CASE("RtlEnumerateGenericTableWithoutSplayingAvl", "[rtl]")
{
    RTL_AVL_TABLE table = {0};
    int entry = 0;

    RtlInitializeGenericTableAvl(
        &table, _test_avl_compare_routine, _test_avl_allocate_routine, _test_avl_free_routine, nullptr);

    // Populate the table
    for (int i = 0; i < 100; ++i) {
        REQUIRE(RtlInsertElementGenericTableAvl(&table, &i, sizeof(i), nullptr) != nullptr);
    }

    int expected_entry = 0;
    PVOID enumerated_entry = nullptr;
    PVOID restart_key = nullptr;
    for (enumerated_entry = RtlEnumerateGenericTableWithoutSplayingAvl(&table, &restart_key);
         restart_key != nullptr && enumerated_entry != nullptr;
         enumerated_entry = RtlEnumerateGenericTableWithoutSplayingAvl(&table, &restart_key)) {

        REQUIRE(expected_entry++ == *(reinterpret_cast<int*>(enumerated_entry)));
    }

    // Remove all elements.
    for (int i = 0; i < 100; ++i) {
        REQUIRE(RtlDeleteElementGenericTableAvl(&table, &i));
    }
}

TEST_CASE("RtlIsGenericTableEmptyAvl", "[rtl]")
{
    RTL_AVL_TABLE table = {0};
    int entry = 0;

    RtlInitializeGenericTableAvl(
        &table, _test_avl_compare_routine, _test_avl_allocate_routine, _test_avl_free_routine, nullptr);

    // Table should be empty after initialization.
    REQUIRE(RtlIsGenericTableEmptyAvl(&table) == TRUE);

    // Table should not be empty after inserting an element.
    REQUIRE(RtlInsertElementGenericTableAvl(&table, &entry, sizeof(entry), nullptr) != nullptr);
    REQUIRE(RtlIsGenericTableEmptyAvl(&table) == FALSE);

    // Table should be empty after removing the element.
    REQUIRE(RtlDeleteElementGenericTableAvl(&table, &entry) == TRUE);
    REQUIRE(RtlIsGenericTableEmptyAvl(&table) == TRUE);
}

TEST_CASE("RtlNumberGenericTableElementsAvl", "[rtl]")
{
    RTL_AVL_TABLE table = {0};
    int entry = 0;

    RtlInitializeGenericTableAvl(
        &table, _test_avl_compare_routine, _test_avl_allocate_routine, _test_avl_free_routine, nullptr);

    for (int i = 0; i < 100; ++i) {
        // Get count of elements prior to insertion.
        REQUIRE(RtlNumberGenericTableElementsAvl(&table) == i);

        // Insert a new element and get the count.
        REQUIRE(RtlInsertElementGenericTableAvl(&table, &i, sizeof(i), nullptr) != nullptr);
        REQUIRE(RtlNumberGenericTableElementsAvl(&table) == i + 1);

        // Re-insertion should not affect count of elements.
        REQUIRE(RtlInsertElementGenericTableAvl(&table, &i, sizeof(i), nullptr) != nullptr);
        REQUIRE(RtlNumberGenericTableElementsAvl(&table) == i + 1);
    }

    // Remove all elements.
    for (int i = 0; i < 100; ++i) {
        REQUIRE(RtlDeleteElementGenericTableAvl(&table, &i));
    }
}