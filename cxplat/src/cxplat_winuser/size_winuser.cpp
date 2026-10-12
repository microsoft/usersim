// Copyright (c) Microsoft Corporation
// SPDX-License-Identifier: MIT
#include "cxplat.h"

#define ENABLE_INTSAFE_SIGNED_FUNCTIONS
#include <intsafe.h>

#define CXPLAT_INTSAFE_OPERATION(stem, operation) CXPLAT_INTSAFE_OPERATION_(stem, operation)
#define CXPLAT_INTSAFE_OPERATION_(stem, operation) stem##operation

#define CXPLAT_DEFINE_SAFE_INTEGER_OPERATIONS(type, name, intsafe_name)                                                   \
    _Must_inspect_result_ cxplat_status_t cxplat_safe_##name##_multiply(                                                  \
        type multiplicand, type multiplier, _Out_ _Deref_out_range_(==, multiplicand * multiplier) type * result)        \
    {                                                                                                                      \
        return SUCCEEDED(CXPLAT_INTSAFE_OPERATION(intsafe_name, Mult)(multiplicand, multiplier, result))                  \
                   ? CXPLAT_STATUS_SUCCESS                                                                                \
                   : CXPLAT_STATUS_ARITHMETIC_OVERFLOW;                                                                   \
    }                                                                                                                      \
                                                                                                                           \
    _Must_inspect_result_ cxplat_status_t                                                                                  \
    cxplat_safe_##name##_add(type augend, type addend, _Out_ _Deref_out_range_(==, augend + addend) type * result)       \
    {                                                                                                                      \
        return SUCCEEDED(CXPLAT_INTSAFE_OPERATION(intsafe_name, Add)(augend, addend, result))                             \
                   ? CXPLAT_STATUS_SUCCESS                                                                                \
                   : CXPLAT_STATUS_ARITHMETIC_OVERFLOW;                                                                   \
    }                                                                                                                      \
                                                                                                                           \
    _Must_inspect_result_ cxplat_status_t cxplat_safe_##name##_subtract(                                                  \
        type minuend, type subtrahend, _Out_ _Deref_out_range_(==, minuend - subtrahend) type * result)                  \
    {                                                                                                                      \
        return SUCCEEDED(CXPLAT_INTSAFE_OPERATION(intsafe_name, Sub)(minuend, subtrahend, result))                        \
                   ? CXPLAT_STATUS_SUCCESS                                                                                \
                   : CXPLAT_STATUS_ARITHMETIC_OVERFLOW;                                                                   \
    }

CXPLAT_SAFE_INTEGER_TYPE_LIST(CXPLAT_DEFINE_SAFE_INTEGER_OPERATIONS)

#undef CXPLAT_DEFINE_SAFE_INTEGER_OPERATIONS
#undef CXPLAT_INTSAFE_OPERATION_
#undef CXPLAT_INTSAFE_OPERATION
