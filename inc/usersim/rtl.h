// Copyright (c) Microsoft Corporation
// SPDX-License-Identifier: MIT

#pragma once
#include "../src/platform.h"
#include <strsafe.h>

#if defined(__cplusplus)
extern "C"
{
#endif

    __declspec(dllexport)
    NTSTATUS
    RtlULongAdd(
        _In_ unsigned long augend,
        _In_ unsigned long addend,
        _Out_ _Deref_out_range_(==, augend + addend) unsigned long* result);

    __declspec(dllexport)
    NTSTATUS
    RtlCreateAcl(_Out_ PACL Acl, unsigned long AclLength, unsigned long AclRevision);

    __declspec(dllexport)
    VOID
    RtlMapGenericMask(_Inout_ PACCESS_MASK AccessMask, _In_ const GENERIC_MAPPING* GenericMapping);

    __declspec(dllexport)
    unsigned long
    RtlLengthSid(_In_ PSID Sid);

    __declspec(dllexport)
    NTSTATUS
    NTAPI
    RtlAddAccessAllowedAce(
        _Inout_ PACL Acl, _In_ unsigned long AceRevision, _In_ ACCESS_MASK AccessMask, _In_ PSID Sid);

    __declspec(dllexport)
    NTSTATUS
    NTAPI
    RtlCreateSecurityDescriptor(_Out_ PSECURITY_DESCRIPTOR SecurityDescriptor, _In_ unsigned long Revision);

    __declspec(dllexport)
    NTSTATUS
    NTAPI
    RtlSetDaclSecurityDescriptor(
        _Inout_ PSECURITY_DESCRIPTOR SecurityDescriptor,
        _In_ BOOLEAN DaclPresent,
        _In_opt_ PACL Dacl,
        _In_ BOOLEAN DaclDefaulted);

    /**
     * @brief Multiplies one value of type size_t by another and check for
     *   overflow.
     * @param[in] multiplicand The value to be multiplied by multiplier.
     * @param[in] multiplier The value by which to multiply multiplicand.
     * @param[out] result A pointer to the result.
     * @retval STATUS_SUCCESS The operation was successful.
     * @retval STATUS_ERROR_ARITHMETIC_OVERFLOW Multiplication overflowed.
     */
    __declspec(dllexport)
    _Must_inspect_result_ NTSTATUS
    RtlSizeTMult(size_t multiplicand, size_t multiplier, _Out_ size_t* result);

    /**
     * @brief Add one value of type size_t by another and check for
     *   overflow.
     * @param[in] augend The value to be added by addend.
     * @param[in] addend The value add to augend.
     * @param[out] result A pointer to the result.
     * @retval STATUS_SUCCESS The operation was successful.
     * @retval STATUS_ERROR_ARITHMETIC_OVERFLOW Addition overflowed.
     */
    __declspec(dllexport)
    _Must_inspect_result_ NTSTATUS
    RtlSizeTAdd(size_t augend, size_t addend, _Out_ _Deref_out_range_(==, augend + addend) size_t* result);

    /**
     * @brief Subtract one value of type size_t from another and check for
     *   overflow or underflow.
     * @param[in] minuend The value from which subtrahend is subtracted.
     * @param[in] subtrahend The value subtract from minuend.
     * @param[out] result A pointer to the result.
     * @retval STATUS_SUCCESS The operation was successful.
     * @retval STATUS_ERROR_ARITHMETIC_OVERFLOW Addition overflowed or underflowed.
     */
    __declspec(dllexport)
    _Must_inspect_result_ NTSTATUS
    RtlSizeTSub(size_t minuend, size_t subtrahend, _Out_ _Deref_out_range_(==, minuend - subtrahend) size_t* result);

    __declspec(dllexport)
    ULONG RtlRandomEx(_Inout_ PULONG seed);

    #define RtlStringCchVPrintfA StringCchVPrintfA

    __declspec(dllexport)
    NTSTATUS WINAPI
    RtlUTF8ToUnicodeN(
        _Out_opt_ PWSTR unicode_string_destination,
        _In_ ULONG unicode_string_max_byte_count,
        _Out_ PULONG unicode_string_actual_byte_count,
        _In_ PCCH utf8_string_source,
        _In_ ULONG utf8_string_byte_count);

#if defined(__cplusplus)
}
#endif
