// Copyright (c) Microsoft Corporation
// SPDX-License-Identifier: MIT

#if !defined(CMAKE_NUGET)
#include <catch2/catch_all.hpp>
#else
#include <catch2/catch.hpp>
#endif
#include "usersim/ex.h"
#include "usersim/io.h"
#include "usersim/mm.h"

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

struct _ex_pool_free_functor
{
    void
    operator()(void* buffer)
    {
        ExFreePool(buffer);
    }
};

struct _io_mdl_free_functor
{
    void
    operator()(void* mdl)
    {
        IoFreeMdl((MDL*)mdl);
    }
};

TEST_CASE("IoBuildPartialMdl", "[io]")
{
    const ULONG buffer_size = 8192;    // 2 pages
    const ULONG partial_offset = 1024; // Offset into buffer
    const ULONG partial_size = 2048;   // Partial size
    ULONG tag = 'tset';

    // Allocate buffer using ExAllocatePoolWithTag.
    std::unique_ptr<void, _ex_pool_free_functor> buffer(ExAllocatePoolWithTag(NonPagedPoolNx, buffer_size, tag));
    REQUIRE(buffer != nullptr);

    // Fill buffer with test data.
    memset(buffer.get(), 0xAA, buffer_size);

    // Allocate source MDL.
    std::unique_ptr<MDL, _io_mdl_free_functor> source_mdl(
        IoAllocateMdl(buffer.get(), buffer_size, FALSE, FALSE, nullptr));
    REQUIRE(source_mdl != nullptr);

    // Allocate target MDL.
    std::unique_ptr<MDL, _io_mdl_free_functor> target_mdl(IoAllocateMdl(nullptr, 0, FALSE, FALSE, nullptr));
    REQUIRE(target_mdl != nullptr);

    // Calculate virtual address for partial MDL.
    PVOID partial_address = (char*)buffer.get() + partial_offset;

    // Test building partial MDL with explicit length.
    IoBuildPartialMdl(source_mdl.get(), target_mdl.get(), partial_address, partial_size);

    // Calculate expected values.
    void* expected_start_va = (void*)((ULONG_PTR)partial_address & ~(PAGE_SIZE - 1));
    ULONG expected_byte_offset = (ULONG)((ULONG_PTR)partial_address & (PAGE_SIZE - 1));

    // Verify target MDL properties.
    REQUIRE(target_mdl->start_va == expected_start_va);
    REQUIRE(target_mdl->byte_count == partial_size);
    REQUIRE(target_mdl->byte_offset == expected_byte_offset);
    REQUIRE(target_mdl->flags == source_mdl->flags);
}

TEST_CASE("IoBuildPartialMdl no length", "[io]")
{
    const ULONG buffer_size = 8192;    // 2 pages
    const ULONG partial_offset = 1024; // Offset into buffer
    ULONG tag = 'tset';

    // Allocate buffer using ExAllocatePoolWithTag.
    std::unique_ptr<void, _ex_pool_free_functor> buffer(ExAllocatePoolWithTag(NonPagedPoolNx, buffer_size, tag));
    REQUIRE(buffer != nullptr);

    // Allocate source MDL.
    std::unique_ptr<MDL, _io_mdl_free_functor> source_mdl(
        IoAllocateMdl(buffer.get(), buffer_size, FALSE, FALSE, nullptr));
    REQUIRE(source_mdl != nullptr);

    // Allocate target MDL.
    std::unique_ptr<MDL, _io_mdl_free_functor> target_mdl(IoAllocateMdl(nullptr, 0, FALSE, FALSE, nullptr));
    REQUIRE(target_mdl != nullptr);

    // Calculate virtual address for partial MDL.
    PVOID partial_address = (char*)buffer.get() + partial_offset;

    // Test building partial MDL with no explicit length (0).
    IoBuildPartialMdl(source_mdl.get(), target_mdl.get(), partial_address, 0);

    // Calculate expected values.
    void* expected_start_va = (void*)((ULONG_PTR)partial_address & ~(PAGE_SIZE - 1));
    ULONG expected_byte_offset = (ULONG)((ULONG_PTR)partial_address & (PAGE_SIZE - 1));

    // Verify target MDL properties - should use remaining bytes from source MDL
    REQUIRE(target_mdl->start_va == expected_start_va);
    REQUIRE(target_mdl->byte_count == buffer_size - partial_offset);
    REQUIRE(target_mdl->byte_offset == expected_byte_offset);
    REQUIRE(target_mdl->flags == source_mdl->flags);
}
