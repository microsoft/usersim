// Copyright (c) Microsoft Corporation
// SPDX-License-Identifier: MIT

#pragma once
#include "..\src\platform.h"
#include "usersim\io.h"
#include "usersim\rtl.h"

CXPLAT_EXTERN_C_BEGIN

#define PsGetCurrentProcess IoGetCurrentProcess

USERSIM_API
HANDLE
PsGetCurrentProcessId();

typedef NTSTATUS (*PGETPROCESSEXITSTATUS)(_In_ PEPROCESS process);

USERSIM_API
_IRQL_requires_max_(APC_LEVEL) NTKERNELAPI NTSTATUS PsGetProcessExitStatus(_In_ PEPROCESS Process);

USERSIM_API
void
usersime_set_process_exit_status_callback(_In_ PGETPROCESSEXITSTATUS callback);

typedef LONGLONG (*PGETPROCESSCREATETIMEQUADPART)(_In_ PEPROCESS process);

USERSIM_API
_IRQL_requires_max_(DISPATCH_LEVEL) NTKERNELAPI LONGLONG PsGetProcessCreateTimeQuadPart(_In_ PEPROCESS Process);

USERSIM_API
void
usersime_set_process_create_time_quadpart_callback(_In_ PGETPROCESSCREATETIMEQUADPART callback);

typedef LARGE_INTEGER (*PGETPROCESSEXITTIME)(VOID);

USERSIM_API
_IRQL_requires_max_(APC_LEVEL) NTKERNELAPI LARGE_INTEGER PsGetProcessExitTime(VOID);

USERSIM_API
void
usersime_set_process_exit_time_callback(_In_ PGETPROCESSEXITTIME callback);

USERSIM_API
_IRQL_requires_max_(DISPATCH_LEVEL) NTKERNELAPI HANDLE PsGetCurrentThreadId();

typedef struct _CLIENT_ID
{
    HANDLE UniqueProcess;
    HANDLE UniqueThread;
} CLIENT_ID;
typedef CLIENT_ID* PCLIENT_ID;

typedef struct _PS_CREATE_NOTIFY_INFO
{
    _In_ SIZE_T Size;
    union
    {
        _In_ ULONG Flags;
        struct
        {
            _In_ ULONG FileOpenNameAvailable : 1;
            _In_ ULONG IsSubsystemProcess : 1;
            _In_ ULONG Reserved : 30;
        };
    };
    _In_ HANDLE ParentProcessId;
    _In_ CLIENT_ID CreatingThreadId;
    _Inout_ struct _FILE_OBJECT* FileObject;
    _In_ PCUNICODE_STRING ImageFileName;
    _In_opt_ PCUNICODE_STRING CommandLine;
    _Inout_ NTSTATUS CreationStatus;
} PS_CREATE_NOTIFY_INFO, *PPS_CREATE_NOTIFY_INFO;

typedef VOID (*PCREATE_PROCESS_NOTIFY_ROUTINE_EX)(
    _Inout_ PEPROCESS process, _In_ HANDLE process_id, _Inout_opt_ PPS_CREATE_NOTIFY_INFO create_info);

USERSIM_API
NTSTATUS
PsSetCreateProcessNotifyRoutineEx(_In_ PCREATE_PROCESS_NOTIFY_ROUTINE_EX notify_routine, _In_ BOOLEAN remove);

USERSIM_API
void
usersime_invoke_process_creation_notify_routine(
    _Inout_ PEPROCESS process, _In_ HANDLE process_id, _Inout_opt_ PPS_CREATE_NOTIFY_INFO create_info);

CXPLAT_EXTERN_C_END
