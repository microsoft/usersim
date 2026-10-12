// Copyright (c) Microsoft Corporation
// SPDX-License-Identifier: MIT

#if !defined(CMAKE_NUGET)
#include <catch2/catch_all.hpp>
#else
#include <catch2/catch.hpp>
#endif

#include "usersim/common.h"
#include "usersim/ex.h"

#include <cstdlib>

static bool
_is_fault_injection_enabled()
{
    char value[32] = {};
    size_t required_size = 0;
    getenv_s(&required_size, value, sizeof(value), "CXPLAT_FAULT_INJECTION_SIMULATION");
    return required_size > 0 && std::strtoull(value, nullptr, 10) > 0;
}

TEST_CASE("usersim fault injection suspension", "[fault_injection]")
{
    auto call_fault_injected_usersim_api = []() {
        UUID uuid = {};
        return ExUuidCreate(&uuid);
    };

    usersim_fault_injection_suspend();
    usersim_fault_injection_suspend();
    NTSTATUS nested_suspension_status = call_fault_injected_usersim_api();

    usersim_fault_injection_resume();
    NTSTATUS single_suspension_status = call_fault_injected_usersim_api();

    usersim_fault_injection_resume();
    NTSTATUS resumed_status = call_fault_injected_usersim_api();

    REQUIRE(nested_suspension_status == STATUS_SUCCESS);
    REQUIRE(single_suspension_status == STATUS_SUCCESS);
    REQUIRE(resumed_status == (_is_fault_injection_enabled() ? STATUS_NOT_SUPPORTED : STATUS_SUCCESS));
}
