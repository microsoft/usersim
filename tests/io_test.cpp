// Copyright (c) Microsoft Corporation
// SPDX-License-Identifier: MIT

#if !defined(CMAKE_NUGET)
#include <catch2/catch_all.hpp>
#else
#include <catch2/catch.hpp>
#endif
#include "usersim/io.h"

struct _DEVICE_OBJECT
{
    int reserved;
};

TEST_CASE("IoAllocateWorkItem", "[io]")
{
    DEVICE_OBJECT device_object = {0};
    PIO_WORKITEM work_item = IoAllocateWorkItem(&device_object);
    REQUIRE(work_item != nullptr);
    IoFreeWorkItem(work_item);
}

typedef struct _work_item_context
{
    PIO_WORKITEM work_item;
    int value;
    bool free_work_item;
} work_item_context_t;

IO_WORKITEM_ROUTINE _worker_routine;
static void
_worker_routine(_In_ PDEVICE_OBJECT device_object, _In_opt_ void* context)
{
    if (context == nullptr) {
        return;
    }
    work_item_context_t* work_item_context = (work_item_context_t*)context;
    work_item_context->value++;
    if (work_item_context->free_work_item) {
        IoFreeWorkItem(work_item_context->work_item);
    }
}

TEST_CASE("IoFreeWorkItem outside of worker", "[io]")
{
    DEVICE_OBJECT device_object = {0};
    PIO_WORKITEM work_item = IoAllocateWorkItem(&device_object);
    REQUIRE(work_item != nullptr);
    work_item_context_t context{.work_item = work_item, .value = 1, .free_work_item = false};

    IoQueueWorkItem(work_item, _worker_routine, DelayedWorkQueue, &context);
    Sleep(500);
    REQUIRE(context.value == 2);

    // Verify we can queue the same work item again once it's completed the first time.
    IoQueueWorkItem(work_item, _worker_routine, DelayedWorkQueue, &context);
    Sleep(500);
    REQUIRE(context.value == 3);

    IoFreeWorkItem(work_item);
}

TEST_CASE("IoFreeWorkItem in worker", "[io]")
{
    DEVICE_OBJECT device_object = {0};
    PIO_WORKITEM work_item = IoAllocateWorkItem(&device_object);
    REQUIRE(work_item != nullptr);

    work_item_context_t context{.work_item = work_item, .value = 1, .free_work_item = true};
    IoQueueWorkItem(work_item, _worker_routine, DelayedWorkQueue, &context);
    Sleep(500);
    REQUIRE(context.value == 2);
}
