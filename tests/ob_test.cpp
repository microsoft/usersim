// Copyright (c) Microsoft Corporation
// SPDX-License-Identifier: MIT

#if !defined(CMAKE_NUGET)
#include <catch2/catch_all.hpp>
#else
#include <catch2/catch.hpp>
#endif
#include "usersim/ob.h"

TEST_CASE("ObfReferenceObject", "[ob]")
{
    int x = 0;
    HANDLE handle = &x;
    void* object;
    REQUIRE(ObReferenceObjectByHandle(handle, 0, nullptr, 0, &object, nullptr) == STATUS_SUCCESS);
    REQUIRE(object == &x);
    REQUIRE(ObfReferenceObject(&x) == 2);
    REQUIRE(ObfReferenceObject(&x) == 3);
    REQUIRE(ObCloseHandle(handle, 0) == STATUS_SUCCESS);
    REQUIRE(ObfDereferenceObject(&x) == 1);

    object = nullptr;
    REQUIRE(ObReferenceObjectByHandle(handle, 0, *ExEventObjectType, 0, &object, nullptr) == STATUS_SUCCESS);
    REQUIRE(object == &x);
}
