// Copyright (c) Microsoft Corporation
// SPDX-License-Identifier: MIT

#include "platform.h"
#include "tracelog.h"

#include <TraceLoggingProvider.h>
#include <winmeta.h>

TRACELOGGING_DEFINE_PROVIDER(
    usersim_tracelog_provider,
    "UsersimProvider",
    // {d1559e65-8d36-46b4-9e4e-977299f8b6f7}
    (0xd1559e65, 0x8d36, 0x46b4, 0x9e, 0x4e, 0x97, 0x72, 0x99, 0xf8, 0xb6, 0xf7));

static bool _usersim_trace_initiated = false;

_Must_inspect_result_ NTSTATUS
usersim_trace_initiate()
{
    if (_usersim_trace_initiated) {
        return STATUS_SUCCESS;
    }
    TLG_STATUS status = TraceLoggingRegister(usersim_tracelog_provider);
    if (status != 0) {
        return STATUS_NO_MEMORY;
    } else {
        _usersim_trace_initiated = true;
        return STATUS_SUCCESS;
    }
}

// Prevent tail call optimization of the call to TraceLoggingUnregister to resolve driver verifier stop C4/DD
// "An attempt was made to unload a driver without calling EtwUnregister".
#pragma optimize("", off)
void
usersim_trace_terminate()
{
    if (_usersim_trace_initiated) {
        TraceLoggingUnregister(usersim_tracelog_provider);
        _usersim_trace_initiated = false;
    }
}
#pragma optimize("", on)

// Keyword definitions
#define KEYWORD_FUNCTION_ENTRY_EXIT USERSIM_TRACELOG_KEYWORD_FUNCTION_ENTRY_EXIT
#define KEYWORD_BASE USERSIM_TRACELOG_KEYWORD_BASE
#define KEYWORD_ERROR USERSIM_TRACELOG_KEYWORD_ERROR
#define KEYWORD_EPOCH USERSIM_TRACELOG_KEYWORD_EPOCH
#define KEYWORD_CORE USERSIM_TRACELOG_KEYWORD_CORE
#define KEYWORD_LINK USERSIM_TRACELOG_KEYWORD_LINK
#define KEYWORD_MAP USERSIM_TRACELOG_KEYWORD_MAP
#define KEYWORD_PROGRAM USERSIM_TRACELOG_KEYWORD_PROGRAM
#define KEYWORD_API USERSIM_TRACELOG_KEYWORD_API
#define KEYWORD_PRINTK USERSIM_TRACELOG_KEYWORD_PRINTK
#define KEYWORD_NATIVE USERSIM_TRACELOG_KEYWORD_NATIVE

// Keyword cases for switch macros
#define CASE_FUNCTION_ENTRY_EXIT case _USERSIM_TRACELOG_KEYWORD_FUNCTION_ENTRY_EXIT
#define CASE_BASE case _USERSIM_TRACELOG_KEYWORD_BASE
#define CASE_ERROR case _USERSIM_TRACELOG_KEYWORD_ERROR
#define CASE_EPOCH case _USERSIM_TRACELOG_KEYWORD_EPOCH
#define CASE_CORE case _USERSIM_TRACELOG_KEYWORD_CORE
#define CASE_LINK case _USERSIM_TRACELOG_KEYWORD_LINK
#define CASE_MAP case _USERSIM_TRACELOG_KEYWORD_MAP
#define CASE_PROGRAM case _USERSIM_TRACELOG_KEYWORD_PROGRAM
#define CASE_API case _USERSIM_TRACELOG_KEYWORD_API
#define CASE_PRINTK case _USERSIM_TRACELOG_KEYWORD_PRINTK
#define CASE_NATIVE case _USERSIM_TRACELOG_KEYWORD_NATIVE

// Log level definitions
#define LEVEL_LOG_ALWAYS USERSIM_TRACELOG_LEVEL_LOG_ALWAYS
#define LEVEL_CRITICAL USERSIM_TRACELOG_LEVEL_CRITICAL
#define LEVEL_ERROR USERSIM_TRACELOG_LEVEL_ERROR
#define LEVEL_WARNING USERSIM_TRACELOG_LEVEL_WARNING
#define LEVEL_INFO USERSIM_TRACELOG_LEVEL_INFO
#define LEVEL_VERBOSE USERSIM_TRACELOG_LEVEL_VERBOSE

// Log level cases for switch macros
#define CASE_LOG_ALWAYS case USERSIM_TRACELOG_LEVEL_LOG_ALWAYS
#define CASE_CRITICAL case USERSIM_TRACELOG_LEVEL_CRITICAL
#define CASE_LEVEL_ERROR case USERSIM_TRACELOG_LEVEL_ERROR
#define CASE_WARNING case USERSIM_TRACELOG_LEVEL_WARNING
#define CASE_INFO case USERSIM_TRACELOG_LEVEL_INFO
#define CASE_VERBOSE case USERSIM_TRACELOG_LEVEL_VERBOSE

#pragma warning(push)
#pragma warning(disable : 6262) // Function uses 'N' bytes of stack.  Consider moving some data to heap.

#define _USERSIM_LOG_NTSTATUS_API_FAILURE(keyword, api, status) \
    TraceLoggingWrite(                                       \
        usersim_tracelog_provider,                              \
        USERSIM_TRACELOG_EVENT_API_ERROR,                       \
        TraceLoggingLevel(WINEVENT_LEVEL_ERROR),             \
        TraceLoggingKeyword((keyword)),                      \
        TraceLoggingString(api, "Api"),                      \
        TraceLoggingNTStatus(status));
#define USERSIM_LOG_NTSTATUS_API_FAILURE_KEYWORD_SWITCH(api_name, status)                 \
    switch (keyword) {                                                                 \
    CASE_FUNCTION_ENTRY_EXIT:                                                          \
        _USERSIM_LOG_NTSTATUS_API_FAILURE(KEYWORD_FUNCTION_ENTRY_EXIT, api_name, status); \
        break;                                                                         \
    CASE_BASE:                                                                         \
        _USERSIM_LOG_NTSTATUS_API_FAILURE(KEYWORD_BASE, api_name, status);                \
        break;                                                                         \
    CASE_ERROR:                                                                        \
        _USERSIM_LOG_NTSTATUS_API_FAILURE(KEYWORD_ERROR, api_name, status);               \
        break;                                                                         \
    CASE_EPOCH:                                                                        \
        _USERSIM_LOG_NTSTATUS_API_FAILURE(KEYWORD_EPOCH, api_name, status);               \
        break;                                                                         \
    CASE_CORE:                                                                         \
        _USERSIM_LOG_NTSTATUS_API_FAILURE(KEYWORD_CORE, api_name, status);                \
        break;                                                                         \
    CASE_LINK:                                                                         \
        _USERSIM_LOG_NTSTATUS_API_FAILURE(KEYWORD_LINK, api_name, status);                \
        break;                                                                         \
    CASE_MAP:                                                                          \
        _USERSIM_LOG_NTSTATUS_API_FAILURE(KEYWORD_MAP, api_name, status);                 \
        break;                                                                         \
    CASE_PROGRAM:                                                                      \
        _USERSIM_LOG_NTSTATUS_API_FAILURE(KEYWORD_PROGRAM, api_name, status);             \
        break;                                                                         \
    CASE_API:                                                                          \
        _USERSIM_LOG_NTSTATUS_API_FAILURE(KEYWORD_API, api_name, status);                 \
        break;                                                                         \
    CASE_PRINTK:                                                                       \
        _USERSIM_LOG_NTSTATUS_API_FAILURE(KEYWORD_PRINTK, api_name, status);              \
        break;                                                                         \
    CASE_NATIVE:                                                                       \
        _USERSIM_LOG_NTSTATUS_API_FAILURE(KEYWORD_NATIVE, api_name, status);              \
        break;                                                                         \
    default:                                                                           \
        break;                                                                         \
    }
__declspec(noinline) void usersim_log_ntstatus_api_failure(
    usersim_tracelog_keyword_t keyword, _In_z_ const char* api_name, NTSTATUS status)
{
    USERSIM_LOG_NTSTATUS_API_FAILURE_KEYWORD_SWITCH(api_name, status);
}

#define _USERSIM_LOG_NTSTATUS_API_FAILURE_MESSAGE(keyword, api, status, message) \
    TraceLoggingWrite(                                                        \
        usersim_tracelog_provider,                                               \
        USERSIM_TRACELOG_EVENT_API_ERROR,                                        \
        TraceLoggingLevel(WINEVENT_LEVEL_ERROR),                              \
        TraceLoggingKeyword((keyword)),                                       \
        TraceLoggingString(api, "api"),                                       \
        TraceLoggingNTStatus(status),                                         \
        TraceLoggingString(message, "Message"));
#define _USERSIM_LOG_NTSTATUS_API_FAILURE_MESSAGE_STRING_KEYWORD_SWITCH(api_name, status, message)         \
    switch (keyword) {                                                                                  \
    CASE_FUNCTION_ENTRY_EXIT:                                                                           \
        _USERSIM_LOG_NTSTATUS_API_FAILURE_MESSAGE(KEYWORD_FUNCTION_ENTRY_EXIT, api_name, status, message); \
        break;                                                                                          \
    CASE_BASE:                                                                                          \
        _USERSIM_LOG_NTSTATUS_API_FAILURE_MESSAGE(KEYWORD_BASE, api_name, status, message);                \
        break;                                                                                          \
    CASE_ERROR:                                                                                         \
        _USERSIM_LOG_NTSTATUS_API_FAILURE_MESSAGE(KEYWORD_ERROR, api_name, status, message);               \
        break;                                                                                          \
    CASE_EPOCH:                                                                                         \
        _USERSIM_LOG_NTSTATUS_API_FAILURE_MESSAGE(KEYWORD_EPOCH, api_name, status, message);               \
        break;                                                                                          \
    CASE_CORE:                                                                                          \
        _USERSIM_LOG_NTSTATUS_API_FAILURE_MESSAGE(KEYWORD_CORE, api_name, status, message);                \
        break;                                                                                          \
    CASE_LINK:                                                                                          \
        _USERSIM_LOG_NTSTATUS_API_FAILURE_MESSAGE(KEYWORD_LINK, api_name, status, message);                \
        break;                                                                                          \
    CASE_MAP:                                                                                           \
        _USERSIM_LOG_NTSTATUS_API_FAILURE_MESSAGE(KEYWORD_MAP, api_name, status, message);                 \
        break;                                                                                          \
    CASE_PROGRAM:                                                                                       \
        _USERSIM_LOG_NTSTATUS_API_FAILURE_MESSAGE(KEYWORD_PROGRAM, api_name, status, message);             \
        break;                                                                                          \
    CASE_API:                                                                                           \
        _USERSIM_LOG_NTSTATUS_API_FAILURE_MESSAGE(KEYWORD_API, api_name, status, message);                 \
        break;                                                                                          \
    CASE_PRINTK:                                                                                        \
        _USERSIM_LOG_NTSTATUS_API_FAILURE_MESSAGE(KEYWORD_PRINTK, api_name, status, message);              \
        break;                                                                                          \
    CASE_NATIVE:                                                                                        \
        _USERSIM_LOG_NTSTATUS_API_FAILURE_MESSAGE(KEYWORD_NATIVE, api_name, status, message);              \
        break;                                                                                          \
    default:                                                                                            \
        break;                                                                                          \
    }
__declspec(noinline) void usersim_log_ntstatus_api_failure_message(
    usersim_tracelog_keyword_t keyword, _In_z_ const char* api_name, NTSTATUS status, _In_z_ const char* message)
{
    _USERSIM_LOG_NTSTATUS_API_FAILURE_MESSAGE_STRING_KEYWORD_SWITCH(api_name, status, message);
}

#define _USERSIM_LOG_MESSAGE(trace_level, keyword, message) \
    TraceLoggingWrite(                                   \
        usersim_tracelog_provider,                          \
        USERSIM_TRACELOG_EVENT_GENERIC_MESSAGE,             \
        TraceLoggingLevel(trace_level),                  \
        TraceLoggingKeyword((keyword)),                  \
        TraceLoggingString(message, "Message"));
#define USERSIM_LOG_MESSAGE_KEYWORD_SWITCH(trace_level, message)                 \
    switch (keyword) {                                                        \
    CASE_FUNCTION_ENTRY_EXIT:                                                 \
        _USERSIM_LOG_MESSAGE(trace_level, KEYWORD_FUNCTION_ENTRY_EXIT, message); \
        break;                                                                \
    CASE_BASE:                                                                \
        _USERSIM_LOG_MESSAGE(trace_level, KEYWORD_BASE, message);                \
        break;                                                                \
    CASE_ERROR:                                                               \
        _USERSIM_LOG_MESSAGE(trace_level, KEYWORD_ERROR, message);               \
        break;                                                                \
    CASE_EPOCH:                                                               \
        _USERSIM_LOG_MESSAGE(trace_level, KEYWORD_EPOCH, message);               \
        break;                                                                \
    CASE_CORE:                                                                \
        _USERSIM_LOG_MESSAGE(trace_level, KEYWORD_CORE, message);                \
        break;                                                                \
    CASE_LINK:                                                                \
        _USERSIM_LOG_MESSAGE(trace_level, KEYWORD_LINK, message);                \
        break;                                                                \
    CASE_MAP:                                                                 \
        _USERSIM_LOG_MESSAGE(trace_level, KEYWORD_MAP, message);                 \
        break;                                                                \
    CASE_PROGRAM:                                                             \
        _USERSIM_LOG_MESSAGE(trace_level, KEYWORD_PROGRAM, message);             \
        break;                                                                \
    CASE_API:                                                                 \
        _USERSIM_LOG_MESSAGE(trace_level, KEYWORD_API, message);                 \
        break;                                                                \
    CASE_PRINTK:                                                              \
        _USERSIM_LOG_MESSAGE(trace_level, KEYWORD_PRINTK, message);              \
        break;                                                                \
    CASE_NATIVE:                                                              \
        _USERSIM_LOG_MESSAGE(trace_level, KEYWORD_NATIVE, message);              \
        break;                                                                \
    default:                                                                  \
        break;                                                                \
    }
__declspec(noinline) void usersim_log_message(
    usersim_tracelog_level_t trace_level, usersim_tracelog_keyword_t keyword, _In_z_ const char* message)
{
    switch (trace_level) {
    CASE_LOG_ALWAYS:
        USERSIM_LOG_MESSAGE_KEYWORD_SWITCH(LEVEL_LOG_ALWAYS, message);
        break;
    CASE_CRITICAL:
        USERSIM_LOG_MESSAGE_KEYWORD_SWITCH(LEVEL_CRITICAL, message);
        break;
    CASE_LEVEL_ERROR:
        USERSIM_LOG_MESSAGE_KEYWORD_SWITCH(LEVEL_ERROR, message);
        break;
    CASE_WARNING:
        USERSIM_LOG_MESSAGE_KEYWORD_SWITCH(LEVEL_WARNING, message);
        break;
    CASE_INFO:
        USERSIM_LOG_MESSAGE_KEYWORD_SWITCH(LEVEL_INFO, message);
        break;
    CASE_VERBOSE:
        USERSIM_LOG_MESSAGE_KEYWORD_SWITCH(LEVEL_VERBOSE, message);
        break;
    }
}

#define _USERSIM_LOG_MESSAGE_STRING(trace_level, keyword, message, string) \
    TraceLoggingWrite(                                                  \
        usersim_tracelog_provider,                                         \
        USERSIM_TRACELOG_EVENT_GENERIC_MESSAGE,                            \
        TraceLoggingLevel(trace_level),                                 \
        TraceLoggingKeyword((keyword)),                                 \
        TraceLoggingString(message, "Message"),                         \
        TraceLoggingString(string, #string));
#define USERSIM_LOG_MESSAGE_STRING_KEYWORD_SWITCH(trace_level, message, string_value)                 \
    switch (keyword) {                                                                             \
    CASE_FUNCTION_ENTRY_EXIT:                                                                      \
        _USERSIM_LOG_MESSAGE_STRING(trace_level, KEYWORD_FUNCTION_ENTRY_EXIT, message, string_value); \
        break;                                                                                     \
    CASE_BASE:                                                                                     \
        _USERSIM_LOG_MESSAGE_STRING(trace_level, KEYWORD_BASE, message, string_value);                \
        break;                                                                                     \
    CASE_ERROR:                                                                                    \
        _USERSIM_LOG_MESSAGE_STRING(trace_level, KEYWORD_ERROR, message, string_value);               \
        break;                                                                                     \
    CASE_EPOCH:                                                                                    \
        _USERSIM_LOG_MESSAGE_STRING(trace_level, KEYWORD_EPOCH, message, string_value);               \
        break;                                                                                     \
    CASE_CORE:                                                                                     \
        _USERSIM_LOG_MESSAGE_STRING(trace_level, KEYWORD_CORE, message, string_value);                \
        break;                                                                                     \
    CASE_LINK:                                                                                     \
        _USERSIM_LOG_MESSAGE_STRING(trace_level, KEYWORD_LINK, message, string_value);                \
        break;                                                                                     \
    CASE_MAP:                                                                                      \
        _USERSIM_LOG_MESSAGE_STRING(trace_level, KEYWORD_MAP, message, string_value);                 \
        break;                                                                                     \
    CASE_PROGRAM:                                                                                  \
        _USERSIM_LOG_MESSAGE_STRING(trace_level, KEYWORD_PROGRAM, message, string_value);             \
        break;                                                                                     \
    CASE_API:                                                                                      \
        _USERSIM_LOG_MESSAGE_STRING(trace_level, KEYWORD_API, message, string_value);                 \
        break;                                                                                     \
    CASE_PRINTK:                                                                                   \
        _USERSIM_LOG_MESSAGE_STRING(trace_level, KEYWORD_PRINTK, message, string_value);              \
        break;                                                                                     \
    CASE_NATIVE:                                                                                   \
        _USERSIM_LOG_MESSAGE_STRING(trace_level, KEYWORD_NATIVE, message, string_value);              \
        break;                                                                                     \
    default:                                                                                       \
        break;                                                                                     \
    }
__declspec(noinline) void usersim_log_message_string(
    usersim_tracelog_level_t trace_level,
    usersim_tracelog_keyword_t keyword,
    _In_z_ const char* message,
    _In_z_ const char* string_value)
{
    switch (trace_level) {
    CASE_LOG_ALWAYS:
        USERSIM_LOG_MESSAGE_STRING_KEYWORD_SWITCH(LEVEL_LOG_ALWAYS, message, string_value);
        break;
    CASE_CRITICAL:
        USERSIM_LOG_MESSAGE_STRING_KEYWORD_SWITCH(LEVEL_CRITICAL, message, string_value);
        break;
    CASE_LEVEL_ERROR:
        USERSIM_LOG_MESSAGE_STRING_KEYWORD_SWITCH(LEVEL_ERROR, message, string_value);
        break;
    CASE_WARNING:
        USERSIM_LOG_MESSAGE_STRING_KEYWORD_SWITCH(LEVEL_WARNING, message, string_value);
        break;
    CASE_INFO:
        USERSIM_LOG_MESSAGE_STRING_KEYWORD_SWITCH(LEVEL_INFO, message, string_value);
        break;
    CASE_VERBOSE:
        USERSIM_LOG_MESSAGE_STRING_KEYWORD_SWITCH(LEVEL_VERBOSE, message, string_value);
        break;
    }
}

#define _USERSIM_LOG_MESSAGE_UTF8_STRING(trace_level, keyword, message, string) \
    TraceLoggingWrite(                                                       \
        usersim_tracelog_provider,                                              \
        USERSIM_TRACELOG_EVENT_GENERIC_MESSAGE,                                 \
        TraceLoggingLevel(trace_level),                                      \
        TraceLoggingKeyword((keyword)),                                      \
        TraceLoggingString(message, "Message"),                              \
        TraceLoggingCountedUtf8String((const char*)(string)->value, (unsigned long)(string)->length, #string));
#define USERSIM_LOG_MESSAGE_UTF8_STRING_KEYWORD_SWITCH(trace_level, message, string_value)                 \
    switch (keyword) {                                                                                  \
    CASE_FUNCTION_ENTRY_EXIT:                                                                           \
        _USERSIM_LOG_MESSAGE_UTF8_STRING(trace_level, KEYWORD_FUNCTION_ENTRY_EXIT, message, string_value); \
        break;                                                                                          \
    CASE_BASE:                                                                                          \
        _USERSIM_LOG_MESSAGE_UTF8_STRING(trace_level, KEYWORD_BASE, message, string_value);                \
        break;                                                                                          \
    CASE_ERROR:                                                                                         \
        _USERSIM_LOG_MESSAGE_UTF8_STRING(trace_level, KEYWORD_ERROR, message, string_value);               \
        break;                                                                                          \
    CASE_EPOCH:                                                                                         \
        _USERSIM_LOG_MESSAGE_UTF8_STRING(trace_level, KEYWORD_EPOCH, message, string_value);               \
        break;                                                                                          \
    CASE_CORE:                                                                                          \
        _USERSIM_LOG_MESSAGE_UTF8_STRING(trace_level, KEYWORD_CORE, message, string_value);                \
        break;                                                                                          \
    CASE_LINK:                                                                                          \
        _USERSIM_LOG_MESSAGE_UTF8_STRING(trace_level, KEYWORD_LINK, message, string_value);                \
        break;                                                                                          \
    CASE_MAP:                                                                                           \
        _USERSIM_LOG_MESSAGE_UTF8_STRING(trace_level, KEYWORD_MAP, message, string_value);                 \
        break;                                                                                          \
    CASE_PROGRAM:                                                                                       \
        _USERSIM_LOG_MESSAGE_UTF8_STRING(trace_level, KEYWORD_PROGRAM, message, string_value);             \
        break;                                                                                          \
    CASE_API:                                                                                           \
        _USERSIM_LOG_MESSAGE_UTF8_STRING(trace_level, KEYWORD_API, message, string_value);                 \
        break;                                                                                          \
    CASE_PRINTK:                                                                                        \
        _USERSIM_LOG_MESSAGE_UTF8_STRING(trace_level, KEYWORD_PRINTK, message, string_value);              \
        break;                                                                                          \
    CASE_NATIVE:                                                                                        \
        _USERSIM_LOG_MESSAGE_UTF8_STRING(trace_level, KEYWORD_NATIVE, message, string_value);              \
        break;                                                                                          \
    default:                                                                                            \
        break;                                                                                          \
    }
__declspec(noinline) void usersim_log_message_utf8_string(
    usersim_tracelog_level_t trace_level,
    usersim_tracelog_keyword_t keyword,
    _In_z_ const char* message,
    _In_ const usersim_utf8_string_t* string)
{
    switch (trace_level) {
    CASE_LOG_ALWAYS:
        USERSIM_LOG_MESSAGE_UTF8_STRING_KEYWORD_SWITCH(LEVEL_LOG_ALWAYS, message, string);
        break;
    CASE_CRITICAL:
        USERSIM_LOG_MESSAGE_UTF8_STRING_KEYWORD_SWITCH(LEVEL_CRITICAL, message, string);
        break;
    CASE_LEVEL_ERROR:
        USERSIM_LOG_MESSAGE_UTF8_STRING_KEYWORD_SWITCH(LEVEL_ERROR, message, string);
        break;
    CASE_WARNING:
        USERSIM_LOG_MESSAGE_UTF8_STRING_KEYWORD_SWITCH(LEVEL_WARNING, message, string);
        break;
    CASE_INFO:
        USERSIM_LOG_MESSAGE_UTF8_STRING_KEYWORD_SWITCH(LEVEL_INFO, message, string);
        break;
    CASE_VERBOSE:
        USERSIM_LOG_MESSAGE_UTF8_STRING_KEYWORD_SWITCH(LEVEL_VERBOSE, message, string);
        break;
    }
}

#define _USERSIM_LOG_MESSAGE_WSTRING(trace_level, keyword, message, wstring) \
    TraceLoggingWrite(                                                    \
        usersim_tracelog_provider,                                           \
        USERSIM_TRACELOG_EVENT_GENERIC_MESSAGE,                              \
        TraceLoggingLevel(trace_level),                                   \
        TraceLoggingKeyword((keyword)),                                   \
        TraceLoggingString(message, "Message"),                           \
        TraceLoggingWideString(wstring, #wstring));
#define USERSIM_LOG_MESSAGE_WSTRING_KEYWORD_SWITCH(trace_level, message, wstring)                 \
    switch (keyword) {                                                                         \
    CASE_FUNCTION_ENTRY_EXIT:                                                                  \
        _USERSIM_LOG_MESSAGE_WSTRING(trace_level, KEYWORD_FUNCTION_ENTRY_EXIT, message, wstring); \
        break;                                                                                 \
    CASE_BASE:                                                                                 \
        _USERSIM_LOG_MESSAGE_WSTRING(trace_level, KEYWORD_BASE, message, wstring);                \
        break;                                                                                 \
    CASE_ERROR:                                                                                \
        _USERSIM_LOG_MESSAGE_WSTRING(trace_level, KEYWORD_ERROR, message, wstring);               \
        break;                                                                                 \
    CASE_EPOCH:                                                                                \
        _USERSIM_LOG_MESSAGE_WSTRING(trace_level, KEYWORD_EPOCH, message, wstring);               \
        break;                                                                                 \
    CASE_CORE:                                                                                 \
        _USERSIM_LOG_MESSAGE_WSTRING(trace_level, KEYWORD_CORE, message, wstring);                \
        break;                                                                                 \
    CASE_LINK:                                                                                 \
        _USERSIM_LOG_MESSAGE_WSTRING(trace_level, KEYWORD_LINK, message, wstring);                \
        break;                                                                                 \
    CASE_MAP:                                                                                  \
        _USERSIM_LOG_MESSAGE_WSTRING(trace_level, KEYWORD_MAP, message, wstring);                 \
        break;                                                                                 \
    CASE_PROGRAM:                                                                              \
        _USERSIM_LOG_MESSAGE_WSTRING(trace_level, KEYWORD_PROGRAM, message, wstring);             \
        break;                                                                                 \
    CASE_API:                                                                                  \
        _USERSIM_LOG_MESSAGE_WSTRING(trace_level, KEYWORD_API, message, wstring);                 \
        break;                                                                                 \
    CASE_PRINTK:                                                                               \
        _USERSIM_LOG_MESSAGE_WSTRING(trace_level, KEYWORD_PRINTK, message, wstring);              \
        break;                                                                                 \
    CASE_NATIVE:                                                                               \
        _USERSIM_LOG_MESSAGE_WSTRING(trace_level, KEYWORD_NATIVE, message, wstring);              \
        break;                                                                                 \
    default:                                                                                   \
        break;                                                                                 \
    }
__declspec(noinline) void usersim_log_message_wstring(
    usersim_tracelog_level_t trace_level,
    usersim_tracelog_keyword_t keyword,
    _In_z_ const char* message,
    _In_z_ const wchar_t* wstring)
{
    switch (trace_level) {
    CASE_LOG_ALWAYS:
        USERSIM_LOG_MESSAGE_WSTRING_KEYWORD_SWITCH(LEVEL_LOG_ALWAYS, message, wstring);
        break;
    CASE_CRITICAL:
        USERSIM_LOG_MESSAGE_WSTRING_KEYWORD_SWITCH(LEVEL_CRITICAL, message, wstring);
        break;
    CASE_LEVEL_ERROR:
        USERSIM_LOG_MESSAGE_WSTRING_KEYWORD_SWITCH(LEVEL_ERROR, message, wstring);
        break;
    CASE_WARNING:
        USERSIM_LOG_MESSAGE_WSTRING_KEYWORD_SWITCH(LEVEL_WARNING, message, wstring);
        break;
    CASE_INFO:
        USERSIM_LOG_MESSAGE_WSTRING_KEYWORD_SWITCH(LEVEL_INFO, message, wstring);
        break;
    CASE_VERBOSE:
        USERSIM_LOG_MESSAGE_WSTRING_KEYWORD_SWITCH(LEVEL_VERBOSE, message, wstring);
        break;
    }
}

#define _USERSIM_LOG_MESSAGE_GUID_GUID_STRING(trace_level, keyword, message, string, guid1, guid2) \
    TraceLoggingWrite(                                                                          \
        usersim_tracelog_provider,                                                                 \
        USERSIM_TRACELOG_EVENT_GENERIC_MESSAGE,                                                    \
        TraceLoggingLevel((trace_level)),                                                       \
        TraceLoggingKeyword((keyword)),                                                         \
        TraceLoggingString((message), "Message"),                                               \
        TraceLoggingString(string, #string),                                                    \
        TraceLoggingGuid((guid1), (#guid1)),                                                    \
        TraceLoggingGuid((guid2), (#guid2)));
#define USERSIM_LOG_MESSAGE_GUID_GUID_STRING_KEYWORD_SWITCH(trace_level, keyword, message, string, guid1, guid2)        \
    switch (keyword) {                                                                                               \
    CASE_FUNCTION_ENTRY_EXIT:                                                                                        \
        _USERSIM_LOG_MESSAGE_GUID_GUID_STRING(trace_level, KEYWORD_FUNCTION_ENTRY_EXIT, message, string, guid1, guid2); \
        break;                                                                                                       \
    CASE_BASE:                                                                                                       \
        _USERSIM_LOG_MESSAGE_GUID_GUID_STRING(trace_level, KEYWORD_BASE, message, string, guid1, guid2);                \
        break;                                                                                                       \
    CASE_ERROR:                                                                                                      \
        _USERSIM_LOG_MESSAGE_GUID_GUID_STRING(trace_level, KEYWORD_ERROR, message, string, guid1, guid2);               \
        break;                                                                                                       \
    CASE_EPOCH:                                                                                                      \
        _USERSIM_LOG_MESSAGE_GUID_GUID_STRING(trace_level, KEYWORD_EPOCH, message, string, guid1, guid2);               \
        break;                                                                                                       \
    CASE_CORE:                                                                                                       \
        _USERSIM_LOG_MESSAGE_GUID_GUID_STRING(trace_level, KEYWORD_CORE, message, string, guid1, guid2);                \
        break;                                                                                                       \
    CASE_LINK:                                                                                                       \
        _USERSIM_LOG_MESSAGE_GUID_GUID_STRING(trace_level, KEYWORD_LINK, message, string, guid1, guid2);                \
        break;                                                                                                       \
    CASE_MAP:                                                                                                        \
        _USERSIM_LOG_MESSAGE_GUID_GUID_STRING(trace_level, KEYWORD_MAP, message, string, guid1, guid2);                 \
        break;                                                                                                       \
    CASE_PROGRAM:                                                                                                    \
        _USERSIM_LOG_MESSAGE_GUID_GUID_STRING(trace_level, KEYWORD_PROGRAM, message, string, guid1, guid2);             \
        break;                                                                                                       \
    CASE_API:                                                                                                        \
        _USERSIM_LOG_MESSAGE_GUID_GUID_STRING(trace_level, KEYWORD_API, message, string, guid1, guid2);                 \
        break;                                                                                                       \
    CASE_PRINTK:                                                                                                     \
        _USERSIM_LOG_MESSAGE_GUID_GUID_STRING(trace_level, KEYWORD_PRINTK, message, string, guid1, guid2);              \
        break;                                                                                                       \
    CASE_NATIVE:                                                                                                     \
        _USERSIM_LOG_MESSAGE_GUID_GUID_STRING(trace_level, KEYWORD_NATIVE, message, string, guid1, guid2);              \
        break;                                                                                                       \
    default:                                                                                                         \
        break;                                                                                                       \
    }
__declspec(noinline) void usersim_log_message_guid_guid_string(
    usersim_tracelog_level_t trace_level,
    usersim_tracelog_keyword_t keyword,
    _In_z_ const char* message,
    _In_z_ const char* string,
    _In_ const GUID* guid1,
    _In_ const GUID* guid2)
{
    switch (trace_level) {
    CASE_LOG_ALWAYS:
        USERSIM_LOG_MESSAGE_GUID_GUID_STRING_KEYWORD_SWITCH(LEVEL_LOG_ALWAYS, keyword, message, string, *guid1, *guid2);
        break;
    CASE_CRITICAL:
        USERSIM_LOG_MESSAGE_GUID_GUID_STRING_KEYWORD_SWITCH(LEVEL_CRITICAL, keyword, message, string, *guid1, *guid2);
        break;
    CASE_LEVEL_ERROR:
        USERSIM_LOG_MESSAGE_GUID_GUID_STRING_KEYWORD_SWITCH(LEVEL_ERROR, keyword, message, string, *guid1, *guid2);
        break;
    CASE_WARNING:
        USERSIM_LOG_MESSAGE_GUID_GUID_STRING_KEYWORD_SWITCH(LEVEL_WARNING, keyword, message, string, *guid1, *guid2);
        break;
    CASE_INFO:
        USERSIM_LOG_MESSAGE_GUID_GUID_STRING_KEYWORD_SWITCH(LEVEL_INFO, keyword, message, string, *guid1, *guid2);
        break;
    CASE_VERBOSE:
        USERSIM_LOG_MESSAGE_GUID_GUID_STRING_KEYWORD_SWITCH(LEVEL_VERBOSE, keyword, message, string, *guid1, *guid2);
        break;
    }
}

#define _USERSIM_LOG_MESSAGE_GUID_GUID(trace_level, keyword, message, guid1, guid2) \
    TraceLoggingWrite(                                                           \
        usersim_tracelog_provider,                                                  \
        USERSIM_TRACELOG_EVENT_GENERIC_MESSAGE,                                     \
        TraceLoggingLevel((trace_level)),                                        \
        TraceLoggingKeyword((keyword)),                                          \
        TraceLoggingString((message), "Message"),                                \
        TraceLoggingGuid((guid1), (#guid1)),                                     \
        TraceLoggingGuid((guid2), (#guid2)));
#define USERSIM_LOG_MESSAGE_GUID_GUID_KEYWORD_SWITCH(trace_level, keyword, message, guid1, guid2)        \
    switch (keyword) {                                                                                \
    CASE_FUNCTION_ENTRY_EXIT:                                                                         \
        _USERSIM_LOG_MESSAGE_GUID_GUID(trace_level, KEYWORD_FUNCTION_ENTRY_EXIT, message, guid1, guid2); \
        break;                                                                                        \
    CASE_BASE:                                                                                        \
        _USERSIM_LOG_MESSAGE_GUID_GUID(trace_level, KEYWORD_BASE, message, guid1, guid2);                \
        break;                                                                                        \
    CASE_ERROR:                                                                                       \
        _USERSIM_LOG_MESSAGE_GUID_GUID(trace_level, KEYWORD_ERROR, message, guid1, guid2);               \
        break;                                                                                        \
    CASE_EPOCH:                                                                                       \
        _USERSIM_LOG_MESSAGE_GUID_GUID(trace_level, KEYWORD_EPOCH, message, guid1, guid2);               \
        break;                                                                                        \
    CASE_CORE:                                                                                        \
        _USERSIM_LOG_MESSAGE_GUID_GUID(trace_level, KEYWORD_CORE, message, guid1, guid2);                \
        break;                                                                                        \
    CASE_LINK:                                                                                        \
        _USERSIM_LOG_MESSAGE_GUID_GUID(trace_level, KEYWORD_LINK, message, guid1, guid2);                \
        break;                                                                                        \
    CASE_MAP:                                                                                         \
        _USERSIM_LOG_MESSAGE_GUID_GUID(trace_level, KEYWORD_MAP, message, guid1, guid2);                 \
        break;                                                                                        \
    CASE_PROGRAM:                                                                                     \
        _USERSIM_LOG_MESSAGE_GUID_GUID(trace_level, KEYWORD_PROGRAM, message, guid1, guid2);             \
        break;                                                                                        \
    CASE_API:                                                                                         \
        _USERSIM_LOG_MESSAGE_GUID_GUID(trace_level, KEYWORD_API, message, guid1, guid2);                 \
        break;                                                                                        \
    CASE_PRINTK:                                                                                      \
        _USERSIM_LOG_MESSAGE_GUID_GUID(trace_level, KEYWORD_PRINTK, message, guid1, guid2);              \
        break;                                                                                        \
    CASE_NATIVE:                                                                                      \
        _USERSIM_LOG_MESSAGE_GUID_GUID(trace_level, KEYWORD_NATIVE, message, guid1, guid2);              \
        break;                                                                                        \
    default:                                                                                          \
        break;                                                                                        \
    }
__declspec(noinline) void usersim_log_message_guid_guid(
    usersim_tracelog_level_t trace_level,
    usersim_tracelog_keyword_t keyword,
    _In_z_ const char* message,
    _In_ const GUID* guid1,
    _In_ const GUID* guid2)
{
    switch (trace_level) {
    CASE_LOG_ALWAYS:
        USERSIM_LOG_MESSAGE_GUID_GUID_KEYWORD_SWITCH(LEVEL_LOG_ALWAYS, keyword, message, *guid1, *guid2);
        break;
    CASE_CRITICAL:
        USERSIM_LOG_MESSAGE_GUID_GUID_KEYWORD_SWITCH(LEVEL_CRITICAL, keyword, message, *guid1, *guid2);
        break;
    CASE_LEVEL_ERROR:
        USERSIM_LOG_MESSAGE_GUID_GUID_KEYWORD_SWITCH(LEVEL_ERROR, keyword, message, *guid1, *guid2);
        break;
    CASE_WARNING:
        USERSIM_LOG_MESSAGE_GUID_GUID_KEYWORD_SWITCH(LEVEL_WARNING, keyword, message, *guid1, *guid2);
        break;
    CASE_INFO:
        USERSIM_LOG_MESSAGE_GUID_GUID_KEYWORD_SWITCH(LEVEL_INFO, keyword, message, *guid1, *guid2);
        break;
    CASE_VERBOSE:
        USERSIM_LOG_MESSAGE_GUID_GUID_KEYWORD_SWITCH(LEVEL_VERBOSE, keyword, message, *guid1, *guid2);
        break;
    }
}

#define _USERSIM_LOG_MESSAGE_GUID(trace_level, keyword, message, guid) \
    TraceLoggingWrite(                                              \
        usersim_tracelog_provider,                                     \
        USERSIM_TRACELOG_EVENT_GENERIC_MESSAGE,                        \
        TraceLoggingLevel((trace_level)),                           \
        TraceLoggingKeyword((keyword)),                             \
        TraceLoggingString((message), "Message"),                   \
        TraceLoggingGuid((guid), (#guid)));
#define USERSIM_LOG_MESSAGE_GUID_KEYWORD_SWITCH(trace_level, keyword, message, guid)        \
    switch (keyword) {                                                                   \
    CASE_FUNCTION_ENTRY_EXIT:                                                            \
        _USERSIM_LOG_MESSAGE_GUID(trace_level, KEYWORD_FUNCTION_ENTRY_EXIT, message, guid); \
        break;                                                                           \
    CASE_BASE:                                                                           \
        _USERSIM_LOG_MESSAGE_GUID(trace_level, KEYWORD_BASE, message, guid);                \
        break;                                                                           \
    CASE_ERROR:                                                                          \
        _USERSIM_LOG_MESSAGE_GUID(trace_level, KEYWORD_ERROR, message, guid);               \
        break;                                                                           \
    CASE_EPOCH:                                                                          \
        _USERSIM_LOG_MESSAGE_GUID(trace_level, KEYWORD_EPOCH, message, guid);               \
        break;                                                                           \
    CASE_CORE:                                                                           \
        _USERSIM_LOG_MESSAGE_GUID(trace_level, KEYWORD_CORE, message, guid);                \
        break;                                                                           \
    CASE_LINK:                                                                           \
        _USERSIM_LOG_MESSAGE_GUID(trace_level, KEYWORD_LINK, message, guid);                \
        break;                                                                           \
    CASE_MAP:                                                                            \
        _USERSIM_LOG_MESSAGE_GUID(trace_level, KEYWORD_MAP, message, guid);                 \
        break;                                                                           \
    CASE_PROGRAM:                                                                        \
        _USERSIM_LOG_MESSAGE_GUID(trace_level, KEYWORD_PROGRAM, message, guid);             \
        break;                                                                           \
    CASE_API:                                                                            \
        _USERSIM_LOG_MESSAGE_GUID(trace_level, KEYWORD_API, message, guid);                 \
        break;                                                                           \
    CASE_PRINTK:                                                                         \
        _USERSIM_LOG_MESSAGE_GUID(trace_level, KEYWORD_PRINTK, message, guid);              \
        break;                                                                           \
    CASE_NATIVE:                                                                         \
        _USERSIM_LOG_MESSAGE_GUID(trace_level, KEYWORD_NATIVE, message, guid);              \
        break;                                                                           \
    default:                                                                             \
        break;                                                                           \
    }
__declspec(noinline) void usersim_log_message_guid(
    usersim_tracelog_level_t trace_level,
    usersim_tracelog_keyword_t keyword,
    _In_z_ const char* message,
    _In_ const GUID* guid)
{
    switch (trace_level) {
    CASE_LOG_ALWAYS:
        USERSIM_LOG_MESSAGE_GUID_KEYWORD_SWITCH(LEVEL_LOG_ALWAYS, keyword, message, *guid);
        break;
    CASE_CRITICAL:
        USERSIM_LOG_MESSAGE_GUID_KEYWORD_SWITCH(LEVEL_CRITICAL, keyword, message, *guid);
        break;
    CASE_LEVEL_ERROR:
        USERSIM_LOG_MESSAGE_GUID_KEYWORD_SWITCH(LEVEL_ERROR, keyword, message, *guid);
        break;
    CASE_WARNING:
        USERSIM_LOG_MESSAGE_GUID_KEYWORD_SWITCH(LEVEL_WARNING, keyword, message, *guid);
        break;
    CASE_INFO:
        USERSIM_LOG_MESSAGE_GUID_KEYWORD_SWITCH(LEVEL_INFO, keyword, message, *guid);
        break;
    CASE_VERBOSE:
        USERSIM_LOG_MESSAGE_GUID_KEYWORD_SWITCH(LEVEL_VERBOSE, keyword, message, *guid);
        break;
    }
}

#define _USERSIM_LOG_MESSAGE_NTSTATUS(trace_level, keyword, message, status) \
    TraceLoggingWrite(                                                    \
        usersim_tracelog_provider,                                           \
        USERSIM_TRACELOG_EVENT_GENERIC_MESSAGE,                              \
        TraceLoggingLevel(trace_level),                                   \
        TraceLoggingKeyword((keyword)),                                   \
        TraceLoggingString(message, "Message"),                           \
        TraceLoggingNTStatus(status));
#define USERSIM_LOG_MESSAGE_NTSTATUS_KEYWORD_SWITCH(trace_level, message, status)                 \
    switch (keyword) {                                                                         \
    CASE_FUNCTION_ENTRY_EXIT:                                                                  \
        _USERSIM_LOG_MESSAGE_NTSTATUS(trace_level, KEYWORD_FUNCTION_ENTRY_EXIT, message, status); \
        break;                                                                                 \
    CASE_BASE:                                                                                 \
        _USERSIM_LOG_MESSAGE_NTSTATUS(trace_level, KEYWORD_BASE, message, status);                \
        break;                                                                                 \
    CASE_ERROR:                                                                                \
        _USERSIM_LOG_MESSAGE_NTSTATUS(trace_level, KEYWORD_ERROR, message, status);               \
        break;                                                                                 \
    CASE_EPOCH:                                                                                \
        _USERSIM_LOG_MESSAGE_NTSTATUS(trace_level, KEYWORD_EPOCH, message, status);               \
        break;                                                                                 \
    CASE_CORE:                                                                                 \
        _USERSIM_LOG_MESSAGE_NTSTATUS(trace_level, KEYWORD_CORE, message, status);                \
        break;                                                                                 \
    CASE_LINK:                                                                                 \
        _USERSIM_LOG_MESSAGE_NTSTATUS(trace_level, KEYWORD_LINK, message, status);                \
        break;                                                                                 \
    CASE_MAP:                                                                                  \
        _USERSIM_LOG_MESSAGE_NTSTATUS(trace_level, KEYWORD_MAP, message, status);                 \
        break;                                                                                 \
    CASE_PROGRAM:                                                                              \
        _USERSIM_LOG_MESSAGE_NTSTATUS(trace_level, KEYWORD_PROGRAM, message, status);             \
        break;                                                                                 \
    CASE_API:                                                                                  \
        _USERSIM_LOG_MESSAGE_NTSTATUS(trace_level, KEYWORD_API, message, status);                 \
        break;                                                                                 \
    CASE_PRINTK:                                                                               \
        _USERSIM_LOG_MESSAGE_NTSTATUS(trace_level, KEYWORD_PRINTK, message, status);              \
        break;                                                                                 \
    CASE_NATIVE:                                                                               \
        _USERSIM_LOG_MESSAGE_NTSTATUS(trace_level, KEYWORD_NATIVE, message, status);              \
        break;                                                                                 \
    default:                                                                                   \
        break;                                                                                 \
    }
__declspec(noinline) void usersim_log_message_ntstatus(
    usersim_tracelog_level_t trace_level, usersim_tracelog_keyword_t keyword, _In_z_ const char* message, NTSTATUS status)
{
    switch (trace_level) {
    CASE_LOG_ALWAYS:
        USERSIM_LOG_MESSAGE_NTSTATUS_KEYWORD_SWITCH(LEVEL_LOG_ALWAYS, message, status);
        break;
    CASE_CRITICAL:
        USERSIM_LOG_MESSAGE_NTSTATUS_KEYWORD_SWITCH(LEVEL_CRITICAL, message, status);
        break;
    CASE_LEVEL_ERROR:
        USERSIM_LOG_MESSAGE_NTSTATUS_KEYWORD_SWITCH(LEVEL_ERROR, message, status);
        break;
    CASE_WARNING:
        USERSIM_LOG_MESSAGE_NTSTATUS_KEYWORD_SWITCH(LEVEL_WARNING, message, status);
        break;
    CASE_INFO:
        USERSIM_LOG_MESSAGE_NTSTATUS_KEYWORD_SWITCH(LEVEL_INFO, message, status);
        break;
    CASE_VERBOSE:
        USERSIM_LOG_MESSAGE_NTSTATUS_KEYWORD_SWITCH(LEVEL_VERBOSE, message, status);
        break;
    }
}

#define _USERSIM_LOG_MESSAGE_UINT64(trace_level, keyword, message, value) \
    TraceLoggingWrite(                                                 \
        usersim_tracelog_provider,                                        \
        USERSIM_TRACELOG_EVENT_GENERIC_MESSAGE,                           \
        TraceLoggingLevel((trace_level)),                              \
        TraceLoggingKeyword((keyword)),                                \
        TraceLoggingString((message), "Message"),                      \
        TraceLoggingUInt64((value), (#value)));
#define USERSIM_LOG_MESSAGE_UINT64_KEYWORD_SWITCH(trace_level, message, value)                 \
    switch (keyword) {                                                                      \
    CASE_FUNCTION_ENTRY_EXIT:                                                               \
        _USERSIM_LOG_MESSAGE_UINT64(trace_level, KEYWORD_FUNCTION_ENTRY_EXIT, message, value); \
        break;                                                                              \
    CASE_BASE:                                                                              \
        _USERSIM_LOG_MESSAGE_UINT64(trace_level, KEYWORD_BASE, message, value);                \
        break;                                                                              \
    CASE_ERROR:                                                                             \
        _USERSIM_LOG_MESSAGE_UINT64(trace_level, KEYWORD_ERROR, message, value);               \
        break;                                                                              \
    CASE_EPOCH:                                                                             \
        _USERSIM_LOG_MESSAGE_UINT64(trace_level, KEYWORD_EPOCH, message, value);               \
        break;                                                                              \
    CASE_CORE:                                                                              \
        _USERSIM_LOG_MESSAGE_UINT64(trace_level, KEYWORD_CORE, message, value);                \
        break;                                                                              \
    CASE_LINK:                                                                              \
        _USERSIM_LOG_MESSAGE_UINT64(trace_level, KEYWORD_LINK, message, value);                \
        break;                                                                              \
    CASE_MAP:                                                                               \
        _USERSIM_LOG_MESSAGE_UINT64(trace_level, KEYWORD_MAP, message, value);                 \
        break;                                                                              \
    CASE_PROGRAM:                                                                           \
        _USERSIM_LOG_MESSAGE_UINT64(trace_level, KEYWORD_PROGRAM, message, value);             \
        break;                                                                              \
    CASE_API:                                                                               \
        _USERSIM_LOG_MESSAGE_UINT64(trace_level, KEYWORD_API, message, value);                 \
        break;                                                                              \
    CASE_PRINTK:                                                                            \
        _USERSIM_LOG_MESSAGE_UINT64(trace_level, KEYWORD_PRINTK, message, value);              \
        break;                                                                              \
    CASE_NATIVE:                                                                            \
        _USERSIM_LOG_MESSAGE_UINT64(trace_level, KEYWORD_NATIVE, message, value);              \
        break;                                                                              \
    default:                                                                                \
        break;                                                                              \
    }
__declspec(noinline) void usersim_log_message_uint64(
    usersim_tracelog_level_t trace_level, usersim_tracelog_keyword_t keyword, _In_z_ const char* message, uint64_t value)
{
    switch (trace_level) {
    CASE_LOG_ALWAYS:
        USERSIM_LOG_MESSAGE_UINT64_KEYWORD_SWITCH(LEVEL_LOG_ALWAYS, message, value);
        break;
    CASE_CRITICAL:
        USERSIM_LOG_MESSAGE_UINT64_KEYWORD_SWITCH(LEVEL_CRITICAL, message, value);
        break;
    CASE_LEVEL_ERROR:
        USERSIM_LOG_MESSAGE_UINT64_KEYWORD_SWITCH(LEVEL_ERROR, message, value);
        break;
    CASE_WARNING:
        USERSIM_LOG_MESSAGE_UINT64_KEYWORD_SWITCH(LEVEL_WARNING, message, value);
        break;
    CASE_INFO:
        USERSIM_LOG_MESSAGE_UINT64_KEYWORD_SWITCH(LEVEL_INFO, message, value);
        break;
    CASE_VERBOSE:
        USERSIM_LOG_MESSAGE_UINT64_KEYWORD_SWITCH(LEVEL_VERBOSE, message, value);
        break;
    }
}

#define _USERSIM_LOG_MESSAGE_UINT64_UINT64(trace_level, keyword, message, value1, value2) \
    TraceLoggingWrite(                                                                 \
        usersim_tracelog_provider,                                                        \
        USERSIM_TRACELOG_EVENT_GENERIC_MESSAGE,                                           \
        TraceLoggingLevel((trace_level)),                                              \
        TraceLoggingKeyword((keyword)),                                                \
        TraceLoggingString((message), "Message"),                                      \
        TraceLoggingUInt64((value1), (#value1)),                                       \
        TraceLoggingUInt64((value2), (#value2)));
#define USERSIM_LOG_MESSAGE_UINT64_UINT64_KEYWORD_SWITCH(trace_level, message, value1, value2)                 \
    switch (keyword) {                                                                                      \
    CASE_FUNCTION_ENTRY_EXIT:                                                                               \
        _USERSIM_LOG_MESSAGE_UINT64_UINT64(trace_level, KEYWORD_FUNCTION_ENTRY_EXIT, message, value1, value2); \
        break;                                                                                              \
    CASE_BASE:                                                                                              \
        _USERSIM_LOG_MESSAGE_UINT64_UINT64(trace_level, KEYWORD_BASE, message, value1, value2);                \
        break;                                                                                              \
    CASE_ERROR:                                                                                             \
        _USERSIM_LOG_MESSAGE_UINT64_UINT64(trace_level, KEYWORD_ERROR, message, value1, value2);               \
        break;                                                                                              \
    CASE_EPOCH:                                                                                             \
        _USERSIM_LOG_MESSAGE_UINT64_UINT64(trace_level, KEYWORD_EPOCH, message, value1, value2);               \
        break;                                                                                              \
    CASE_CORE:                                                                                              \
        _USERSIM_LOG_MESSAGE_UINT64_UINT64(trace_level, KEYWORD_CORE, message, value1, value2);                \
        break;                                                                                              \
    CASE_LINK:                                                                                              \
        _USERSIM_LOG_MESSAGE_UINT64_UINT64(trace_level, KEYWORD_LINK, message, value1, value2);                \
        break;                                                                                              \
    CASE_MAP:                                                                                               \
        _USERSIM_LOG_MESSAGE_UINT64_UINT64(trace_level, KEYWORD_MAP, message, value1, value2);                 \
        break;                                                                                              \
    CASE_PROGRAM:                                                                                           \
        _USERSIM_LOG_MESSAGE_UINT64_UINT64(trace_level, KEYWORD_PROGRAM, message, value1, value2);             \
        break;                                                                                              \
    CASE_API:                                                                                               \
        _USERSIM_LOG_MESSAGE_UINT64_UINT64(trace_level, KEYWORD_API, message, value1, value2);                 \
        break;                                                                                              \
    CASE_PRINTK:                                                                                            \
        _USERSIM_LOG_MESSAGE_UINT64_UINT64(trace_level, KEYWORD_PRINTK, message, value1, value2);              \
        break;                                                                                              \
    CASE_NATIVE:                                                                                            \
        _USERSIM_LOG_MESSAGE_UINT64_UINT64(trace_level, KEYWORD_NATIVE, message, value1, value2);              \
        break;                                                                                              \
    default:                                                                                                \
        break;                                                                                              \
    }
__declspec(noinline) void usersim_log_message_uint64_uint64(
    usersim_tracelog_level_t trace_level,
    usersim_tracelog_keyword_t keyword,
    _In_z_ const char* message,
    uint64_t value1,
    uint64_t value2)
{
    switch (trace_level) {
    CASE_LOG_ALWAYS:
        USERSIM_LOG_MESSAGE_UINT64_UINT64_KEYWORD_SWITCH(LEVEL_LOG_ALWAYS, message, value1, value2);
        break;
    CASE_CRITICAL:
        USERSIM_LOG_MESSAGE_UINT64_UINT64_KEYWORD_SWITCH(LEVEL_CRITICAL, message, value1, value2);
        break;
    CASE_LEVEL_ERROR:
        USERSIM_LOG_MESSAGE_UINT64_UINT64_KEYWORD_SWITCH(LEVEL_ERROR, message, value1, value2);
        break;
    CASE_WARNING:
        USERSIM_LOG_MESSAGE_UINT64_UINT64_KEYWORD_SWITCH(LEVEL_WARNING, message, value1, value2);
        break;
    CASE_INFO:
        USERSIM_LOG_MESSAGE_UINT64_UINT64_KEYWORD_SWITCH(LEVEL_INFO, message, value1, value2);
        break;
    CASE_VERBOSE:
        USERSIM_LOG_MESSAGE_UINT64_UINT64_KEYWORD_SWITCH(LEVEL_VERBOSE, message, value1, value2);
        break;
    }
}

#define _USERSIM_LOG_MESSAGE_ERROR(trace_level, keyword, message, error) \
    TraceLoggingWrite(                                                \
        usersim_tracelog_provider,                                       \
        USERSIM_TRACELOG_EVENT_GENERIC_MESSAGE,                          \
        TraceLoggingLevel(trace_level),                               \
        TraceLoggingKeyword((keyword)),                               \
        TraceLoggingString(message, "Message"),                       \
        TraceLoggingWinError(error));
#define USERSIM_LOG_MESSAGE_ERROR_KEYWORD_SWITCH(trace_level, message, error)                 \
    switch (keyword) {                                                                     \
    CASE_FUNCTION_ENTRY_EXIT:                                                              \
        _USERSIM_LOG_MESSAGE_ERROR(trace_level, KEYWORD_FUNCTION_ENTRY_EXIT, message, error); \
        break;                                                                             \
    CASE_BASE:                                                                             \
        _USERSIM_LOG_MESSAGE_ERROR(trace_level, KEYWORD_BASE, message, error);                \
        break;                                                                             \
    CASE_ERROR:                                                                            \
        _USERSIM_LOG_MESSAGE_ERROR(trace_level, KEYWORD_ERROR, message, error);               \
        break;                                                                             \
    CASE_EPOCH:                                                                            \
        _USERSIM_LOG_MESSAGE_ERROR(trace_level, KEYWORD_EPOCH, message, error);               \
        break;                                                                             \
    CASE_CORE:                                                                             \
        _USERSIM_LOG_MESSAGE_ERROR(trace_level, KEYWORD_CORE, message, error);                \
        break;                                                                             \
    CASE_LINK:                                                                             \
        _USERSIM_LOG_MESSAGE_ERROR(trace_level, KEYWORD_LINK, message, error);                \
        break;                                                                             \
    CASE_MAP:                                                                              \
        _USERSIM_LOG_MESSAGE_ERROR(trace_level, KEYWORD_MAP, message, error);                 \
        break;                                                                             \
    CASE_PROGRAM:                                                                          \
        _USERSIM_LOG_MESSAGE_ERROR(trace_level, KEYWORD_PROGRAM, message, error);             \
        break;                                                                             \
    CASE_API:                                                                              \
        _USERSIM_LOG_MESSAGE_ERROR(trace_level, KEYWORD_API, message, error);                 \
        break;                                                                             \
    CASE_PRINTK:                                                                           \
        _USERSIM_LOG_MESSAGE_ERROR(trace_level, KEYWORD_PRINTK, message, error);              \
        break;                                                                             \
    CASE_NATIVE:                                                                           \
        _USERSIM_LOG_MESSAGE_ERROR(trace_level, KEYWORD_NATIVE, message, error);              \
        break;                                                                             \
    default:                                                                               \
        break;                                                                             \
    }
__declspec(noinline) void usersim_log_message_error(
    usersim_tracelog_level_t trace_level, usersim_tracelog_keyword_t keyword, _In_z_ const char* message, usersim_result_t error)
{
    switch (trace_level) {
    CASE_LOG_ALWAYS:
        USERSIM_LOG_MESSAGE_ERROR_KEYWORD_SWITCH(LEVEL_LOG_ALWAYS, message, error);
        break;
    CASE_CRITICAL:
        USERSIM_LOG_MESSAGE_ERROR_KEYWORD_SWITCH(LEVEL_CRITICAL, message, error);
        break;
    CASE_LEVEL_ERROR:
        USERSIM_LOG_MESSAGE_ERROR_KEYWORD_SWITCH(LEVEL_ERROR, message, error);
        break;
    CASE_WARNING:
        USERSIM_LOG_MESSAGE_ERROR_KEYWORD_SWITCH(LEVEL_WARNING, message, error);
        break;
    CASE_INFO:
        USERSIM_LOG_MESSAGE_ERROR_KEYWORD_SWITCH(LEVEL_INFO, message, error);
        break;
    CASE_VERBOSE:
        USERSIM_LOG_MESSAGE_ERROR_KEYWORD_SWITCH(LEVEL_VERBOSE, message, error);
        break;
    }
}

#define _USERSIM_LOG_NTSTATUS_WSTRING_API(keyword, wstring, api, status) \
    TraceLoggingWrite(                                                \
        usersim_tracelog_provider,                                       \
        USERSIM_TRACELOG_EVENT_API_ERROR,                                \
        TraceLoggingLevel(USERSIM_TRACELOG_LEVEL_INFO),                  \
        TraceLoggingKeyword((keyword)),                               \
        TraceLoggingWideString(wstring, "Message"),                   \
        TraceLoggingString(api, "Api"),                               \
        TraceLoggingNTStatus(status));
__declspec(noinline) void usersim_log_ntstatus_wstring_api(
    usersim_tracelog_keyword_t keyword, _In_z_ const wchar_t* wstring, _In_z_ const char* api, NTSTATUS status)
{
    switch (keyword) {
    CASE_FUNCTION_ENTRY_EXIT:
        _USERSIM_LOG_NTSTATUS_WSTRING_API(KEYWORD_FUNCTION_ENTRY_EXIT, wstring, api, status);
        break;
    CASE_BASE:
        _USERSIM_LOG_NTSTATUS_WSTRING_API(KEYWORD_BASE, wstring, api, status);
        break;
    CASE_ERROR:
        _USERSIM_LOG_NTSTATUS_WSTRING_API(KEYWORD_ERROR, wstring, api, status);
        break;
    CASE_EPOCH:
        _USERSIM_LOG_NTSTATUS_WSTRING_API(KEYWORD_EPOCH, wstring, api, status);
        break;
    CASE_CORE:
        _USERSIM_LOG_NTSTATUS_WSTRING_API(KEYWORD_CORE, wstring, api, status);
        break;
    CASE_LINK:
        _USERSIM_LOG_NTSTATUS_WSTRING_API(KEYWORD_LINK, wstring, api, status);
        break;
    CASE_MAP:
        _USERSIM_LOG_NTSTATUS_WSTRING_API(KEYWORD_MAP, wstring, api, status);
        break;
    CASE_PROGRAM:
        _USERSIM_LOG_NTSTATUS_WSTRING_API(KEYWORD_PROGRAM, wstring, api, status);
        break;
    CASE_API:
        _USERSIM_LOG_NTSTATUS_WSTRING_API(KEYWORD_API, wstring, api, status);
        break;
    CASE_PRINTK:
        _USERSIM_LOG_NTSTATUS_WSTRING_API(KEYWORD_PRINTK, wstring, api, status);
        break;
    CASE_NATIVE:
        _USERSIM_LOG_NTSTATUS_WSTRING_API(KEYWORD_NATIVE, wstring, api, status);
        break;
    default:
        break;
    }
}

#pragma warning(pop)
