// Copyright (c) Microsoft Corporation
// SPDX-License-Identifier: MIT

#if !defined(CMAKE_NUGET)
#include <catch2/catch_all.hpp>
#else
#include <catch2/catch.hpp>
#endif

#include <limits>

#include "cxplat.h"

template <typename T>
constexpr T
_cxplat_safe_integer_min_for_subtract_failure()
{
    return std::numeric_limits<T>::is_signed ? std::numeric_limits<T>::lowest() : static_cast<T>(0);
}

#define USERSIM_DEFINE_CXPLAT_SAFE_INTEGER_TESTS(type, name, intsafe_name)                                                \
    TEST_CASE("usersim cxplat_safe_" #name "_multiply", "[cxplat][size]")                                                 \
    {                                                                                                                      \
        type result;                                                                                                       \
        REQUIRE(cxplat_safe_##name##_multiply(static_cast<type>(3), static_cast<type>(5), &result) ==                    \
                CXPLAT_STATUS_SUCCESS);                                                                                    \
        REQUIRE(result == static_cast<type>(15));                                                                          \
                                                                                                                           \
        REQUIRE(                                                                                                           \
            cxplat_safe_##name##_multiply(std::numeric_limits<type>::max(), static_cast<type>(2), &result) ==            \
            CXPLAT_STATUS_ARITHMETIC_OVERFLOW);                                                                            \
    }                                                                                                                      \
                                                                                                                           \
    TEST_CASE("usersim cxplat_safe_" #name "_add", "[cxplat][size]")                                                      \
    {                                                                                                                      \
        type result;                                                                                                       \
        REQUIRE(cxplat_safe_##name##_add(static_cast<type>(3), static_cast<type>(5), &result) ==                         \
                CXPLAT_STATUS_SUCCESS);                                                                                    \
        REQUIRE(result == static_cast<type>(8));                                                                           \
                                                                                                                           \
        REQUIRE(cxplat_safe_##name##_add(std::numeric_limits<type>::max(), static_cast<type>(1), &result) ==             \
                CXPLAT_STATUS_ARITHMETIC_OVERFLOW);                                                                        \
    }                                                                                                                      \
                                                                                                                           \
    TEST_CASE("usersim cxplat_safe_" #name "_subtract", "[cxplat][size]")                                                 \
    {                                                                                                                      \
        type result;                                                                                                       \
        REQUIRE(cxplat_safe_##name##_subtract(static_cast<type>(5), static_cast<type>(3), &result) ==                    \
                CXPLAT_STATUS_SUCCESS);                                                                                    \
        REQUIRE(result == static_cast<type>(2));                                                                           \
                                                                                                                           \
        REQUIRE(                                                                                                           \
            cxplat_safe_##name##_subtract(                                                                                 \
                _cxplat_safe_integer_min_for_subtract_failure<type>(), static_cast<type>(1), &result) ==                 \
            CXPLAT_STATUS_ARITHMETIC_OVERFLOW);                                                                            \
    }

CXPLAT_SAFE_INTEGER_TYPE_LIST(USERSIM_DEFINE_CXPLAT_SAFE_INTEGER_TESTS)

#undef USERSIM_DEFINE_CXPLAT_SAFE_INTEGER_TESTS
