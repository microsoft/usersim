// Copyright (c) Microsoft Corporation
// SPDX-License-Identifier: MIT

#pragma once
#include "..\src\platform.h"
#include "usersim\io.h"

#if defined(__cplusplus)
extern "C"
{
#endif

    #define PsGetCurrentProcess IoGetCurrentProcess

    USERSIM_API
	HANDLE
    PsGetCurrentProcessId();

    USERSIM_API
    _IRQL_requires_max_(DISPATCH_LEVEL) NTKERNELAPI HANDLE PsGetCurrentThreadId();

#if defined(__cplusplus)
}
#endif
