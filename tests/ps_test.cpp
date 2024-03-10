// Copyright (c) Microsoft Corporation
// SPDX-License-Identifier: MIT

#if !defined(CMAKE_NUGET)
#include <catch2/catch_all.hpp>
#else
#include <catch2/catch.hpp>
#endif
#include "usersim/ps.h"

TEST_CASE("PsSetCreateProcessNotifyRoutineEx", "[ps]")
{
    auto notify_routine = [](PEPROCESS, HANDLE, PPS_CREATE_NOTIFY_INFO) {};
    // First call should succeed.
    auto status = PsSetCreateProcessNotifyRoutineEx(notify_routine, FALSE);
    REQUIRE(status == STATUS_SUCCESS);
    // Try to set the routine again, should fail.
    status = PsSetCreateProcessNotifyRoutineEx(notify_routine, FALSE);
    REQUIRE(status == STATUS_INVALID_PARAMETER);

    // Remove the routine. Should succeed.
    status = PsSetCreateProcessNotifyRoutineEx(notify_routine, TRUE);
    REQUIRE(status == STATUS_SUCCESS);

    // Try to remove the routine again, should fail.
    status = PsSetCreateProcessNotifyRoutineEx(notify_routine, TRUE);
    REQUIRE(status == STATUS_INVALID_PARAMETER);

    // Try to remove with a NULL routine, should fail.
    status = PsSetCreateProcessNotifyRoutineEx(NULL, TRUE);
    REQUIRE(status == STATUS_INVALID_PARAMETER);

    // Try set with a NULL routine, should fail.
    status = PsSetCreateProcessNotifyRoutineEx(NULL, FALSE);
    REQUIRE(status == STATUS_INVALID_PARAMETER);

    // Set the routine again, should succeed.
    status = PsSetCreateProcessNotifyRoutineEx(notify_routine, FALSE);
    REQUIRE(status == STATUS_SUCCESS);

    // Remove the routine. Should succeed.
    status = PsSetCreateProcessNotifyRoutineEx(notify_routine, TRUE);
    REQUIRE(status == STATUS_SUCCESS);
}