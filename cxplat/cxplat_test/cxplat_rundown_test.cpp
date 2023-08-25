// Copyright (c) Microsoft Corporation
// SPDX-License-Identifier: MIT

#if !defined(CMAKE_NUGET)
#include <catch2/catch_all.hpp>
#else
#include <catch2/catch.hpp>
#endif
#include "cxplat.h"
#include <windows.h>

TEST_CASE("rundown_protection", "[rundown]")
{
    cxplat_rundown_reference_t rundown_reference;
    cxplat_initialize_rundown_protection(&rundown_reference);
    cxplat_acquire_rundown_protection(&rundown_reference);
    cxplat_release_rundown_protection(&rundown_reference);
    cxplat_wait_for_rundown_protection_release(&rundown_reference);

    cxplat_reinitialize_rundown_protection(&rundown_reference);
    cxplat_acquire_rundown_protection(&rundown_reference);
    cxplat_release_rundown_protection(&rundown_reference);
    cxplat_wait_for_rundown_protection_release(&rundown_reference);
}
