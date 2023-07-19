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