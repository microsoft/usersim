// Copyright (c) Microsoft Corporation
// SPDX-License-Identifier: MIT
//
// This file contains user-mode definitions for Rtl* APIs
// implemented in usersim.dll, and can be used by unit tests.
#pragma once

#include "../src/platform.h"

#include <strsafe.h>

CXPLAT_EXTERN_C_BEGIN

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
RtlAddAccessAllowedAce(_Inout_ PACL Acl, _In_ unsigned long AceRevision, _In_ ACCESS_MASK AccessMask, _In_ PSID Sid);

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
ULONG
RtlRandomEx(_Inout_ PULONG seed);

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
    _In_ PVOID void_failed_assertion, _In_ PVOID void_file_name, _In_ ULONG line_number, _In_opt_ PSTR mutable_message);

typedef struct _STRING
{
    USHORT Length;
    USHORT MaximumLength;
    _Field_size_bytes_part_opt_(MaximumLength, Length) PCHAR Buffer;
} STRING;
typedef STRING* PSTRING;
typedef STRING ANSI_STRING;
typedef PSTRING PANSI_STRING;

typedef struct _UNICODE_STRING
{
    USHORT Length;
    USHORT MaximumLength;
    _Field_size_bytes_part_opt_(MaximumLength, Length) PWCH Buffer;
} UNICODE_STRING, *PUNICODE_STRING;
typedef const UNICODE_STRING* PCUNICODE_STRING;

typedef STRING UTF8_STRING;
typedef PSTRING PUTF8_STRING;

#define DECLARE_CONST_UNICODE_STRING(_var, _string)                                                                   \
    const WCHAR _var##_buffer[] = _string;                                                                            \
    __pragma(warning(push)) __pragma(warning(disable : 4221)) __pragma(warning(disable : 4204))                       \
        const UNICODE_STRING _var = {sizeof(_string) - sizeof(WCHAR), sizeof(_string), (PWCH)_var##_buffer} __pragma( \
            warning(pop))

USERSIM_API VOID
RtlInitString(_Out_ PSTRING destination_string, _In_opt_ __drv_aliasesMem PCSTR source_string);

_IRQL_requires_max_(DISPATCH_LEVEL) _At_(destination_string->Buffer, _Post_equal_to_(source_string))
    _At_(destination_string->Length, _Post_equal_to_(_String_length_(source_string) * sizeof(WCHAR)))
        _At_(destination_string->MaximumLength, _Post_equal_to_((_String_length_(source_string) + 1) * sizeof(WCHAR)))
            USERSIM_API VOID NTAPI
    RtlInitUnicodeString(_Out_ PUNICODE_STRING destination_string, _In_opt_z_ __drv_aliasesMem PCWSTR source_string);

_IRQL_requires_max_(DISPATCH_LEVEL) USERSIM_API VOID NTAPI
    RtlInitUTF8String(_Out_ PUTF8_STRING DestinationString, _In_opt_z_ __drv_aliasesMem const char* SourceString);

_IRQL_requires_max_(PASSIVE_LEVEL) USERSIM_API VOID NTAPI
    RtlFreeUnicodeString(_Inout_ _At_(UnicodeString->Buffer, _Frees_ptr_opt_) PUNICODE_STRING UnicodeString);

_When_(
    AllocateDestinationString,
    _At_(DestinationString->MaximumLength, _Out_range_(<=, (SourceString->MaximumLength / sizeof(WCHAR)))))
    _When_(
        !AllocateDestinationString,
        _At_(DestinationString->Buffer, _Const_) _At_(DestinationString->MaximumLength, _Const_))
        _IRQL_requires_max_(PASSIVE_LEVEL)
            _When_(AllocateDestinationString, _Must_inspect_result_) USERSIM_API NTSTATUS NTAPI
    RtlUnicodeStringToUTF8String(
        _When_(AllocateDestinationString, _Out_ _At_(DestinationString->Buffer, __drv_allocatesMem(Mem)))
            _When_(!AllocateDestinationString, _Inout_) PUTF8_STRING DestinationString,
        _In_ PCUNICODE_STRING SourceString,
        _In_ BOOLEAN AllocateDestinationString);

_IRQL_requires_max_(PASSIVE_LEVEL) _Must_inspect_result_ NTSYSAPI NTSTATUS NTAPI RtlUTF8StringToUnicodeString(
    _When_(AllocateDestinationString, _Out_ _At_(DestinationString->Buffer, __drv_allocatesMem(Mem)))
        _When_(!AllocateDestinationString, _Inout_) PUNICODE_STRING DestinationString,
    _In_ PUTF8_STRING SourceString,
    _In_ BOOLEAN AllocateDestinationString);

_IRQL_requires_max_(PASSIVE_LEVEL) USERSIM_API void NTAPI
    RtlFreeUTF8String(_Inout_ _At_(utf8String->Buffer, _Frees_ptr_opt_) PUTF8_STRING utf8String);

typedef struct _OBJECT_ATTRIBUTES
{
    ULONG Length;
    HANDLE RootDirectory;
    PUNICODE_STRING ObjectName;
    ULONG Attributes;
    SECURITY_DESCRIPTOR* SecurityDescriptor;
    SECURITY_QUALITY_OF_SERVICE* SecurityQualityOfService;
} OBJECT_ATTRIBUTES, *POBJECT_ATTRIBUTES;

typedef ULONG CLONG;

typedef struct _RTL_BALANCED_LINKS
{
    struct _RTL_BALANCED_LINKS* Parent;
    struct _RTL_BALANCED_LINKS* LeftChild;
    struct _RTL_BALANCED_LINKS* RightChild;
    CHAR Balance;
    UCHAR Reserved[3];
} RTL_BALANCED_LINKS;
typedef RTL_BALANCED_LINKS* PRTL_BALANCED_LINKS;

typedef enum _TABLE_SEARCH_RESULT
{
    TableEmptyTree,
    TableFoundNode,
    TableInsertAsLeft,
    TableInsertAsRight
} TABLE_SEARCH_RESULT;

typedef enum _RTL_GENERIC_COMPARE_RESULTS
{
    GenericLessThan,
    GenericGreaterThan,
    GenericEqual
} RTL_GENERIC_COMPARE_RESULTS;

typedef RTL_GENERIC_COMPARE_RESULTS (*PRTL_AVL_COMPARE_ROUTINE)(
    _In_ struct _RTL_AVL_TABLE* Table, _In_ PVOID FirstStruct, _In_ PVOID SecondStruct);

typedef PVOID (*PRTL_AVL_ALLOCATE_ROUTINE)(_In_ struct _RTL_AVL_TABLE* Table, _In_ CLONG ByteSize);

typedef VOID (*PRTL_AVL_FREE_ROUTINE)(_In_ struct _RTL_AVL_TABLE* Table, _In_ PVOID Buffer);

typedef struct _RTL_AVL_TABLE
{
    RTL_BALANCED_LINKS BalancedRoot;
    PVOID OrderedPointer;
    ULONG WhichOrderedElement;
    ULONG NumberGenericTableElements;
    ULONG DepthOfTree;
    PRTL_BALANCED_LINKS RestartKey;
    ULONG DeleteCount;
    PRTL_AVL_COMPARE_ROUTINE CompareRoutine;
    PRTL_AVL_ALLOCATE_ROUTINE AllocateRoutine;
    PRTL_AVL_FREE_ROUTINE FreeRoutine;
    PVOID TableContext;
} RTL_AVL_TABLE, *PRTL_AVL_TABLE;

NTSYSAPI VOID
RtlInitializeGenericTableAvl(
    _Out_ PRTL_AVL_TABLE Table,
    _In_ PRTL_AVL_COMPARE_ROUTINE CompareRoutine,
    _In_ PRTL_AVL_ALLOCATE_ROUTINE AllocateRoutine,
    _In_ PRTL_AVL_FREE_ROUTINE FreeRoutine,
    _In_opt_ PVOID TableContext);

NTSYSAPI BOOLEAN
RtlDeleteElementGenericTableAvl(_In_ PRTL_AVL_TABLE Table, _In_ PVOID Buffer);

NTSYSAPI PVOID
RtlEnumerateGenericTableWithoutSplayingAvl(_In_ PRTL_AVL_TABLE Table, _Inout_ PVOID* RestartKey);

NTSYSAPI PVOID
RtlEnumerateGenericTableAvl(_In_ PRTL_AVL_TABLE Table, _In_ BOOLEAN Restart);

NTSYSAPI PVOID
RtlGetElementGenericTableAvl(_In_ PRTL_AVL_TABLE Table, _In_ ULONG I);

NTSYSAPI PVOID
RtlInsertElementGenericTableAvl(
    _In_ PRTL_AVL_TABLE Table, _In_ PVOID Buffer, _In_ CLONG BufferSize, _Out_opt_ PBOOLEAN NewElement);

NTSYSAPI PVOID
RtlInsertElementGenericTableFullAvl(
    _In_ PRTL_AVL_TABLE Table,
    _In_ PVOID Buffer,
    _In_ CLONG BufferSize,
    _Out_opt_ PBOOLEAN NewElement,
    _In_ PVOID NodeOrParent,
    _In_ TABLE_SEARCH_RESULT SearchResult);

NTSYSAPI BOOLEAN
RtlIsGenericTableEmptyAvl(_In_ PRTL_AVL_TABLE Table);

NTSYSAPI PVOID
RtlLookupElementGenericTableAvl(_In_ PRTL_AVL_TABLE Table, _In_ PVOID Buffer);

NTSYSAPI PVOID
RtlLookupElementGenericTableFullAvl(
    _In_ PRTL_AVL_TABLE Table, _In_ PVOID Buffer, _Out_ PVOID* NodeOrParent, _Out_ TABLE_SEARCH_RESULT* SearchResult);

NTSYSAPI PVOID
RtlLookupFirstMatchingElementGenericTableAvl(_In_ PRTL_AVL_TABLE Table, _In_ PVOID Buffer, _In_ PVOID* RestartKey);

NTSYSAPI ULONG
RtlNumberGenericTableElementsAvl(_In_ PRTL_AVL_TABLE Table);

// Include Rtl* implementations from ntdll.lib.
#pragma comment(lib, "ntdll.lib")

CXPLAT_EXTERN_C_END

#if defined(__cplusplus)
// The functions below throw C++ exceptions so tests can catch them to verify error behavior.

USERSIM_API __analysis_noreturn void
RtlAssertCPP(
    _In_ PVOID void_failed_assertion, _In_ PVOID void_file_name, _In_ ULONG line_number, _In_opt_ PSTR mutable_message);

#endif