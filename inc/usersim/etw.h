// Copyright (c) Microsoft Corporation
// SPDX-License-Identifier: MIT
#pragma once
#include "usersim/ke.h"

#ifdef __cplusplus
extern "C"
{
#endif

    typedef _IRQL_requires_max_(PASSIVE_LEVEL) _IRQL_requires_same_ VOID NTAPI ETWENABLECALLBACK(
        _In_ LPCGUID SourceId,
        _In_ ULONG ControlCode,
        _In_ UCHAR Level,
        _In_ ULONGLONG MatchAnyKeyword,
        _In_ ULONGLONG MatchAllKeyword,
        _In_opt_ PEVENT_FILTER_DESCRIPTOR FilterData,
        _Inout_opt_ PVOID CallbackContext);

    typedef ETWENABLECALLBACK* PETWENABLECALLBACK;

    _IRQL_requires_max_(PASSIVE_LEVEL) USERSIM_API NTSTATUS EtwRegister(
        _In_ LPCGUID provider_id,
        _In_opt_ PETWENABLECALLBACK enable_callback,
        _In_opt_ PVOID callback_context,
        _Out_ PREGHANDLE reg_handle);

    _IRQL_requires_max_(PASSIVE_LEVEL) USERSIM_API NTSTATUS EtwUnregister(_In_ REGHANDLE reg_handle);

    _IRQL_requires_max_(HIGH_LEVEL) USERSIM_API NTSTATUS EtwWriteTransfer(
        REGHANDLE reg_handle,
        _In_ EVENT_DESCRIPTOR const* descriptor,
        _In_opt_ LPCGUID activity_id,
        _In_opt_ LPCGUID related_activity_id,
        _In_range_(2, 128) UINT32 data_size,
        _Inout_cap_(data_size) EVENT_DATA_DESCRIPTOR* data);

    USERSIM_API NTSTATUS
    EtwSetInformation(
        REGHANDLE reg_handle,
        EVENT_INFO_CLASS information_class,
        _In_opt_ PVOID information,
        UINT16 const information_size);

#ifdef __cplusplus
}
#endif
