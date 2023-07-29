// Copyright (c) Microsoft Corporation
// SPDX-License-Identifier: MIT

#pragma once
#include "../src/platform.h"
#include <strsafe.h>
#include "../usersim_dll_skeleton/dll_skeleton.h"

#if defined(__cplusplus)
extern "C"
{
#endif

    USERSIM_API
    NTSTATUS
    RtlULongAdd(
        _In_ unsigned long augend,
        _In_ unsigned long addend,
        _Out_ _Deref_out_range_(==, augend + addend) unsigned long* result);

    USERSIM_API
    NTSTATUS
    RtlCreateAcl(_Out_ PACL Acl, unsigned long AclLength, unsigned long AclRevision);

    USERSIM_API
    VOID
    RtlMapGenericMask(_Inout_ PACCESS_MASK AccessMask, _In_ const GENERIC_MAPPING* GenericMapping);

    USERSIM_API
    unsigned long
    RtlLengthSid(_In_ PSID sid);

    USERSIM_API
    BOOLEAN
    RtlValidSid(_In_ PSID sid);

    USERSIM_API
    NTSTATUS
    NTAPI
    RtlAddAccessAllowedAce(
        _Inout_ PACL Acl, _In_ unsigned long AceRevision, _In_ ACCESS_MASK AccessMask, _In_ PSID Sid);

    USERSIM_API
    NTSTATUS
    NTAPI
    RtlCreateSecurityDescriptor(_Out_ PSECURITY_DESCRIPTOR SecurityDescriptor, _In_ unsigned long Revision);

    USERSIM_API
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
    USERSIM_API
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
    USERSIM_API
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
    USERSIM_API
    _Must_inspect_result_ NTSTATUS
    RtlSizeTSub(size_t minuend, size_t subtrahend, _Out_ _Deref_out_range_(==, minuend - subtrahend) size_t* result);

    USERSIM_API
    ULONG RtlRandomEx(_Inout_ PULONG seed);

    #define RtlStringCchVPrintfA StringCchVPrintfA

    USERSIM_API
    NTSTATUS WINAPI
    RtlUTF8ToUnicodeN(
        _Out_opt_ PWSTR unicode_string_destination,
        _In_ ULONG unicode_string_max_byte_count,
        _Out_ PULONG unicode_string_actual_byte_count,
        _In_ PCCH utf8_string_source,
        _In_ ULONG utf8_string_byte_count);

    USERSIM_API
    __analysis_noreturn VOID NTAPI
    RtlAssert(
        _In_ PVOID void_failed_assertion,
        _In_ PVOID void_file_name,
        _In_ ULONG line_number,
        _In_opt_ PSTR mutable_message);

    _IRQL_requires_max_(DISPATCH_LEVEL) _At_(DestinationString->Buffer, _Post_equal_to_(SourceString))
    _At_(DestinationString->Length, _Post_equal_to_(_String_length_(SourceString) * sizeof(WCHAR)))
    _At_(DestinationString->MaximumLength, _Post_equal_to_((_String_length_(SourceString) + 1) * sizeof(WCHAR)))
    USERSIM_API VOID NTAPI
    RtlInitUnicodeString(
        _Out_ PUNICODE_STRING destination_string,
        _In_opt_z_ __drv_aliasesMem PCWSTR source_string);

// Include Rtl* implementations from ntdll.lib.
#pragma comment(lib, "ntdll.lib")

#if defined(__cplusplus)
}

// The functions below throw C++ exceptions so tests can catch them to verify error behavior.

USERSIM_API __analysis_noreturn void
RtlAssertCPP(
    _In_ PVOID void_failed_assertion,
    _In_ PVOID void_file_name,
    _In_ ULONG line_number,
    _In_opt_ PSTR mutable_message);

#endif
