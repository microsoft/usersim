// Copyright (c) Microsoft Corporation
// SPDX-License-Identifier: MIT

#pragma once

// Native NT process-telemetry query definitions used to implement (usersim's PsGetProcessStartKey)
// and to verify (eBPF for Windows api_test) the process start key. Per the official documentation
// these types "have no associated import library or header file", so they must be declared locally;
// this shared header keeps usersim and its consumers from maintaining divergent private copies.
//
// This is a leaf header: it only declares native NT types and depends solely on the basic Windows
// integer/handle/status types (ULONG, ULONG64, HANDLE, PVOID, PULONG, NTSTATUS, NTAPI). Include it
// after <windows.h> (and, where NTSTATUS is not otherwise available, a header that provides it).
//
// https://learn.microsoft.com/en-us/windows/win32/devnotes/process_telemetry_id_information_type

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

// ProcessInformationClass value used to query PROCESS_TELEMETRY_ID_INFORMATION.
enum
{
    ProcessTelemetryIdInformation = 64
};

typedef NTSTATUS(NTAPI* NtQueryInformationProcess_t)(
    _In_ HANDLE ProcessHandle,
    _In_ ULONG ProcessInformationClass,
    _Out_writes_bytes_(ProcessInformationLength) PVOID ProcessInformation,
    _In_ ULONG ProcessInformationLength,
    _Out_opt_ PULONG ReturnLength);
