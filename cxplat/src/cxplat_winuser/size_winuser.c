// Copyright (c) Microsoft Corporation
// SPDX-License-Identifier: MIT
#include "cxplat.h"
#include <intsafe.h>

_Must_inspect_result_ cxplat_status_t
cxplat_safe_size_t_multiply(
    size_t multiplicand, size_t multiplier, _Out_ _Deref_out_range_(==, multiplicand* multiplier) size_t* result)
{
    return SUCCEEDED(SizeTMult(multiplicand, multiplier, result)) ? CXPLAT_STATUS_SUCCESS
                                                                  : CXPLAT_STATUS_ARITHMETIC_OVERFLOW;
}

_Must_inspect_result_ cxplat_status_t
cxplat_safe_size_t_add(size_t augend, size_t addend, _Out_ _Deref_out_range_(==, augend + addend) size_t* result)
{
    return SUCCEEDED(SizeTAdd(augend, addend, result)) ? CXPLAT_STATUS_SUCCESS : CXPLAT_STATUS_ARITHMETIC_OVERFLOW;
}

_Must_inspect_result_ cxplat_status_t
cxplat_safe_size_t_subtract(
    size_t minuend, size_t subtrahend, _Out_ _Deref_out_range_(==, minuend - subtrahend) size_t* result)
{
    return SUCCEEDED(SizeTSub(minuend, subtrahend, result)) ? CXPLAT_STATUS_SUCCESS : CXPLAT_STATUS_ARITHMETIC_OVERFLOW;
}