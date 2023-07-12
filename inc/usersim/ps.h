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

    __declspec(dllexport)
	HANDLE
    PsGetCurrentProcessId();

    __declspec(dllexport)
    _IRQL_requires_max_(DISPATCH_LEVEL) NTKERNELAPI HANDLE PsGetCurrentThreadId();

#if defined(__cplusplus)
}
#endif
