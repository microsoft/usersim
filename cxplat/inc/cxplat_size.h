// Copyright (c) Microsoft Corporation
// SPDX-License-Identifier: MIT
#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

CXPLAT_EXTERN_C_BEGIN

/**
 * @brief Supported integer types for cxplat safe arithmetic helpers.
 *
 * The second parameter is the public function-name stem, and the third is the
 * corresponding IntSafe/NtIntSafe primitive stem.
 */
#define CXPLAT_SAFE_INTEGER_TYPE_LIST(X) \
    X(size_t, size_t, SizeT)             \
    X(uint8_t, uint8_t, UInt8)           \
    X(int8_t, int8_t, Int8)              \
    X(uint16_t, uint16_t, UInt16)        \
    X(int16_t, int16_t, Int16)           \
    X(uint32_t, uint32_t, UInt32)        \
    X(int32_t, int32_t, Int32)           \
    X(uint64_t, uint64_t, UInt64)        \
    X(int64_t, int64_t, Int64)

/**
 * @brief Safe checked arithmetic helpers for the supported integer types.
 *
 * Each listed type exposes:
 * - cxplat_safe_<type>_multiply
 * - cxplat_safe_<type>_add
 * - cxplat_safe_<type>_subtract
 *
 * All helpers return CXPLAT_STATUS_SUCCESS on success and
 * CXPLAT_STATUS_ARITHMETIC_OVERFLOW when the operation would overflow or
 * underflow.
 */
#define CXPLAT_DECLARE_SAFE_INTEGER_OPERATIONS(type, name, intsafe_name)                                                  \
    _Must_inspect_result_ cxplat_status_t cxplat_safe_##name##_multiply(                                                  \
        type multiplicand, type multiplier, _Out_ _Deref_out_range_(==, multiplicand * multiplier) type * result);       \
    _Must_inspect_result_ cxplat_status_t                                                                                  \
    cxplat_safe_##name##_add(type augend, type addend, _Out_ _Deref_out_range_(==, augend + addend) type * result);      \
    _Must_inspect_result_ cxplat_status_t cxplat_safe_##name##_subtract(                                                  \
        type minuend, type subtrahend, _Out_ _Deref_out_range_(==, minuend - subtrahend) type * result);

CXPLAT_SAFE_INTEGER_TYPE_LIST(CXPLAT_DECLARE_SAFE_INTEGER_OPERATIONS)

#undef CXPLAT_DECLARE_SAFE_INTEGER_OPERATIONS

CXPLAT_EXTERN_C_END
