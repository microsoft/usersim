// Copyright (c) Microsoft Corporation
// SPDX-License-Identifier: MIT

#include "platform.h"
#include "kernel_um.h"
#include "usersim/ps.h"

// Ps* functions.

HANDLE
PsGetCurrentProcessId() { return (HANDLE)(uintptr_t)GetCurrentProcessId(); }

_IRQL_requires_max_(DISPATCH_LEVEL) NTKERNELAPI HANDLE
PsGetCurrentThreadId() { return (HANDLE)(uintptr_t)GetCurrentThreadId(); }