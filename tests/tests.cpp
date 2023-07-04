// Copyright (c) Microsoft Corporation
// SPDX-License-Identifier: MIT

#define CATCH_CONFIG_MAIN
#if !defined(CMAKE_NUGET)
#include <catch2/catch_all.hpp>
#else
#include <catch2/catch.hpp>
#endif
#include "usersim/ex.h"
#include "usersim/io.h"
#include "usersim/ke.h"
#include "usersim/mm.h"

TEST_CASE("DriverEntry", "[wdf]")
{
    HMODULE module = LoadLibraryW(L"sample.dll");
    REQUIRE(module != nullptr);

    FreeLibrary(module);
}

TEST_CASE("irql", "[ke]")
{
    REQUIRE(KeGetCurrentIrql() == PASSIVE_LEVEL);

    KIRQL old_irql;
    KeRaiseIrql(DISPATCH_LEVEL, &old_irql);
    REQUIRE(old_irql == PASSIVE_LEVEL);
    REQUIRE(KeGetCurrentIrql() == DISPATCH_LEVEL);

    KeRaiseIrql(DISPATCH_LEVEL, &old_irql);
    REQUIRE(old_irql == DISPATCH_LEVEL);
    REQUIRE(KeGetCurrentIrql() == DISPATCH_LEVEL);

    KeLowerIrql(DISPATCH_LEVEL);
    REQUIRE(KeGetCurrentIrql() == DISPATCH_LEVEL);

    KeLowerIrql(PASSIVE_LEVEL);
    REQUIRE(KeGetCurrentIrql() == PASSIVE_LEVEL);

    old_irql = KeRaiseIrqlToDpcLevel();
    REQUIRE(old_irql == PASSIVE_LEVEL);
    REQUIRE(KeGetCurrentIrql() == DISPATCH_LEVEL);

    KeLowerIrql(old_irql);
    REQUIRE(KeGetCurrentIrql() == PASSIVE_LEVEL);
}

TEST_CASE("spin lock", "[ke]")
{
    KSPIN_LOCK lock;
    KeInitializeSpinLock(&lock);
    REQUIRE(KeGetCurrentIrql() == PASSIVE_LEVEL);

    // Test KeAcquireSpinLock.
    KIRQL old_irql;
    KeAcquireSpinLock(&lock, &old_irql);
    REQUIRE(old_irql == PASSIVE_LEVEL);
    REQUIRE(KeGetCurrentIrql() == DISPATCH_LEVEL);

    KeReleaseSpinLock(&lock, old_irql);
    REQUIRE(KeGetCurrentIrql() == PASSIVE_LEVEL);

    // Test KeAcquireSpinLockRaiseToDpc.
    old_irql = KeAcquireSpinLockRaiseToDpc(&lock);
    REQUIRE(old_irql == PASSIVE_LEVEL);
    REQUIRE(KeGetCurrentIrql() == DISPATCH_LEVEL);

    KeReleaseSpinLock(&lock, old_irql);
    REQUIRE(KeGetCurrentIrql() == PASSIVE_LEVEL);

    // Test KeAcquireSpinLockAtDpcLevel.
    old_irql = KeRaiseIrqlToDpcLevel();
    KeAcquireSpinLockAtDpcLevel(&lock);
    KeReleaseSpinLockFromDpcLevel(&lock);
    KeLowerIrql(old_irql);
}

TEST_CASE("MmAllocatePagesForMdlEx", "[mm]")
{
    PHYSICAL_ADDRESS start_address{.QuadPart = 0};
    PHYSICAL_ADDRESS end_address{.QuadPart = -1};
    PHYSICAL_ADDRESS page_size{.QuadPart = PAGE_SIZE};
    const size_t byte_count = 256;
    MDL* mdl = MmAllocatePagesForMdlEx(
        start_address, end_address, page_size, byte_count, MmCached, MM_ALLOCATE_FULLY_REQUIRED);
    REQUIRE(mdl != nullptr);

    void* base_address = MmGetSystemAddressForMdlSafe(mdl, NormalPagePriority);
    REQUIRE(base_address != nullptr);

    REQUIRE(MmGetMdlByteCount(mdl) == byte_count);
    REQUIRE(MmGetMdlByteOffset(mdl) == 0);

    MmUnmapLockedPages(base_address, mdl);
    MmFreePagesFromMdl(mdl);
    ExFreePool(mdl);
}

TEST_CASE("IoAllocateMdl", "[mm]")
{
    const size_t byte_count = 256;
    ULONG tag = 'tset';
    void* buffer = ExAllocatePoolWithTag(NonPagedPool, byte_count, tag);
    REQUIRE(buffer != nullptr);

    MDL* mdl = IoAllocateMdl(buffer, byte_count, FALSE, FALSE, nullptr);
    REQUIRE(mdl != nullptr);

    MmBuildMdlForNonPagedPool(mdl);

    void* base_address = MmGetSystemAddressForMdlSafe(mdl, NormalPagePriority);
    REQUIRE(base_address == buffer);

    REQUIRE(MmGetMdlByteCount(mdl) == byte_count);

    IoFreeMdl(mdl);
    ExFreePoolWithTag(buffer, tag);
}

TEST_CASE("processor count", "[ke]")
{
    ULONG count = KeQueryMaximumProcessorCount();
    REQUIRE(count > 0);

    REQUIRE(KeQueryMaximumProcessorCountEx(ALL_PROCESSOR_GROUPS) == count);
    REQUIRE(KeQueryActiveProcessorCount() == count);
    REQUIRE(KeQueryActiveProcessorCountEx(ALL_PROCESSOR_GROUPS) == count);
}

TEST_CASE("semaphore", "[ke]")
{
    REQUIRE(usersim_platform_initiate() == STATUS_SUCCESS);
    KSEMAPHORE semaphore;

    // Create a semaphore that is ready to be signaled twice.
    KeInitializeSemaphore(&semaphore, 2, 2);

    // Verify that it is signaled.
    REQUIRE(KeReadStateSemaphore(&semaphore) != 0);

    // Verify that it is still signaled after calling KeReadStateSemaphore.
    REQUIRE(KeReadStateSemaphore(&semaphore) != 0);

    // Verify we can take a reference.
    LARGE_INTEGER timeout = {0};
    REQUIRE(KeWaitForSingleObject(&semaphore, Executive, KernelMode, TRUE, &timeout) == STATUS_SUCCESS);

    // Verify that it is still signaled, since the count should now be 1.
    REQUIRE(KeReadStateSemaphore(&semaphore) != 0);

    // Verify we can take a second reference.
    REQUIRE(KeWaitForSingleObject(&semaphore, Executive, KernelMode, TRUE, &timeout) == STATUS_SUCCESS);

    // The semaphore should now be not-signaled.
    REQUIRE(KeReadStateSemaphore(&semaphore) == 0);

    // Release one reference.
    KeReleaseSemaphore(&semaphore, 1, 1, FALSE);

    // It should now be signaled again.
    REQUIRE(KeReadStateSemaphore(&semaphore) != 0);

    // Release a second reference.
    KeReleaseSemaphore(&semaphore, 1, 1, FALSE);

    // It should still be signaled.
    REQUIRE(KeReadStateSemaphore(&semaphore) != 0);

    usersim_platform_terminate();
}

TEST_CASE("threads", "[ke]")
{
    PKTHREAD thread = KeGetCurrentThread();
    REQUIRE(thread != nullptr);

    ULONG processor_count = KeQueryActiveProcessorCount();
    REQUIRE(processor_count > 0);

    KAFFINITY new_affinity = (1 << processor_count) - 1;
    KAFFINITY old_affinity = KeSetSystemAffinityThreadEx(new_affinity);
    REQUIRE(old_affinity != 0);

    KeRevertToUserAffinityThreadEx(old_affinity);
}

void
dpc_routine(
    _In_ PRKDPC dpc, _In_opt_ void* deferred_context, _In_opt_ void* system_argument1, _In_opt_ void* system_argument2)
{
    uint64_t argument1 = (uintptr_t)system_argument1;
    uint64_t argument2 = (uintptr_t)system_argument2;
    (*(uint64_t*)deferred_context) += argument1 + argument2;
}

TEST_CASE("dpcs", "[ke]")
{
    REQUIRE(usersim_platform_initiate() == STATUS_SUCCESS);

    uint64_t context = 1;
    KDPC dpc;
    KeInitializeDpc(&dpc, dpc_routine, &context);
    REQUIRE(KeRemoveQueueDpc(&dpc) == FALSE);
    KeSetTargetProcessorDpc(&dpc, 0);

    REQUIRE(KeInsertQueueDpc(&dpc, (void*)(uintptr_t)0, (void*)(uintptr_t)1) == TRUE);
    REQUIRE(KeRemoveQueueDpc(&dpc) == TRUE);
    REQUIRE(KeRemoveQueueDpc(&dpc) == FALSE);

    REQUIRE(KeInsertQueueDpc(&dpc, (void*)(uintptr_t)1, (void*)(uintptr_t)2) == TRUE);
    REQUIRE(KeInsertQueueDpc(&dpc, (void*)(uintptr_t)2, (void*)(uintptr_t)3) == FALSE);
    KeFlushQueuedDpcs();
    REQUIRE(context == 1 + 1 + 2);

    usersim_platform_terminate();
}

TEST_CASE("KeBugCheck", "[ke]")
{
    try {
        KeBugCheckCPP(17);
        REQUIRE(FALSE);
    } catch (std::exception e) {
        REQUIRE(
            strcmp(
                e.what(), "*** STOP 0x000011 (0x00000000000000,0x00000000000000,0x00000000000000,0x00000000000000)") ==
            0);
    }
}

TEST_CASE("KeBugCheckEx", "[ke]")
{
    try {
        KeBugCheckExCPP(17, 1, 2, 3, 4);
        REQUIRE(FALSE);
    } catch (std::exception e) {
        REQUIRE(
            strcmp(
                e.what(), "*** STOP 0x000011 (0x00000000000001,0x00000000000002,0x00000000000003,0x00000000000004)") ==
            0);
    }
}

TEST_CASE("ExAllocatePool", "[ex]")
{
    uint64_t* buffer = (uint64_t*)ExAllocatePoolUninitialized(NonPagedPool, 8, 'tset');
    REQUIRE(buffer != nullptr);
    REQUIRE(*buffer != 0);
    *buffer = 0;
    ExFreePool(buffer);

    buffer = (uint64_t*)ExAllocatePoolWithTag(NonPagedPool, 8, 'tset');
    REQUIRE(buffer != nullptr);
    REQUIRE(*buffer == 0);
    *buffer = 42;
    ExFreePool(buffer);
}

TEST_CASE("ExFreePool null", "[ex]")
{
    try {
        ExFreePoolCPP(nullptr);
        REQUIRE(FALSE);
    } catch (std::exception e) {
        REQUIRE(
            strcmp(
                e.what(), "*** STOP 0x0000c2 (0x00000000000046,0x00000000000000,0x00000000000000,0x00000000000000)") ==
            0);
    } catch (...) {
        REQUIRE(FALSE);
    }

    try {
        ExFreePoolWithTagCPP(nullptr, 'tset');
        REQUIRE(FALSE);
    } catch (std::exception e) {
        REQUIRE(
            strcmp(
                e.what(), "*** STOP 0x0000c2 (0x00000000000046,0x00000000000000,0x00000000000000,0x00000000000000)") ==
            0);
    }
}