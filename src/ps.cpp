// Copyright (c) Microsoft Corporation
// SPDX-License-Identifier: MIT

#include "kernel_um.h"
#include "platform.h"
#include "usersim/ps.h"

// Ps* functions.

HANDLE
PsGetCurrentProcessId() { return (HANDLE)(uintptr_t)GetCurrentProcessId(); }

_IRQL_requires_max_(DISPATCH_LEVEL) NTKERNELAPI HANDLE PsGetCurrentThreadId()
{
    return (HANDLE)(uintptr_t)GetCurrentThreadId();
}

typedef struct _PROCESS_TELEMETRY_ID_INFORMATION
{
    ULONG HeaderSize;
    ULONG ProcessId;
    ULONG64 ProcessStartKey;
    ULONG64 CreateTime;
    ULONG64 CreateInterruptTime;
    ULONG64 CreateUnbiasedInterruptTime;
    ULONG64 ProcessSequenceNumber;
    ULONG64 SessionCreateTime;
    ULONG SessionId;
    ULONG BootId;
    ULONG ImageChecksum;
    ULONG ImageTimeDateStamp;
    ULONG UserSidOffset;
    ULONG ImagePathOffset;
    ULONG PackageNameOffset;
    ULONG RelativeAppNameOffset;
    ULONG CommandLineOffset;
} PROCESS_TELEMETRY_ID_INFORMATION, *PPROCESS_TELEMETRY_ID_INFORMATION;

typedef enum _PROCESSINFOCLASS
{
    ProcessBasicInformation = 0,
    ProcessDebugPort = 7,
    ProcessWow64Information = 26,
    ProcessImageFileName = 27,
    ProcessBreakOnTermination = 29,
    ProcessTelemetryIdInformation = 64,
    ProcessSubsystemInformation = 75
} PROCESSINFOCLASS;

typedef NTSTATUS (NTAPI *NtQueryInformationProcess_t)(
    _In_ HANDLE ProcessHandle,
    _In_ PROCESSINFOCLASS ProcessInformationClass,
    _Out_writes_bytes_(ProcessInformationLength) PVOID ProcessInformation,
    _In_ ULONG ProcessInformationLength,
    _Out_opt_ PULONG ReturnLength);

static PGETPROCESSSTARTKEY _usersim_get_process_start_key_callback = nullptr;
USERSIM_API
void
usersime_set_process_start_key_callback(_In_ PGETPROCESSSTARTKEY callback)
{
    _usersim_get_process_start_key_callback = callback;
}

USERSIM_API
ULONGLONG
PsGetProcessStartKey(_In_ PEPROCESS process)
{

    if (_usersim_get_process_start_key_callback != nullptr) {
        return _usersim_get_process_start_key_callback(process);
    }

    PROCESS_TELEMETRY_ID_INFORMATION telemetryId{};
    NtQueryInformationProcess_t NtQueryInformationProcess = reinterpret_cast<NtQueryInformationProcess_t>(
        GetProcAddress(GetModuleHandle(L"ntdll.dll"),
        "NtQueryInformationProcess"));
    NTSTATUS status = NtQueryInformationProcess(
        GetCurrentProcess(),
        ProcessTelemetryIdInformation,
        &telemetryId,
        sizeof(telemetryId),
        nullptr);

    if (NT_SUCCESS(status)) {
        return telemetryId.ProcessStartKey;
    }
    return 0;
}

static PGETPROCESSEXITSTATUS _usersim_get_process_exit_status_callback = NULL;

USERSIM_API
NTSTATUS
PsGetProcessExitStatus(_In_ PEPROCESS Process)
{
    if (_usersim_get_process_exit_status_callback != NULL) {
        return _usersim_get_process_exit_status_callback(Process);
    }

    // Fall back to a failure code
    return -1;
}

USERSIM_API
void
usersime_set_process_exit_status_callback(_In_ PGETPROCESSEXITSTATUS callback)
{
    _usersim_get_process_exit_status_callback = callback;
}

static PGETPROCESSCREATETIMEQUADPART _usersim_get_process_create_time_quadpart_callback = NULL;

USERSIM_API
LONGLONG
PsGetProcessCreateTimeQuadPart(_In_ PEPROCESS Process)
{
    if (_usersim_get_process_create_time_quadpart_callback != NULL) {
        return _usersim_get_process_create_time_quadpart_callback(Process);
    }

    // Fall back to the beginning of time
    return 0;
}

static PGETTHREADCREATETIME _usersim_get_thread_start_time_callback = nullptr;

USERSIM_API
void
usersime_set_thread_create_time_callback(_In_ PGETTHREADCREATETIME callback)
{
    _usersim_get_thread_start_time_callback = callback;
}

USERSIM_API
LONGLONG
PsGetThreadCreateTime(_In_ HANDLE thread)
{
    FILETIME creation, exit, kernel, user;

    if (_usersim_get_thread_start_time_callback != nullptr) {
        return _usersim_get_thread_start_time_callback(thread);
    }

    if (GetThreadTimes(GetCurrentThread(), &creation, &exit, &kernel, &user)) {
        return static_cast<LONGLONG>(creation.dwLowDateTime) | (static_cast<LONGLONG>(creation.dwHighDateTime) << 32);
    }

    // Fall back to the beginning of time
    return 0;
}

USERSIM_API
void
usersime_set_process_create_time_quadpart_callback(_In_ PGETPROCESSCREATETIMEQUADPART callback)
{
    _usersim_get_process_create_time_quadpart_callback = callback;
}

static PGETPROCESSEXITTIME _usersim_get_process_exit_time_callback = NULL;

USERSIM_API
LARGE_INTEGER PsGetProcessExitTime(VOID)
{
    if (_usersim_get_process_exit_time_callback != NULL) {
        return _usersim_get_process_exit_time_callback();
    }

    // Fall back to the beginning of time
    LARGE_INTEGER li = {0};
    return li;
}

void
usersime_set_process_exit_time_callback(_In_ PGETPROCESSEXITTIME callback)
{
    _usersim_get_process_exit_time_callback = callback;
}

static PCREATE_PROCESS_NOTIFY_ROUTINE_EX _usersim_process_creation_notify_routine = NULL;

USERSIM_API
NTSTATUS
PsSetCreateProcessNotifyRoutineEx(_In_ PCREATE_PROCESS_NOTIFY_ROUTINE_EX notify_routine, _In_ BOOLEAN remove)
{
    if (notify_routine == NULL) {
        return STATUS_INVALID_PARAMETER;
    }

    // Remove the routine.
    if (remove) {
        if (_usersim_process_creation_notify_routine == NULL) {
            return STATUS_INVALID_PARAMETER;
        }
        _usersim_process_creation_notify_routine = NULL;
        return STATUS_SUCCESS;
    }
    // Only one routine is supported.
    if (_usersim_process_creation_notify_routine != NULL) {
        return STATUS_INVALID_PARAMETER;
    }

    // Set the routine.
    _usersim_process_creation_notify_routine = notify_routine;
    return STATUS_SUCCESS;
}

void
usersime_invoke_process_creation_notify_routine(
    _Inout_ PEPROCESS process, _In_ HANDLE process_id, _Inout_opt_ PPS_CREATE_NOTIFY_INFO create_info)
{
    if (_usersim_process_creation_notify_routine != NULL) {
        _usersim_process_creation_notify_routine(process, process_id, create_info);
    }
}