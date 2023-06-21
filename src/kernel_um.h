// Copyright (c) Microsoft Corporation
// SPDX-License-Identifier: MIT

#pragma once

#include <synchapi.h>
#include <winnt.h>

#if defined(__cplusplus)
extern "C"
{
#endif

#define NTKERNELAPI

// Defines

#define KdPrintEx(_x_) DbgPrintEx _x_
#define NT_SUCCESS(Status) (((NTSTATUS)(Status)) >= 0)
#define PAGED_CODE()

#define STATUS_SUCCESS 0
#define STATUS_INSUFFICIENT_RESOURCES ((NTSTATUS)0xC000009AL)
#define STATUS_ACCESS_DENIED ((NTSTATUS)0xC0000022L)
#define STATUS_NOT_IMPLEMENTED ((NTSTATUS)0xC0000002L)
#define STATUS_NOT_FOUND ((NTSTATUS)0xC0000225L)
#define STATUS_UNSUCCESSFUL ((NTSTATUS)0xC0000001L)
#define STATUS_BUFFER_TOO_SMALL ((NTSTATUS)0xC0000023L)
#define STATUS_NO_MORE_MATCHES ((NTSTATUS)0xC0000273L)
#define STATUS_OBJECT_PATH_NOT_FOUND ((NTSTATUS)0xC000003AL)
#define STATUS_CANCELLED ((NTSTATUS)0xC0000120L)
#define STATUS_ALREADY_INITIALIZED ((NTSTATUS)0xC0000510L)
#define STATUS_NOT_SUPPORTED ((NTSTATUS)0xC00000BBL)
#define STATUS_NO_SUCH_FILE ((NTSTATUS)0xC000000FL)
#define STATUS_BUFFER_OVERFLOW ((NTSTATUS)0x80000005L)
#define STATUS_OBJECTID_EXISTS ((NTSTATUS)0xC000022BL)
#define STATUS_VERIFIER_STOP ((NTSTATUS)0xC0000421L)
#define STATUS_TOO_MANY_COMMANDS ((NTSTATUS)0xC00000C1L)
#define STATUS_NONE_MAPPED ((NTSTATUS)0xC0000073L)
#define STATUS_DRIVER_UNABLE_TO_LOAD ((NTSTATUS)0xC000026CL)
#define STATUS_INVALID_DEVICE_REQUEST ((NTSTATUS)0xC0000010L)
#define STATUS_OBJECT_NAME_EXISTS ((NTSTATUS)0x40000000L)
#define RPC_NT_CALL_FAILED ((NTSTATUS)0xC002001BL)
#define STATUS_INVALID_IMAGE_FORMAT ((NTSTATUS)0xC000007BL)
#define STATUS_TOO_MANY_NODES ((NTSTATUS)0xC000020EL)
#define STATUS_GENERIC_COMMAND_FAILED ((NTSTATUS)0xC0150026L)
#define STATUS_CONTENT_BLOCKED ((NTSTATUS)0xC0000804L)
#define STATUS_RESOURCE_NOT_OWNED ((NTSTATUS)0xC0000264L)
#define PASSIVE_LEVEL 0
#define DISPATCH_LEVEL 2

    // Typedefs

    typedef ULONG LOGICAL;

    typedef enum _MODE
    {
        KernelMode,
        UserMode,
        MaximumMode
    } MODE;

    // Functions

    unsigned long __cdecl DbgPrintEx(
        _In_ unsigned long component_id, _In_ unsigned long level, _In_z_ _Printf_format_string_ PCSTR format, ...);

    unsigned long long
    QueryInterruptTimeEx();

    void
    FatalListEntryError(_In_ void* p1, _In_ void* p2, _In_ void* p3);

    // Inline functions
    _Must_inspect_result_ BOOLEAN CFORCEINLINE
    IsListEmpty(_In_ const LIST_ENTRY* list_head)
    {
        return (BOOLEAN)(list_head->Flink == list_head);
    }

    FORCEINLINE
    void
    InsertTailList(_Inout_ LIST_ENTRY* list_head, _Out_ __drv_aliasesMem LIST_ENTRY* entry)
    {
        LIST_ENTRY* PrevEntry;
        PrevEntry = list_head->Blink;
        if (PrevEntry->Flink != list_head) {
            FatalListEntryError((void*)PrevEntry, (void*)list_head, (void*)PrevEntry->Flink);
        }

        entry->Flink = list_head;
        entry->Blink = PrevEntry;
        PrevEntry->Flink = entry;
        list_head->Blink = entry;
        return;
    }

    FORCEINLINE
    void
    InsertHeadList(_Inout_ LIST_ENTRY* list_head, _Out_ __drv_aliasesMem LIST_ENTRY* entry)
    {
        LIST_ENTRY* NextEntry;
        NextEntry = list_head->Flink;
        if (NextEntry->Blink != list_head) {
            FatalListEntryError((void*)NextEntry, (void*)list_head, (void*)NextEntry->Blink);
        }

        entry->Flink = NextEntry;
        entry->Blink = list_head;
        NextEntry->Blink = entry;
        list_head->Flink = entry;
        return;
    }

    FORCEINLINE
    BOOLEAN
    RemoveEntryList(_In_ LIST_ENTRY* entry)
    {
        LIST_ENTRY* PrevEntry;
        LIST_ENTRY* NextEntry;

        NextEntry = entry->Flink;
        PrevEntry = entry->Blink;
        if ((NextEntry->Blink != entry) || (PrevEntry->Flink != entry)) {
            FatalListEntryError((void*)PrevEntry, (void*)entry, (void*)NextEntry);
        }

        PrevEntry->Flink = NextEntry;
        NextEntry->Blink = PrevEntry;
        return (BOOLEAN)(PrevEntry == NextEntry);
    }

    FORCEINLINE
    void
    InitializeListHead(_Out_ LIST_ENTRY* list_head)
    {
        list_head->Flink = list_head->Blink = list_head;
        return;
    }

    FORCEINLINE
    LIST_ENTRY*
    RemoveHeadList(_Inout_ LIST_ENTRY* list_head)
    {
        LIST_ENTRY* entry;
        LIST_ENTRY* NextEntry;

        entry = list_head->Flink;

        NextEntry = entry->Flink;
        if ((entry->Blink != list_head) || (NextEntry->Blink != entry)) {
            FatalListEntryError((void*)list_head, (void*)entry, (void*)NextEntry);
        }

        list_head->Flink = NextEntry;
        NextEntry->Blink = list_head;

        return entry;
    }

    FORCEINLINE
    void
    AppendTailList(_Inout_ LIST_ENTRY* list_head, _Inout_ LIST_ENTRY* list_to_append)
    {
        LIST_ENTRY* list_end = list_head->Blink;
        list_head->Blink->Flink = list_to_append;
        list_head->Blink = list_to_append->Blink;
        list_to_append->Blink->Flink = list_head;
        list_to_append->Blink = list_end;
    }

#define ObReferenceObject(process) UNREFERENCED_PARAMETER(process)
#define ObDereferenceObject(process) UNREFERENCED_PARAMETER(process)

#if defined(__cplusplus)
}
#endif
