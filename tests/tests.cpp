// Copyright (c) Microsoft Corporation
// SPDX-License-Identifier: MIT

#define CATCH_CONFIG_MAIN
#if !defined(CMAKE_NUGET)
#include <catch2/catch_all.hpp>
#else
#include <catch2/catch.hpp>
#endif
#include "usersim/ex.h"
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

TEST_CASE("mdl", "[mm]")
{
    PHYSICAL_ADDRESS start_address{.QuadPart = 0};
    PHYSICAL_ADDRESS end_address{.QuadPart = -1};
    PHYSICAL_ADDRESS page_size{.QuadPart = PAGE_SIZE};
    size_t length = 256;
    MDL* mdl =
        MmAllocatePagesForMdlEx(start_address, end_address, page_size, length, MmCached, MM_ALLOCATE_FULLY_REQUIRED);
    REQUIRE(mdl != nullptr);

    void* base_address = MmGetSystemAddressForMdlSafe(mdl, NormalPagePriority);
    REQUIRE(base_address != nullptr);

    REQUIRE(MmGetMdlByteCount(mdl) == length);

    MmUnmapLockedPages(base_address, mdl);
    MmFreePagesFromMdl(mdl);
    ExFreePool(mdl);
}