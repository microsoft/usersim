// Copyright (c) Microsoft Corporation
// SPDX-License-Identifier: MIT

#pragma once
#include "..\src\framework.h"

#if defined(__cplusplus)
extern "C"
{
#endif

    typedef struct _UNICODE_STRING
    {
        USHORT Length;
        USHORT MaximumLength;
        _Field_size_bytes_part_opt_(MaximumLength, Length) PWCH Buffer;
    } UNICODE_STRING, *PUNICODE_STRING;
    typedef const UNICODE_STRING* PCUNICODE_STRING;

    typedef struct _driver_object DRIVER_OBJECT, *PDRIVER_OBJECT;
#define DPFLTR_IHVDRIVER_ID 0
#define DPFLTR_INFO_LEVEL 0

    typedef NTSTATUS(DRIVER_INITIALIZE)(_In_ PDRIVER_OBJECT driver_object, _In_ PUNICODE_STRING registry_path);

#if defined(__cplusplus)
}
#endif