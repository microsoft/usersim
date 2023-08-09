// Copyright (c) Microsoft Corporation
// SPDX-License-Identifier: MIT

#if !defined(CMAKE_NUGET)
#include <catch2/catch_all.hpp>
#else
#include <catch2/catch.hpp>
#endif
#include "usersim/zw.h"

static void
_test_zwcreatekey(_In_ PCWSTR path)
{
    NTSTATUS status;
    HANDLE key_handle = nullptr;
    UNICODE_STRING object_name;
    RtlInitUnicodeString(&object_name, path);
    OBJECT_ATTRIBUTES object_attributes = {.Length = sizeof(OBJECT_ATTRIBUTES), .ObjectName = &object_name};
    ULONG disposition;
    status = ZwCreateKey(&key_handle, KEY_WRITE, &object_attributes, 0, nullptr, REG_OPTION_VOLATILE, &disposition);
    REQUIRE(status == STATUS_SUCCESS);
    REQUIRE((disposition == REG_CREATED_NEW_KEY || disposition == REG_OPENED_EXISTING_KEY));
    REQUIRE(key_handle != nullptr);
    REQUIRE(ZwClose(key_handle) == STATUS_SUCCESS);

    // Try creating when already exists.  We use KEY_QUERY_VALUE to make sure that read access
    // still finds the same key we created above.
    status =
        ZwCreateKey(&key_handle, KEY_QUERY_VALUE, &object_attributes, 0, nullptr, REG_OPTION_VOLATILE, &disposition);
    REQUIRE(status == STATUS_SUCCESS);
    REQUIRE(disposition == REG_OPENED_EXISTING_KEY);
    REQUIRE(ZwClose(key_handle) == STATUS_SUCCESS);

    // Delete the key we created above.
    status =
        ZwCreateKey(&key_handle, DELETE, &object_attributes, 0, nullptr, REG_OPTION_VOLATILE, &disposition);
    REQUIRE(status == STATUS_SUCCESS);
    REQUIRE(disposition == REG_OPENED_EXISTING_KEY);
    REQUIRE(ZwDeleteKey(key_handle) == STATUS_SUCCESS);
    REQUIRE(ZwClose(key_handle) == STATUS_SUCCESS);
}

TEST_CASE("ZwCreateKey HKCU", "[zw]")
{
    // In kernel-mode code, HKCU is \Registry\User.
    _test_zwcreatekey(L"\\Registry\\User\\Software\\Usersim\\Test");
}

TEST_CASE("ZwCreateKey HKLM", "[zw]")
{
    // In kernel-mode code, HKLM is \Registry\Machine.
    // Usersim will internally use HKCU if the test is not run as admin.
    _test_zwcreatekey(L"\\Registry\\Machine\\Software\\Usersim\\Test");
}