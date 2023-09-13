// Copyright (c) Microsoft Corporation
// SPDX-License-Identifier: MIT

#if !defined(CMAKE_NUGET)
#include <catch2/catch_all.hpp>
#else
#include <catch2/catch.hpp>
#endif

#include "cxplat_fault_injection.h"

#include <windows.h>

struct _on_exit
{
    std::function<void()> _func;
    _on_exit(std::function<void()> func) : _func(func) {}
    ~_on_exit() { _func(); }
};

/// @brief  Enumerates the expected outcomes of fault injection tests.
// The test will cycle through and perform the following actions:
// 1. ExpectFault: Fault should be expected. This is the initial state, with no prior calls to
//    cxplat_fault_injection_inject_fault(). It should return true.
// 2. DontExpectFault: Fault should not be expected. This the second state, with one prior call to
//    cxplat_fault_injection_inject_fault(). It should return false.
// 3. DontExpectFaultAfterReload: Fault should not be expected. This is the third state, with some number of
//    prior calls to cxplat_fault_injection_inject_fault() followed by a call to cxplat_fault_injection_uninitialize()
//    and cxplat_fault_injection_initialize(). It should return false as the state is persisted across reloads.
// 4. ExpectFaultAfterReset: Fault should be expected. This is the fourth state, with some number of prior calls to
//    cxplat_fault_injection_inject_fault() followed by a call to cxplat_fault_injection_reset(). It should return true
//    as the state is reset.
// 5. ExpectFaultDifferentCallsite: Fault should be expected. This is the fifth state, with the state reset and a call
//    to cxplat_fault_injection_inject_fault() at a different callsite. It should return true as the callsite is
//    different.
enum class _fault_injection_expected_outcome
{
    ExpectFault,                  /// < Fault should be expected.
    DontExpectFault,              /// < Fault should not be expected.
    DontExpectFaultAfterReload,   /// < Fault should not be expected after reloading.
    ExpectFaultAfterReset,        /// < Fault should be expected after reset.
    ExpectFaultDifferentCallsite, /// < Fault should be expected at a different callsite.
};

TEST_CASE("fault_injection", "[fault_injection]")
{
    bool fault_injection_enabled = false;
    // Verify it's disabled by default.
    REQUIRE(cxplat_fault_injection_is_enabled() == false);

    // Verify it's enabled when initialized.
    REQUIRE(cxplat_fault_injection_initialize(0) == CXPLAT_STATUS_SUCCESS);
    fault_injection_enabled = true;

    REQUIRE(cxplat_fault_injection_is_enabled() == true);

    // Verify that calling initialize again succeeds.
    REQUIRE(cxplat_fault_injection_initialize(0) == CXPLAT_STATUS_INVALID_STATE);

    // Verify that adding a module succeeds.
    REQUIRE(cxplat_fault_injection_add_module(GetModuleHandle(nullptr)) == CXPLAT_STATUS_SUCCESS);

    // Verify that adding a module again succeeds.
    REQUIRE(cxplat_fault_injection_add_module(GetModuleHandle(nullptr)) == CXPLAT_STATUS_SUCCESS);

    _on_exit _([&]() {
        if (fault_injection_enabled) {
            cxplat_fault_injection_uninitialize();
        }
    });

    // Clear the fault injection state.
    cxplat_fault_injection_reset();

    for (_fault_injection_expected_outcome state = _fault_injection_expected_outcome::ExpectFault;
         state <= _fault_injection_expected_outcome::ExpectFaultDifferentCallsite;
         state = (_fault_injection_expected_outcome)((int)state + 1)) {
        bool fault_expected;
        switch (state) {
        case _fault_injection_expected_outcome::ExpectFault:
            fault_expected = true;
            break;
        case _fault_injection_expected_outcome::DontExpectFault:
            fault_expected = false;
            break;
        case _fault_injection_expected_outcome::DontExpectFaultAfterReload:
            cxplat_fault_injection_uninitialize();
            fault_injection_enabled = false;
            REQUIRE(cxplat_fault_injection_initialize(0) == CXPLAT_STATUS_SUCCESS);
            REQUIRE(cxplat_fault_injection_add_module(GetModuleHandle(nullptr)) == CXPLAT_STATUS_SUCCESS);
            fault_injection_enabled = true;
            fault_expected = false;
            break;
        case _fault_injection_expected_outcome::ExpectFaultAfterReset:
            cxplat_fault_injection_reset();
            fault_expected = true;
            break;
        case _fault_injection_expected_outcome::ExpectFaultDifferentCallsite:
            cxplat_fault_injection_reset();
            fault_expected = true;
            REQUIRE(cxplat_fault_injection_inject_fault() == true);
            break;
        }
        REQUIRE(cxplat_fault_injection_inject_fault() == fault_expected);
    }

    // Verify that removing a module succeeds.
    REQUIRE(cxplat_fault_injection_remove_module(GetModuleHandle(nullptr)) == CXPLAT_STATUS_SUCCESS);

    // Verify that removing a module again fails.
    REQUIRE(cxplat_fault_injection_remove_module(GetModuleHandle(nullptr)) == CXPLAT_STATUS_SUCCESS);
}
