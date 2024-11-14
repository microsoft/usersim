// Copyright (c) Microsoft Corporation
// SPDX-License-Identifier: MIT

#if !defined(CMAKE_NUGET)
#include <catch2/catch_all.hpp>
#else
#include <catch2/catch.hpp>
#endif
#include "cxplat.h"

TEST_CASE("processor", "[processor]")
{
    REQUIRE(cxplat_initialize() == CXPLAT_STATUS_SUCCESS);
    uint32_t maximum = cxplat_get_maximum_processor_count();
    REQUIRE(maximum > 0);
    uint32_t active = cxplat_get_active_processor_count();
    REQUIRE(active > 0);
    REQUIRE(active <= maximum);
    uint32_t current = cxplat_get_current_processor_number();
    REQUIRE(current >= 0);
    REQUIRE(current < maximum);
    cxplat_cleanup();
}

TEST_CASE("queued_spin_lock", "[processor]")
{
    REQUIRE(cxplat_initialize() == CXPLAT_STATUS_SUCCESS);
    cxplat_queue_spin_lock_t lock;
    cxplat_lock_queue_handle_t handle;
    cxplat_acquire_in_stack_queued_spin_lock(&lock, &handle);
    cxplat_release_in_stack_queued_spin_lock(&handle);
    cxplat_cleanup();
}

TEST_CASE("spin_lock", "[processor]")
{
    REQUIRE(cxplat_initialize() == CXPLAT_STATUS_SUCCESS);
    cxplat_irql_t irql;
    cxplat_spin_lock_t lock;
    REQUIRE(cxplat_get_current_irql() == PASSIVE_LEVEL);
    irql = cxplat_acquire_spin_lock(&lock);
    REQUIRE(cxplat_get_current_irql() == DISPATCH_LEVEL);
    cxplat_release_spin_lock(&lock, irql);
    REQUIRE(cxplat_get_current_irql() == PASSIVE_LEVEL);

    irql = cxplat_raise_irql(DISPATCH_LEVEL);
    REQUIRE(cxplat_get_current_irql() == DISPATCH_LEVEL);
    REQUIRE(irql == PASSIVE_LEVEL);
    cxplat_acquire_spin_lock_at_dpc_level(&lock);

    cxplat_release_spin_lock_from_dpc_level(&lock);

    cxplat_lower_irql(irql);
    REQUIRE(cxplat_get_current_irql() == PASSIVE_LEVEL);

    cxplat_cleanup();
}