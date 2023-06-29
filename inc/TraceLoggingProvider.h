// Copyright (c) Microsoft Corporation
// SPDX-License-Identifier: MIT
#pragma once
#include <sal.h>

// Include original TraceLoggingProvider.h
#include "../shared/TraceLoggingProvider.h"

#ifndef NO_TRACE_LOGGING_OVERRIDE

#ifdef __cplusplus
extern "C"
{
#endif

    // Data item type values.
    typedef enum
    {
        _tlgLevel = 0,
        _tlgKeyword,
        _tlgPsz,
        _tlgScalarVal,
        _tlgNTStatus,
        _tlgCountedUtf8String,
        _tlgPwsz,
        _tlgGuid,
        _tlgUInt64,
        _tlgUInt32,
        _tlgInt32,
        _tlgLong,
        _tlgWinError,
        _tlgOpcode,
        _tlgPointer,
        _tlgBool,
    } usersim_tlg_type_t;

    void
    usersim_trace_logging_write(_In_ const TraceLoggingHProvider hProvider, _In_z_ const char* event_name, size_t argc, ...);

    BOOLEAN
    usersim_trace_logging_provider_enabled(
        _In_ const TraceLoggingHProvider hProvider, UCHAR event_level, ULONGLONG event_keyword);

    void
    usersim_trace_logging_set_enabled(bool enabled, UCHAR event_level, ULONGLONG event_keyword);

#define USERSIM_GET_NTH_ARG(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, N, ...) N
#define USERSIM_APPEND(X, Y) X Y
#define USERSIM_VA_ARGS(...) __VA_ARGS__
#define USERSIM_COUNT_VA_ARGS(...) USERSIM_VA_ARGS(USERSIM_GET_NTH_ARG(__VA_ARGS__, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0))
#define USERSIM_VA_ARGC(...) USERSIM_APPEND(USERSIM_COUNT_VA_ARGS, (, __VA_ARGS__))

// Redefine calls we want to override
#undef TraceLoggingWrite
#define TraceLoggingWrite(hProvider, eventName, ...) \
    usersim_trace_logging_write(hProvider, eventName, USERSIM_VA_ARGC(__VA_ARGS__), __VA_ARGS__)

#undef TraceLoggingNTStatus
#define TraceLoggingNTStatus(...) _tlgNTStatus, USERSIM_VA_ARGC(__VA_ARGS__), __VA_ARGS__

#undef TraceLoggingString
#define TraceLoggingString(...) _tlgPsz, USERSIM_VA_ARGC(__VA_ARGS__), __VA_ARGS__

#undef TraceLoggingWideString
#define TraceLoggingWideString(...) _tlgPwsz, USERSIM_VA_ARGC(__VA_ARGS__), __VA_ARGS__

#undef TraceLoggingCountedUtf8String
#define TraceLoggingCountedUtf8String(pchValue, cchValue, ...) \
    _tlgCountedUtf8String, pchValue, cchValue, USERSIM_VA_ARGC(__VA_ARGS__), __VA_ARGS__

#undef TraceLoggingGuid
#define TraceLoggingGuid(...) _tlgGuid, USERSIM_VA_ARGC(__VA_ARGS__), __VA_ARGS__

#undef TraceLoggingUInt64
#define TraceLoggingUInt64(...) _tlgUInt64, USERSIM_VA_ARGC(__VA_ARGS__), __VA_ARGS__

#undef TraceLoggingUInt32
#define TraceLoggingUInt32(...) _tlgUInt32, USERSIM_VA_ARGC(__VA_ARGS__), __VA_ARGS__

#undef TraceLoggingInt32
#define TraceLoggingInt32(...) _tlgInt32, USERSIM_VA_ARGC(__VA_ARGS__), __VA_ARGS__

#undef TraceLoggingLong
#define TraceLoggingLong(...) _tlgLong, USERSIM_VA_ARGC(__VA_ARGS__), __VA_ARGS__

#undef TraceLoggingWinError
#define TraceLoggingWinError(...) _tlgWinError, USERSIM_VA_ARGC(__VA_ARGS__), __VA_ARGS__

#undef TraceLoggingLevel
#define TraceLoggingLevel(level) _tlgLevel, level

#undef TraceLoggingKeyword
#define TraceLoggingKeyword(keyword) _tlgKeyword, keyword

#undef TraceLoggingOpcode
#define TraceLoggingOpcode(opcode) _tlgOpcode, opcode

#undef TraceLoggingPointer
#define TraceLoggingPointer(...) _tlgPointer, USERSIM_VA_ARGC(__VA_ARGS__), __VA_ARGS__

#undef TraceLoggingBool
#define TraceLoggingBool(...) _tlgBool, USERSIM_VA_ARGC(__VA_ARGS__), __VA_ARGS__

#undef TraceLoggingProviderEnabled
#define TraceLoggingProviderEnabled(hProvider, eventLevel, eventKeyword) \
    usersim_trace_logging_provider_enabled(hProvider, eventLevel, eventKeyword)

#define _tlgWriteTransfer_EtwWriteTransfer(...)

#ifdef __cplusplus
}
#endif
#endif // NO_TRACE_LOGGING_OVERRIDE