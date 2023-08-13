// Copyright (c) Microsoft Corporation
// SPDX-License-Identifier: MIT

#include "platform.h"
#include "usersim/etw.h"
#include "usersim/ex.h"

typedef struct
{
    GUID provider_id;
    PETWENABLECALLBACK enable_callback;
    PVOID callback_context;
} usersim_etw_provider_t;

_IRQL_requires_max_(PASSIVE_LEVEL) NTSTATUS EtwRegister(
    _In_ LPCGUID provider_id,
    _In_opt_ PETWENABLECALLBACK enable_callback,
    _In_opt_ PVOID callback_context,
    _Out_ PREGHANDLE reg_handle)
{
    usersim_etw_provider_t* provider = (usersim_etw_provider_t*)usersim_allocate(sizeof(*provider));
    if (provider == nullptr) {
        return STATUS_NO_MEMORY;
    }
    provider->provider_id = *provider_id;
    provider->enable_callback = enable_callback;
    provider->callback_context = callback_context;
    *reg_handle = (uintptr_t)provider;
    return STATUS_SUCCESS;
}

_IRQL_requires_max_(PASSIVE_LEVEL) NTSTATUS EtwUnregister(_In_ REGHANDLE reg_handle)
{
    usersim_free((void*)(uintptr_t)reg_handle);
    return STATUS_SUCCESS;
}

_IRQL_requires_max_(HIGH_LEVEL) NTSTATUS EtwWriteTransfer(
    REGHANDLE reg_handle,
    _In_ EVENT_DESCRIPTOR const* desc,
    _In_opt_ LPCGUID activity_id,
    _In_opt_ LPCGUID related_activity_id,
    _In_range_(2, 128) UINT32 data_size,
    _Inout_cap_(data_size) EVENT_DATA_DESCRIPTOR* data)
{
    usersim_etw_provider_t* provider = (usersim_etw_provider_t*)reg_handle;

    // TODO(#70): implement similar to usersim_trace_logging_write().
    UNREFERENCED_PARAMETER(provider);
    UNREFERENCED_PARAMETER(desc);
    UNREFERENCED_PARAMETER(activity_id);
    UNREFERENCED_PARAMETER(related_activity_id);
    UNREFERENCED_PARAMETER(data_size);
    UNREFERENCED_PARAMETER(data);
    return STATUS_SUCCESS;
}

NTSTATUS
EtwSetInformation(
    REGHANDLE reg_handle, EVENT_INFO_CLASS information_class, _In_opt_ PVOID information, UINT16 const information_size)
{
    UNREFERENCED_PARAMETER(reg_handle);
    UNREFERENCED_PARAMETER(information_class);
    UNREFERENCED_PARAMETER(information);
    UNREFERENCED_PARAMETER(information_size);
    return STATUS_SUCCESS;
}
