// Copyright (c) Microsoft Corporation
// SPDX-License-Identifier: MIT
#pragma once

#include "platform.h"

#include <limits.h>
#include <stdint.h>
#define WIN32_LEAN_AND_MEAN // Exclude rarely-used stuff from Windows headers
// Windows Header Files
#include <windows.h>

// Convert a Win32 failure error code to an NTSTATUS failure.
_Ret_range_(LONG_MIN, -1) __forceinline usersim_result_t win32_error_to_usersim_error(uint32_t error)
{
    usersim_result_t result;

    switch (error) {
    case ERROR_OUTOFMEMORY:
    case ERROR_NOT_ENOUGH_MEMORY:
        result = STATUS_NO_MEMORY;
        break;

    case ERROR_PATH_NOT_FOUND:
        result = STATUS_OBJECT_PATH_NOT_FOUND;
        break;

    case ERROR_NOT_FOUND:
        result = STATUS_NOT_FOUND;
        break;

    case ERROR_INVALID_PARAMETER:
        result = STATUS_INVALID_PARAMETER;
        break;

    case ERROR_NO_MORE_ITEMS:
    case ERROR_NO_MORE_MATCHES:
        result = STATUS_NO_MORE_MATCHES;
        break;

    case ERROR_INVALID_HANDLE:
        result = STATUS_INVALID_HANDLE;
        break;

    case ERROR_NOT_SUPPORTED:
        result = STATUS_NOT_SUPPORTED;
        break;

    case ERROR_MORE_DATA:
        result = STATUS_BUFFER_OVERFLOW;
        break;

    case ERROR_FILE_NOT_FOUND:
        result = STATUS_NO_SUCH_FILE;
        break;

    case ERROR_ALREADY_INITIALIZED:
        result = STATUS_ALREADY_INITIALIZED;
        break;

    case ERROR_OBJECT_ALREADY_EXISTS:
        result = STATUS_OBJECTID_EXISTS;
        break;

    case ERROR_VERIFIER_STOP:
        result = STATUS_VERIFIER_STOP;
        break;

    case ERROR_NONE_MAPPED:
        result = STATUS_NONE_MAPPED;
        break;

    case ERROR_BAD_DRIVER:
        result = STATUS_DRIVER_UNABLE_TO_LOAD;
        break;

    case ERROR_INVALID_FUNCTION:
        result = STATUS_INVALID_DEVICE_REQUEST;
        break;

    case ERROR_TOO_MANY_CMDS:
        result = STATUS_TOO_MANY_COMMANDS;
        break;

    case RPC_S_CALL_FAILED:
        result = RPC_NT_CALL_FAILED;
        break;

    case ERROR_BAD_EXE_FORMAT:
        result = STATUS_INVALID_IMAGE_FORMAT;
        break;

    case ERROR_ACCESS_DENIED:
        result = STATUS_ACCESS_DENIED;
        break;

    case ERROR_NOT_OWNER:
        result = STATUS_RESOURCE_NOT_OWNED;
        break;

    case ERROR_CONTENT_BLOCKED:
        result = STATUS_CONTENT_BLOCKED;
        break;

    case ERROR_ARITHMETIC_OVERFLOW:
        result = STATUS_INTEGER_OVERFLOW;
        break;

    case ERROR_GENERIC_COMMAND_FAILED:
        result = STATUS_GENERIC_COMMAND_FAILED;
        break;

    case ERROR_ALREADY_REGISTERED:
        // Currently STATUS_ALREADY_REGISTERED is mapped to
        // ERROR_INTERNAL_ERROR instead of ERROR_ALREADY_REGISTERED.
    case ERROR_INTERNAL_ERROR:
        result = STATUS_ALREADY_REGISTERED;
        break;

    case ERROR_TOO_MANY_NAMES:
        result = STATUS_TOO_MANY_NODES;
        break;

    case ERROR_NO_SYSTEM_RESOURCES:
        result = STATUS_INSUFFICIENT_RESOURCES;
        break;

    case ERROR_OPERATION_ABORTED:
        result = STATUS_CANCELLED;
        break;

    case ERROR_NOACCESS:
        result = STATUS_ACCESS_VIOLATION;
        break;

    default:
        result = STATUS_UNSUCCESSFUL;
        break;
    }

    return result;
}

// Convert a Win32 error code (whether success or failure) to an NTSTATUS value.
_When_(
    error != ERROR_SUCCESS && error != ERROR_IO_PENDING && error != ERROR_OBJECT_NAME_EXISTS,
    _Ret_range_(LONG_MIN, -1)) __forceinline usersim_result_t win32_error_code_to_usersim_result(uint32_t error)
{
    usersim_result_t result;

    switch (error) {
    case ERROR_SUCCESS:
        result = STATUS_SUCCESS;
        break;

    case ERROR_IO_PENDING:
        result = STATUS_PENDING;
        break;

    case ERROR_OBJECT_NAME_EXISTS:
        result = STATUS_OBJECT_NAME_EXISTS;
        break;

    default:
        result = win32_error_to_usersim_error(error);
        break;
    }

    return result;
}
