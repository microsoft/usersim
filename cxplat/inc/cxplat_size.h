// Copyright (c) Microsoft Corporation
// SPDX-License-Identifier: MIT
#pragma once

#include <stdbool.h>

CXPLAT_EXTERN_C_BEGIN

/**
 * @brief Multiplies one value of type size_t by another and check for
 *   overflow.
 * @param[in] multiplicand The value to be multiplied by multiplier.
 * @param[in] multiplier The value by which to multiply multiplicand.
 * @param[out] result A pointer to the result.
 * @retval CXPLAT_STATUS_SUCCESS The operation was successful.
 * @retval CXPLAT_STATUS_ARITHMETIC_OVERFLOW Multiplication overflowed.
 */
_Must_inspect_result_ cxplat_status_t
cxplat_safe_size_t_multiply(
    size_t multiplicand, size_t multiplier, _Out_ _Deref_out_range_(==, multiplicand* multiplier) size_t* result);

/**
 * @brief Add one value of type size_t by another and check for
 *   overflow.
 * @param[in] augend The value to be added by addend.
 * @param[in] addend The value add to augend.
 * @param[out] result A pointer to the result.
 * @retval CXPLAT_STATUS_SUCCESS The operation was successful.
 * @retval CXPLAT_STATUS_ARITHMETIC_OVERFLOW Addition overflowed.
 */
_Must_inspect_result_ cxplat_status_t
cxplat_safe_size_t_add(size_t augend, size_t addend, _Out_ _Deref_out_range_(==, augend + addend) size_t* result);

/**
 * @brief Subtract one value of type size_t from another and check for
 *   overflow or underflow.
 * @param[in] minuend The value from which subtrahend is subtracted.
 * @param[in] subtrahend The value subtract from minuend.
 * @param[out] result A pointer to the result.
 * @retval CXPLAT_STATUS_SUCCESS The operation was successful.
 * @retval CXPLAT_STATUS_ARITHMETIC_OVERFLOW Addition overflowed or underflowed.
 */
_Must_inspect_result_ cxplat_status_t
cxplat_safe_size_t_subtract(
    size_t minuend, size_t subtrahend, _Out_ _Deref_out_range_(==, minuend - subtrahend) size_t* result);

CXPLAT_EXTERN_C_END
