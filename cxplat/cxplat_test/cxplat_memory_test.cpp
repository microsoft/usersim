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
    cxplat_pool_flags_t pool_flags = (cxplat_pool_flags_t)(CXPLAT_POOL_FLAG_NON_PAGED | CXPLAT_POOL_FLAG_UNINITIALIZED);
    uint64_t* buffer = (uint64_t*)cxplat_allocate(pool_flags, 8, TEST_TAG);
    REQUIRE(buffer != nullptr);
#ifndef NDEBUG
    REQUIRE(*buffer != 0);
#endif
    *buffer = 0;
    cxplat_free(buffer, pool_flags, TEST_TAG);

    // Try an allocation that must be initialized.
    buffer = (uint64_t*)cxplat_allocate(CXPLAT_POOL_FLAG_NON_PAGED, 8, TEST_TAG);
    REQUIRE(buffer != nullptr);
    REQUIRE(*buffer == 0);
    *buffer = 42;

    // Try a free with an unknown tag.
    cxplat_free(buffer, CXPLAT_POOL_FLAG_NON_PAGED, 0);
}

TEST_CASE("reallocate unaligned", "[memory]")
{
    // Try an allocation that need not be cache aligned.
    uint64_t* original_buffer = (uint64_t*)cxplat_allocate(CXPLAT_POOL_FLAG_NON_PAGED, 8, TEST_TAG);
    REQUIRE(original_buffer != nullptr);

    uint64_t* new_buffer = (uint64_t*)cxplat_reallocate(original_buffer, CXPLAT_POOL_FLAG_NON_PAGED, 8, 16, TEST_TAG);
    REQUIRE(new_buffer != nullptr);
    REQUIRE(new_buffer[1] == 0);

    cxplat_free(new_buffer, CXPLAT_POOL_FLAG_NON_PAGED, TEST_TAG);
}

TEST_CASE("allocate with aligned pool", "[memory]")
{
    // Try an allocation that must be cache aligned.
    cxplat_pool_flags_t pool_flags = (cxplat_pool_flags_t)(CXPLAT_POOL_FLAG_NON_PAGED | CXPLAT_POOL_FLAG_CACHE_ALIGNED);
    uint64_t* original_buffer = (uint64_t*)cxplat_allocate(pool_flags, 8, TEST_TAG);
    REQUIRE(original_buffer != nullptr);

    uint64_t* new_buffer = (uint64_t*)cxplat_reallocate(original_buffer, pool_flags, 8, 16, TEST_TAG);
    REQUIRE(new_buffer != nullptr);
    REQUIRE((((uintptr_t)new_buffer) % 64) == 0);
    REQUIRE(new_buffer[1] == 0);

    cxplat_free(new_buffer, pool_flags, TEST_TAG);
}

TEST_CASE("cxplat_free null", "[memory]") { cxplat_free(nullptr, CXPLAT_POOL_FLAG_NON_PAGED, TEST_TAG); }

TEST_CASE("cxplat_duplicate_string", "[memory]")
{
    char* string = cxplat_duplicate_string("test");
    REQUIRE(string != nullptr);
    REQUIRE(strcmp(string, "test") == 0);

    cxplat_free_string(string);
}

TEST_CASE("cxplat_duplicate_utf8_string", "[memory]")
{
    const char string[] = "test";
    cxplat_utf8_string_t source = CXPLAT_UTF8_STRING_FROM_CONST_STRING(string);
    REQUIRE(source.length == strlen(string));

    cxplat_utf8_string_t destination;
    cxplat_status_t status = cxplat_duplicate_utf8_string(&destination, &source);
    REQUIRE(status == CXPLAT_STATUS_SUCCESS);
    REQUIRE(destination.length == source.length);
    REQUIRE(memcmp(destination.value, source.value, source.length) == 0);

    cxplat_free_utf8_string(&destination);
}

TEST_CASE("cxplat_allocate_lookaside_list", "[memory]")
{
    void* lookaside;
    cxplat_status_t status = cxplat_allocate_lookaside_list(8, TEST_TAG, CXPLAT_POOL_FLAG_NON_PAGED, &lookaside);
    REQUIRE(status == CXPLAT_STATUS_SUCCESS);
    REQUIRE(lookaside != nullptr);

    void* entry = cxplat_lookaside_list_alloc(lookaside);
    REQUIRE(entry != nullptr);

    cxplat_lookaside_list_free(lookaside, entry);
    cxplat_free_lookaside_list(lookaside, TEST_TAG);
}