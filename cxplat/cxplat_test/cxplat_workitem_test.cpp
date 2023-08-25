// Copyright (c) Microsoft Corporation
// SPDX-License-Identifier: MIT

#if !defined(CMAKE_NUGET)
#include <catch2/catch_all.hpp>
#else
#include <catch2/catch.hpp>
#endif
#include "cxplat.h"
#include <windows.h>

static void
_test_work_item_routine(_Inout_opt_ void* work_item_context)
{
    int* context = (int*)work_item_context;
    REQUIRE(*context == 1);
    (*context)++;
}

TEST_CASE("queue_preemptible_work_item", "[workitem]")
{
    REQUIRE(cxplat_initialize() == CXPLAT_STATUS_SUCCESS);

    cxplat_preemptible_work_item_t* work_item = nullptr;
    int context = 1;
    REQUIRE(
        cxplat_allocate_preemptible_work_item(nullptr, &work_item, _test_work_item_routine, &context) ==
        CXPLAT_STATUS_SUCCESS);

    cxplat_queue_preemptible_work_item(work_item);
    cxplat_wait_for_preemptible_work_items_complete();
    REQUIRE(context == 2);

    cxplat_cleanup();
}

TEST_CASE("free_preemptible_work_item", "[workitem]")
{
    REQUIRE(cxplat_initialize() == CXPLAT_STATUS_SUCCESS);

    cxplat_preemptible_work_item_t* work_item = nullptr;
    int context = 1;
    REQUIRE(
        cxplat_allocate_preemptible_work_item(nullptr, &work_item, _test_work_item_routine, &context) ==
        CXPLAT_STATUS_SUCCESS);
    cxplat_free_preemptible_work_item(work_item);
    REQUIRE(context == 1);

    cxplat_cleanup();
}
