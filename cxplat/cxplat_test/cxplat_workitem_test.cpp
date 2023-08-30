// Copyright (c) Microsoft Corporation
// SPDX-License-Identifier: MIT

#if !defined(CMAKE_NUGET)
#include <catch2/catch_all.hpp>
#else
#include <catch2/catch.hpp>
#endif
#include "cxplat.h"
#include <windows.h>

typedef struct _work_item_context
{
    cxplat_preemptible_work_item_t* work_item;
    int value;
    bool free_work_item;
} work_item_context_t;

static void
_test_work_item_routine(_Inout_opt_ void* context)
{
    if (context == nullptr) {
        return;
    }
    work_item_context_t* work_item_context = (work_item_context_t*)context;
    work_item_context->value++;
    if (work_item_context->free_work_item) {
        cxplat_free_preemptible_work_item(work_item_context->work_item);
    }
}

TEST_CASE("queue_preemptible_work_item and free afterwards", "[workitem]")
{
    REQUIRE(cxplat_initialize() == CXPLAT_STATUS_SUCCESS);

    cxplat_preemptible_work_item_t* work_item = nullptr;
    work_item_context_t context{.value = 1, .free_work_item = false};
    REQUIRE(
        cxplat_allocate_preemptible_work_item(nullptr, &work_item, _test_work_item_routine, &context) ==
        CXPLAT_STATUS_SUCCESS);
    context.work_item = work_item;

    cxplat_queue_preemptible_work_item(work_item);
    Sleep(500);
    REQUIRE(context.value == 2);

    // Verify we can re-queue the same work item after it's completed the first time,
    // as long as we haven't yet called cxplat_wait_for_preemptible_work_items_complete().
    cxplat_queue_preemptible_work_item(work_item);
    cxplat_wait_for_preemptible_work_items_complete();
    REQUIRE(context.value == 3);

    cxplat_free_preemptible_work_item(work_item);
    cxplat_cleanup();
}

TEST_CASE("queue_preemptible_work_item and free inside", "[workitem]")
{
    REQUIRE(cxplat_initialize() == CXPLAT_STATUS_SUCCESS);

    cxplat_preemptible_work_item_t* work_item = nullptr;
    work_item_context_t context{.value = 1, .free_work_item = true};
    REQUIRE(
        cxplat_allocate_preemptible_work_item(nullptr, &work_item, _test_work_item_routine, &context) ==
        CXPLAT_STATUS_SUCCESS);
    context.work_item = work_item;

    cxplat_queue_preemptible_work_item(work_item);
    cxplat_wait_for_preemptible_work_items_complete();
    REQUIRE(context.value == 2);

    cxplat_cleanup();
}

TEST_CASE("free_preemptible_work_item", "[workitem]")
{
    REQUIRE(cxplat_initialize() == CXPLAT_STATUS_SUCCESS);

    cxplat_preemptible_work_item_t* work_item = nullptr;
    work_item_context_t context{.work_item = work_item, .value = 1, .free_work_item = false};
    REQUIRE(
        cxplat_allocate_preemptible_work_item(nullptr, &work_item, _test_work_item_routine, &context) ==
        CXPLAT_STATUS_SUCCESS);
    cxplat_free_preemptible_work_item(work_item);
    REQUIRE(context.value == 1);

    cxplat_cleanup();
}
