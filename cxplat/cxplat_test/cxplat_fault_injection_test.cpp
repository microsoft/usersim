// Copyright (c) Microsoft Corporation
// SPDX-License-Identifier: MIT

#if !defined(CMAKE_NUGET)
#include <catch2/catch_all.hpp>
#else
#include <catch2/catch.hpp>
#endif

#include <windows.h>

#include "cxplat_fault_injection.h"


struct _on_exit
{
    std::function<void()> _func;
    _on_exit(std::function<void()> func) : _func(func) {}
    ~_on_exit() { _func(); }
};

enum class _fault_injection_test_state
{
    ExpectedFault,               /// < Fault should be injected.
    DontExpectedFault,            /// < Fault should not be injected.
    DontExpectedFaultAfterReload, /// < Fault should not be injected after reloading.
    ExpectedFaultAfterReset,     /// < Fault should be injected after reset.
    ExpectedFaultDifferentCallsite,   /// < Fault should be injected at a different callsite.
};

TEST_CASE("fault_injection", "[fault_injection]")
{
    bool fault_injection_enabled = false;
    // Verify it's disabled by default.
    REQUIRE(cxplat_fault_injection_is_enabled() == false);

    // Verify it's enabled when initialized.
    REQUIRE(cxplat_fault_injection_initialize(0) == CXPLAT_STATUS_SUCCESS);
    fault_injection_enabled = true;

    REQUIRE(cxplat_fault_injection_add_module(GetModuleHandle(nullptr)) == CXPLAT_STATUS_SUCCESS);

    _on_exit _([&]() {
        if (fault_injection_enabled) {
            cxplat_fault_injection_uninitialize();
        }
    });
    REQUIRE(cxplat_fault_injection_is_enabled() == true);

    // Clear the fault injection state.
    cxplat_fault_injection_reset();

    for (_fault_injection_test_state state = _fault_injection_test_state::ExpectedFault;
         state <= _fault_injection_test_state::ExpectedFaultDifferentCallsite;
         state = (_fault_injection_test_state)((int)state + 1)) {
        bool fault_expected;
        switch (state) {
        case _fault_injection_test_state::ExpectedFault:
            fault_expected = true;
            break;
        case _fault_injection_test_state::DontExpectedFault:
            fault_expected  = false;
            break;
        case _fault_injection_test_state::DontExpectedFaultAfterReload:
            cxplat_fault_injection_uninitialize();
            fault_injection_enabled = false;
            REQUIRE(cxplat_fault_injection_initialize(0) == CXPLAT_STATUS_SUCCESS);
            REQUIRE(cxplat_fault_injection_add_module(GetModuleHandle(nullptr)) == CXPLAT_STATUS_SUCCESS);
            fault_injection_enabled = true;
            fault_expected = false;
            break;
        case _fault_injection_test_state::ExpectedFaultAfterReset:
            cxplat_fault_injection_reset();
            fault_expected = true;
            break;
        case _fault_injection_test_state::ExpectedFaultDifferentCallsite:
            cxplat_fault_injection_reset();
            fault_expected = true;
            REQUIRE(cxplat_fault_injection_inject_fault() == true);
            break;
        }
        REQUIRE(cxplat_fault_injection_inject_fault() == fault_expected);
    }
}