// Copyright (c) Microsoft Corporation
// SPDX-License-Identifier: MIT

#define CATCH_CONFIG_MAIN
#if !defined(CMAKE_NUGET)
#include <catch2/catch_all.hpp>
#else
#include <catch2/catch.hpp>
#endif
#include "cxplat.h"

#define TEST_TAG 'tset'

TEST_CASE("allocate", "[memory]")
{
    // Try an allocation that need not be cache aligned.
    uint64_t* buffer = (uint64_t*)cxplat_allocate_with_tag(CxPlatNonPagedPoolNx, 8, TEST_TAG, false);
    REQUIRE(buffer != nullptr);
    REQUIRE(*buffer != 0);
    *buffer = 0;
    cxplat_free(buffer);

    // Try an allocation that must be cache aligned.
    buffer = (uint64_t*)cxplat_allocate_with_tag(CxPlatNonPagedPoolNxCacheAligned, 8, TEST_TAG, false);
    REQUIRE(buffer != nullptr);
    REQUIRE(*buffer != 0);
    REQUIRE((((uintptr_t)buffer) % 64) == 0);
    *buffer = 0;
    cxplat_free(buffer);

    // Try an allocation that must be initialized.
    buffer = (uint64_t*)cxplat_allocate_with_tag(CxPlatNonPagedPoolNx, 8, TEST_TAG, true);
    REQUIRE(buffer != nullptr);
    REQUIRE(*buffer == 0);
    *buffer = 42;
    cxplat_free(buffer);
}

TEST_CASE("reallocate unaligned", "[memory]")
{
    // Try an allocation that need not be cache aligned.
    uint64_t* original_buffer = (uint64_t*)cxplat_allocate_with_tag(CxPlatNonPagedPoolNx, 8, TEST_TAG, true);
    REQUIRE(original_buffer != nullptr);

    uint64_t* new_buffer = (uint64_t*)cxplat_reallocate(original_buffer, 8, 16);
    REQUIRE(new_buffer != nullptr);
    REQUIRE(new_buffer[1] == 0);

    cxplat_free(new_buffer);
}

TEST_CASE("reallocate aligned", "[memory]")
{
    // Try an allocation that must be cache aligned.
    uint64_t* original_buffer = (uint64_t*)cxplat_allocate_with_tag(CxPlatNonPagedPoolNxCacheAligned, 8, TEST_TAG, true);
    REQUIRE(original_buffer != nullptr);

    uint64_t* new_buffer = (uint64_t*)cxplat_reallocate(original_buffer, 8, 16);
    REQUIRE(new_buffer != nullptr);
    REQUIRE((((uintptr_t)new_buffer) % 64) == 0);
    REQUIRE(new_buffer[1] == 0);

    cxplat_free(new_buffer);
}

TEST_CASE("cxplat_free null", "[ex]") { cxplat_free(nullptr); }