// Copyright (c) Microsoft Corporation
// SPDX-License-Identifier: MIT

#pragma once
#include "..\src\platform.h"
#include "usersim\io.h"

CXPLAT_EXTERN_C_BEGIN

#define PsGetCurrentProcess IoGetCurrentProcess

USERSIM_API
HANDLE
PsGetCurrentProcessId();

USERSIM_API
_IRQL_requires_max_(DISPATCH_LEVEL) NTKERNELAPI HANDLE PsGetCurrentThreadId();

CXPLAT_EXTERN_C_END
