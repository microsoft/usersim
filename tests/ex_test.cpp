// Copyright (c) Microsoft Corporation
// SPDX-License-Identifier: MIT

#if !defined(CMAKE_NUGET)
#include <catch2/catch_all.hpp>
#else
#include <catch2/catch.hpp>
#endif
#include "usersim/ex.h"
#include "cxplat_winuser.h"

#include <thread>

TEST_CASE("ExAllocatePool", "[ex]")
{
    // Try an allocation that need not be cache aligned.
    uint64_t* buffer = (uint64_t*)ExAllocatePoolUninitialized(NonPagedPoolNx, 8, 'tset');
    REQUIRE(buffer != nullptr);
#ifndef NDEBUG
    REQUIRE(*buffer != 0);
#endif
    *buffer = 0;
    ExFreePool(buffer);

    // Try an allocation that must be cache aligned.
    buffer = (uint64_t*)ExAllocatePoolUninitialized(NonPagedPoolNxCacheAligned, 8, 'tset');
    REQUIRE(buffer != nullptr);
#ifndef NDEBUG
    REQUIRE(*buffer != 0);
#endif
    REQUIRE((((uintptr_t)buffer) % 64) == 0);
    *buffer = 0;
    ExFreePool(buffer);

    buffer = (uint64_t*)ExAllocatePoolWithTag(NonPagedPoolNx, 8, 'tset');
    REQUIRE(buffer != nullptr);
    REQUIRE(*buffer == 0);
    *buffer = 42;
    ExFreePool(buffer);
}

TEST_CASE("ExAllocatePool2", "[ex]")
{
    // Try an allocation that need not be cache aligned.
    uint64_t* buffer = (uint64_t*)ExAllocatePool2(CXPLAT_POOL_FLAG_NON_PAGED | CXPLAT_POOL_FLAG_UNINITIALIZED, 8, 'tset');
    REQUIRE(buffer != nullptr);
#ifndef NDEBUG
    REQUIRE(*buffer != 0);
#endif
    *buffer = 0;
    ExFreePool(buffer);

    // Try an allocation that must be cache aligned.
    buffer = (uint64_t*)ExAllocatePool2(
        CXPLAT_POOL_FLAG_NON_PAGED | CXPLAT_POOL_FLAG_UNINITIALIZED | CXPLAT_POOL_FLAG_CACHE_ALIGNED, 8, 'tset');
    REQUIRE(buffer != nullptr);
#ifndef NDEBUG
    REQUIRE(*buffer != 0);
#endif
    REQUIRE((((uintptr_t)buffer) % 64) == 0);
    *buffer = 0;
    ExFreePool(buffer);

    buffer = (uint64_t*)ExAllocatePool2(CXPLAT_POOL_FLAG_NON_PAGED, 8, 'tset');
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
                e.what(),
                "*** STOP 0x000000c2 (0x0000000000000046,0x0000000000000000,0x0000000000000000,0x0000000000000000)") ==
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
                e.what(),
                "*** STOP 0x000000c2 (0x0000000000000046,0x0000000000000000,0x0000000000000000,0x0000000000000000)") ==
            0);
    }
}

TEST_CASE("ExRaiseAccessViolation", "[ex]")
{
    try {
        ExRaiseAccessViolationCPP();
        REQUIRE(FALSE);
    } catch (std::exception e) {
        PCSTR message = e.what();
        PCSTR ex = message + strlen("Exception: ");
        int64_t code = _atoi64(ex);
        REQUIRE(code == STATUS_ACCESS_VIOLATION);
    }
}

TEST_CASE("ExRaiseDatatypeMisalignment", "[ex]")
{
    try {
        ExRaiseDatatypeMisalignmentCPP();
        REQUIRE(FALSE);
    } catch (std::exception e) {
        PCSTR message = e.what();
        PCSTR ex = message + strlen("Exception: ");
        int64_t code = _atoi64(ex);
        REQUIRE(code == STATUS_DATATYPE_MISALIGNMENT);
    }
}

TEST_CASE("EX_RUNDOWN_REF", "[ex]")
{
    EX_RUNDOWN_REF ref;
    std::atomic<bool> thread_completed = false;
    ExInitializeRundownProtection(&ref);

    // Acquire before rundown is initiated.
    // Acquire the first rundown protection reference.
    REQUIRE(ExAcquireRundownProtection(&ref));

    // Acquire the second rundown protection reference.
    REQUIRE(ExAcquireRundownProtection(&ref));

    // Wait for the rundown protection to be released.
    std::thread thread([&]() {
        // Wait for the rundown protection to be released.
        ExWaitForRundownProtectionRelease(&ref);
        thread_completed = true;
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Thread should be waiting for the rundown protection to be released.
    REQUIRE(!thread_completed);

    // Acquire after rundown is initiated.
    // Future acquire of the rundown protection should fail.
    REQUIRE(!ExAcquireRundownProtection(&ref));

    // Release the second rundown protection reference.
    ExReleaseRundownProtection(&ref);

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Thread should be waiting for the rundown protection to be released.
    REQUIRE(!thread_completed);

    // Release the first rundown protection reference.
    ExReleaseRundownProtection(&ref);

    // Thread should have completed.
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    thread.join();

    // Thread should be waiting for the rundown protection to be released.
    REQUIRE(thread_completed);

    // Acquire after rundown is completed.

    // Future acquire of the rundown protection should fail.
    REQUIRE(!ExAcquireRundownProtection(&ref));
}

TEST_CASE("EX_RUNDOWN_REF_CACHE_AWARE", "[ex]")
{
    EX_RUNDOWN_REF_CACHE_AWARE* ref = ExAllocateCacheAwareRundownProtection(NonPagedPoolNx, 'tset');
    std::atomic<bool> thread_completed = false;
    REQUIRE(ref != nullptr);

    // Acquire before rundown is initiated.
    // Acquire the first rundown protection reference.
    REQUIRE(ExAcquireRundownProtectionCacheAware(ref));

    // Acquire the second rundown protection reference.
    REQUIRE(ExAcquireRundownProtectionCacheAware(ref));

    // Wait for the rundown protection to be released.
    std::thread thread([&]() {
        // Wait for the rundown protection to be released.
        ExWaitForRundownProtectionRelease(ref);
        thread_completed = true;
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Thread should be waiting for the rundown protection to be released.
    REQUIRE(!thread_completed);

    // Acquire after rundown is initiated.
    // Future acquire of the rundown protection should fail.
    REQUIRE(!ExAcquireRundownProtectionCacheAware(ref));

    // Release the second rundown protection reference.
    ExReleaseRundownProtectionCacheAware(ref);

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Thread should be waiting for the rundown protection to be released.
    REQUIRE(!thread_completed);

    // Release the first rundown protection reference.
    ExReleaseRundownProtectionCacheAware(ref);

    // Thread should have completed.
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    thread.join();
    REQUIRE(thread_completed);

    // Acquire after rundown is completed.

    // Future acquire of the rundown protection should fail.
    REQUIRE(!ExAcquireRundownProtectionCacheAware(ref));

    ExFreeCacheAwareRundownProtection(ref);
}