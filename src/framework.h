// Copyright (c) Microsoft Corporation
// SPDX-License-Identifier: MIT

#pragma once

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#define WIN32_LEAN_AND_MEAN // Exclude rarely-used stuff from Windows headers
// Windows Header Files
#include <windows.h>
#include <rpc.h>
// winioctl.h does not protect against multiple inclusion, so we need to
// do so ourselves.
#ifndef WDI_STORAGE_PREDICT_FAILURE_DPS_GUID
#include <winioctl.h>
#endif
typedef _Return_type_success_(return >= 0) long NTSTATUS;
typedef NTSTATUS* PNTSTATUS;
#define NT_SUCCESS(status) (((NTSTATUS)(status)) >= 0)
#include "kernel_um.h"

#include <../km/netioddk.h>
#include <netiodef.h>
#define STATUS_NOINTERFACE ((NTSTATUS)0xC00002B9L)
#include "cxplat.h"

#pragma comment(lib, "rpcrt4")

#if !defined(UNREFERENCED_PARAMETER)
#define UNREFERENCED_PARAMETER(X) (X)
#endif

// Types and functions from the ntddk duplicated here to allow user and kernel more closely align.

CXPLAT_EXTERN_C_BEGIN

typedef LIST_ENTRY usersim_list_entry_t;

inline void
usersim_list_initialize(_Out_ usersim_list_entry_t* list_head)
{

    list_head->Flink = list_head->Blink = list_head;
    return;
}

inline bool
usersim_list_is_empty(_In_ const usersim_list_entry_t* list_head)
{

    return (list_head->Flink == list_head);
}

inline void
usersim_list_insert_tail(_Inout_ usersim_list_entry_t* list_head, _Out_ usersim_list_entry_t* entry)
{
    usersim_list_entry_t* previous_entry;
    previous_entry = list_head->Blink;

    entry->Flink = list_head;
    entry->Blink = previous_entry;
    previous_entry->Flink = entry;
    list_head->Blink = entry;
    CXPLAT_DEBUG_ASSERT(list_head->Blink->Flink == list_head);
    CXPLAT_DEBUG_ASSERT(list_head->Flink->Blink == list_head);
    return;
}

inline bool
usersim_list_remove_entry(_Inout_ usersim_list_entry_t* entry)
{
    usersim_list_entry_t* previous_entry;
    usersim_list_entry_t* next_entry;

    next_entry = entry->Flink;
    previous_entry = entry->Blink;

    previous_entry->Flink = next_entry;
    next_entry->Blink = previous_entry;

    // Mark this entry as not in a list, for use by KeRemoveQueueDpc.
    entry->Flink = entry->Blink = entry;

    return (previous_entry == next_entry);
}

inline usersim_list_entry_t*
usersim_list_remove_head_entry(_Inout_ usersim_list_entry_t* list_head)
{
    if (list_head->Flink == list_head) {
        return list_head;
    }
    usersim_list_entry_t* removed = list_head->Flink;
    list_head->Flink = removed->Flink;
    removed->Flink->Blink = list_head;

    return removed;
}

inline void
usersim_list_append_tail_list(_Inout_ usersim_list_entry_t* list_head, _Inout_ usersim_list_entry_t* list_to_append)
{
    usersim_list_entry_t* list_end = list_head->Blink;

    list_head->Blink->Flink = list_to_append;
    list_head->Blink = list_to_append->Blink;
    list_to_append->Blink->Flink = list_head;
    list_to_append->Blink = list_end;
}

inline void
usersim_probe_for_write(_Out_writes_bytes_(length) void* address, size_t length, unsigned long alignment)
{
    if (((uintptr_t)address % alignment) != 0) {
        RaiseException(STATUS_DATATYPE_MISALIGNMENT, 0, 0, NULL);
    }
    UNREFERENCED_PARAMETER(length);
}

CXPLAT_EXTERN_C_END
