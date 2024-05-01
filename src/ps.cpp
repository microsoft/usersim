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

static PGETPROCESSEXITSTATUS _usersim_get_process_exit_status_callback = NULL;

NTSTATUS
PsGetProcessExitStatus(_In_ PEPROCESS Process) {
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