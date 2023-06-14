// Copyright (c) Microsoft Corporation
// SPDX-License-Identifier: MIT

#include "platform.h"
#include "kernel_um.h"
#include "usersim/ps.h"

// Ps* functions.

HANDLE
PsGetCurrentProcessId() { return (HANDLE)(uintptr_t)GetCurrentProcessId(); }

HANDLE
PsGetCurrentThreadId() { return (HANDLE)(uintptr_t)GetCurrentThreadId(); }