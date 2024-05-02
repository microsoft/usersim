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

TEST_CASE("PsGetProcessExitStatus", "[ps]")
{
    // If no callback is installed, we default to -1
    auto status = PsGetProcessExitStatus((PEPROCESS)0);
    REQUIRE(status == -1);

    usersime_set_process_exit_status_callback([](PEPROCESS proc) -> NTSTATUS { return ((int)proc) + 1; });

    status = PsGetProcessExitStatus((PEPROCESS)0);
    REQUIRE(status == 1);

    status = PsGetProcessExitStatus((PEPROCESS)1234);
    REQUIRE(status == 1235);

    // Setting back to a NULL callback reverts to returning -1
    usersime_set_process_exit_status_callback(NULL);
    status = PsGetProcessExitStatus((PEPROCESS)0);
    REQUIRE(status == -1);
}

TEST_CASE("PsGetProcessCreateTimeQuadPart", "[ps]")
{
    // If no callback is installed, we default to 0
    auto time = PsGetProcessCreateTimeQuadPart((PEPROCESS)0);
    REQUIRE(time == 0);

    usersime_set_process_create_time_quadpart_callback([](PEPROCESS proc) -> LONGLONG { return ((int)proc) + 1; });

    time = PsGetProcessCreateTimeQuadPart((PEPROCESS)0);
    REQUIRE(time == 1);

    time = PsGetProcessCreateTimeQuadPart((PEPROCESS)1234);
    REQUIRE(time == 1235);

    // Setting back to a NULL callback reverts to returning 0
    usersime_set_process_create_time_quadpart_callback(NULL);
    time = PsGetProcessCreateTimeQuadPart((PEPROCESS)0);
    REQUIRE(time == 0);
}

TEST_CASE("PsGetProcessExitTime", "[ps]")
{
    // If no callback is installed, we default to 0
    auto time = PsGetProcessExitTime();
    REQUIRE(time.QuadPart == 0);

    usersime_set_process_exit_time_callback([]() -> LARGE_INTEGER { return {123}; });

    time = PsGetProcessExitTime();
    REQUIRE(time.QuadPart == 123);

    // Setting back to a NULL callback reverts to returning 0
    usersime_set_process_exit_time_callback(NULL);
    time = PsGetProcessExitTime();
    REQUIRE(time.QuadPart == 0);
}