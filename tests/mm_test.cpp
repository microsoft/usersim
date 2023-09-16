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

struct _mm_mdl_free_functor
{
    void
    operator()(void* mdl)
    {
        MmFreePagesFromMdl((MDL*)mdl);
        ExFreePool(mdl);
    }
};

TEST_CASE("MmAllocatePagesForMdlEx", "[mm]")
{
    PHYSICAL_ADDRESS start_address{.QuadPart = 0};
    PHYSICAL_ADDRESS end_address{.QuadPart = -1};
    PHYSICAL_ADDRESS page_size{.QuadPart = PAGE_SIZE};
    const size_t byte_count = 256;
    std::unique_ptr<MDL, _mm_mdl_free_functor> mdl(MmAllocatePagesForMdlEx(
        start_address, end_address, page_size, byte_count, MmCached, MM_ALLOCATE_FULLY_REQUIRED));
    REQUIRE(mdl != nullptr);

    void* base_address = MmGetSystemAddressForMdlSafe(mdl.get(), NormalPagePriority);
    REQUIRE(base_address != nullptr);

    REQUIRE(MmGetMdlByteCount(mdl.get()) == byte_count);
    REQUIRE(MmGetMdlByteOffset(mdl.get()) == 0);

    MmUnmapLockedPages(base_address, mdl.get());
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

TEST_CASE("IoAllocateMdl", "[mm]")
{
    const size_t byte_count = 256;
    ULONG tag = 'tset';
    std::unique_ptr<void, _ex_pool_free_functor> buffer(ExAllocatePoolWithTag(NonPagedPool, byte_count, tag));
    REQUIRE(buffer != nullptr);

    std::unique_ptr<MDL, _io_mdl_free_functor> mdl(IoAllocateMdl(buffer.get(), byte_count, FALSE, FALSE, nullptr));
    REQUIRE(mdl != nullptr);

    MmBuildMdlForNonPagedPool(mdl.get());

    void* base_address = MmGetSystemAddressForMdlSafe(mdl.get(), NormalPagePriority);
    REQUIRE(base_address == buffer.get());

    REQUIRE(MmGetMdlByteCount(mdl.get()) == byte_count);
}

ULONG
test_probe_for_read(_In_ const volatile void* address, SIZE_T length, ULONG alignment)
{
    try {
        ProbeForReadCPP(address, length, alignment);
        return 0;
    } catch (std::exception e) {
        PCSTR message = e.what();
        PCSTR ex = message + strlen("Exception: ");
        int64_t code = _atoi64(ex);
        return (ULONG)code;
    }
}

TEST_CASE("ProbeForRead", "[mm]")
{
    // Verify a null pointer access results in STATUS_ACCESS_VIOLATION.
    REQUIRE(test_probe_for_read(nullptr, 8, 8) == STATUS_ACCESS_VIOLATION);

    // Verify a valid access does nothing.
    uint64_t x = 0;
    REQUIRE(test_probe_for_read(&x, sizeof(x), sizeof(x)) == STATUS_SUCCESS);

    // Verify a misaligned pointer access results in STATUS_DATATYPE_MISALIGNMENT.
    REQUIRE(test_probe_for_read(((char*)&x) + 4, sizeof(uint32_t), sizeof(uint64_t)) == STATUS_DATATYPE_MISALIGNMENT);

    // Verify a read past end of memory results in STATUS_ACCESS_VIOLATION.
    REQUIRE(test_probe_for_read(&x, 65536, 8) == STATUS_ACCESS_VIOLATION);
}

ULONG
test_probe_for_write(_In_ volatile void* address, SIZE_T length, ULONG alignment)
{
    try {
        ProbeForWriteCPP(address, length, alignment);
        return 0;
    } catch (std::exception e) {
        PCSTR message = e.what();
        PCSTR ex = message + strlen("Exception: ");
        int64_t code = _atoi64(ex);
        return (ULONG)code;
    }
}

TEST_CASE("ProbeForWrite", "[mm]")
{
    // Verify a null pointer access results in STATUS_ACCESS_VIOLATION.
    REQUIRE(test_probe_for_write(nullptr, 8, 8) == STATUS_ACCESS_VIOLATION);

    // Verify a valid access does nothing.
    uint64_t x = 0;
    REQUIRE(test_probe_for_write(&x, sizeof(x), sizeof(x)) == STATUS_SUCCESS);

    // Verify a misaligned pointer access results in STATUS_DATATYPE_MISALIGNMENT.
    REQUIRE(test_probe_for_write(((char*)&x) + 4, sizeof(uint32_t), sizeof(uint64_t)) == STATUS_DATATYPE_MISALIGNMENT);

    // Verify a write past end of memory results in STATUS_ACCESS_VIOLATION.
    REQUIRE(test_probe_for_write(&x, 65536, 8) == STATUS_ACCESS_VIOLATION);
}
