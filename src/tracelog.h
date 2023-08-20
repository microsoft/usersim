// Copyright (c) Microsoft Corporation
// SPDX-License-Identifier: MIT

#pragma once

#include "platform.h"

TRACELOGGING_DECLARE_PROVIDER(usersim_tracelog_provider);

#ifdef __cplusplus
extern "C"
{
#endif

#define USERSIM_TRACELOG_EVENT_SUCCESS "UsersimSuccess"
#define USERSIM_TRACELOG_EVENT_RETURN "UsersimReturn"
#define USERSIM_TRACELOG_EVENT_GENERIC_ERROR "UsersimGenericError"
#define USERSIM_TRACELOG_EVENT_GENERIC_MESSAGE "UsersimGenericMessage"
#define USERSIM_TRACELOG_EVENT_API_ERROR "UsersimApiError"

#define USERSIM_TRACELOG_KEYWORD_FUNCTION_ENTRY_EXIT 0x1
#define USERSIM_TRACELOG_KEYWORD_BASE 0x2
#define USERSIM_TRACELOG_KEYWORD_ERROR 0x4
#define USERSIM_TRACELOG_KEYWORD_EPOCH 0x8
#define USERSIM_TRACELOG_KEYWORD_CORE 0x10
#define USERSIM_TRACELOG_KEYWORD_LINK 0x20
#define USERSIM_TRACELOG_KEYWORD_MAP 0x40
#define USERSIM_TRACELOG_KEYWORD_PROGRAM 0x80
#define USERSIM_TRACELOG_KEYWORD_API 0x100
#define USERSIM_TRACELOG_KEYWORD_PRINTK 0x200
#define USERSIM_TRACELOG_KEYWORD_NATIVE 0x400

#define USERSIM_TRACELOG_LEVEL_LOG_ALWAYS WINEVENT_LEVEL_LOG_ALWAYS
#define USERSIM_TRACELOG_LEVEL_CRITICAL WINEVENT_LEVEL_CRITICAL
#define USERSIM_TRACELOG_LEVEL_ERROR WINEVENT_LEVEL_ERROR
#define USERSIM_TRACELOG_LEVEL_WARNING WINEVENT_LEVEL_WARNING
#define USERSIM_TRACELOG_LEVEL_INFO WINEVENT_LEVEL_INFO
#define USERSIM_TRACELOG_LEVEL_VERBOSE WINEVENT_LEVEL_VERBOSE

    typedef enum _usersim_tracelog_keyword
    {
        _USERSIM_TRACELOG_KEYWORD_FUNCTION_ENTRY_EXIT,
        _USERSIM_TRACELOG_KEYWORD_BASE,
        _USERSIM_TRACELOG_KEYWORD_ERROR,
        _USERSIM_TRACELOG_KEYWORD_EPOCH,
        _USERSIM_TRACELOG_KEYWORD_CORE,
        _USERSIM_TRACELOG_KEYWORD_LINK,
        _USERSIM_TRACELOG_KEYWORD_MAP,
        _USERSIM_TRACELOG_KEYWORD_PROGRAM,
        _USERSIM_TRACELOG_KEYWORD_API,
        _USERSIM_TRACELOG_KEYWORD_PRINTK,
        _USERSIM_TRACELOG_KEYWORD_NATIVE
    } usersim_tracelog_keyword_t;

    typedef enum _usersim_tracelog_level
    {
        _USERSIM_TRACELOG_LEVEL_LOG_ALWAYS,
        _USERSIM_TRACELOG_LEVEL_CRITICAL,
        _USERSIM_TRACELOG_LEVEL_ERROR,
        _USERSIM_TRACELOG_LEVEL_WARNING,
        _USERSIM_TRACELOG_LEVEL_INFO,
        _USERSIM_TRACELOG_LEVEL_VERBOSE
    } usersim_tracelog_level_t;

    _Must_inspect_result_ usersim_result_t
    usersim_trace_initiate();

    void
    usersim_trace_terminate();

#define USERSIM_LOG_FUNCTION_SUCCESS()                                                             \
    if (TraceLoggingProviderEnabled(                                                            \
            usersim_tracelog_provider, USERSIM_TRACELOG_LEVEL_VERBOSE, USERSIM_TRACELOG_KEYWORD_BASE)) { \
        TraceLoggingWrite(                                                                      \
            usersim_tracelog_provider,                                                             \
            USERSIM_TRACELOG_EVENT_SUCCESS,                                                        \
            TraceLoggingLevel(WINEVENT_LEVEL_VERBOSE),                                          \
            TraceLoggingKeyword(USERSIM_TRACELOG_KEYWORD_BASE),                                    \
            TraceLoggingString(__FUNCTION__ " returned success", "Message"));                   \
    }

#define USERSIM_LOG_FUNCTION_ERROR(result)                                                         \
    if (TraceLoggingProviderEnabled(                                                            \
            usersim_tracelog_provider, USERSIM_TRACELOG_LEVEL_VERBOSE, USERSIM_TRACELOG_KEYWORD_BASE)) { \
        TraceLoggingWrite(                                                                      \
            usersim_tracelog_provider,                                                             \
            USERSIM_TRACELOG_EVENT_GENERIC_ERROR,                                                  \
            TraceLoggingLevel(WINEVENT_LEVEL_ERROR),                                            \
            TraceLoggingKeyword(USERSIM_TRACELOG_KEYWORD_BASE),                                    \
            TraceLoggingString(__FUNCTION__ " returned error", "ErrorMessage"),                 \
            TraceLoggingLong(result, "Error"));                                                 \
    }

#define USERSIM_LOG_ENTRY()                                                                                       \
    if (TraceLoggingProviderEnabled(                                                                           \
            usersim_tracelog_provider, USERSIM_TRACELOG_LEVEL_VERBOSE, USERSIM_TRACELOG_KEYWORD_FUNCTION_ENTRY_EXIT)) { \
        TraceLoggingWrite(                                                                                     \
            usersim_tracelog_provider,                                                                            \
            USERSIM_TRACELOG_EVENT_GENERIC_MESSAGE,                                                               \
            TraceLoggingLevel(WINEVENT_LEVEL_VERBOSE),                                                         \
            TraceLoggingKeyword(USERSIM_TRACELOG_KEYWORD_FUNCTION_ENTRY_EXIT),                                    \
            TraceLoggingOpcode(WINEVENT_OPCODE_START),                                                         \
            TraceLoggingString(__FUNCTION__, "Entry"));                                                        \
    }

#define USERSIM_LOG_EXIT()                                                                                        \
    if (TraceLoggingProviderEnabled(                                                                           \
            usersim_tracelog_provider, USERSIM_TRACELOG_LEVEL_VERBOSE, USERSIM_TRACELOG_KEYWORD_FUNCTION_ENTRY_EXIT)) { \
        TraceLoggingWrite(                                                                                     \
            usersim_tracelog_provider,                                                                            \
            USERSIM_TRACELOG_EVENT_GENERIC_MESSAGE,                                                               \
            TraceLoggingLevel(WINEVENT_LEVEL_VERBOSE),                                                         \
            TraceLoggingKeyword(USERSIM_TRACELOG_KEYWORD_FUNCTION_ENTRY_EXIT),                                    \
            TraceLoggingOpcode(WINEVENT_OPCODE_STOP),                                                          \
            TraceLoggingString(__FUNCTION__, "Exit"));                                                         \
    }

#define USERSIM_RETURN_ERROR(error)                   \
    do {                                           \
        uint32_t local_result = (error);           \
        if (local_result == ERROR_SUCCESS) {       \
            USERSIM_LOG_FUNCTION_SUCCESS();           \
        } else {                                   \
            USERSIM_LOG_FUNCTION_ERROR(local_result); \
        }                                          \
        return local_result;                       \
    } while (false);

    void
    usersim_log_ntstatus_api_failure(usersim_tracelog_keyword_t keyword, _In_z_ const char* api_name, NTSTATUS status);
#define USERSIM_LOG_NTSTATUS_API_FAILURE(keyword, api, status)                \
    if (TraceLoggingProviderEnabled(usersim_tracelog_provider, 0, keyword)) { \
        usersim_log_ntstatus_api_failure(_##keyword##, #api, status);         \
    }

    void
    usersim_log_ntstatus_api_failure_message(
        usersim_tracelog_keyword_t keyword, _In_z_ const char* api_name, NTSTATUS status, _In_z_ const char* message);
#define USERSIM_LOG_NTSTATUS_API_FAILURE_MESSAGE(keyword, api, status, message)        \
    if (TraceLoggingProviderEnabled(usersim_tracelog_provider, 0, keyword)) {          \
        usersim_log_ntstatus_api_failure_message(_##keyword##, #api, status, message); \
    }

    void
    usersim_log_message(usersim_tracelog_level_t trace_level, usersim_tracelog_keyword_t keyword, _In_z_ const char* message);
#define USERSIM_LOG_MESSAGE(trace_level, keyword, message)                              \
    if (TraceLoggingProviderEnabled(usersim_tracelog_provider, trace_level, keyword)) { \
        usersim_log_message(_##trace_level##, _##keyword##, message);                   \
    }

    void
    usersim_log_message_string(
        usersim_tracelog_level_t trace_level,
        usersim_tracelog_keyword_t keyword,
        _In_z_ const char* message,
        _In_z_ const char* string_value);
#define USERSIM_LOG_MESSAGE_STRING(trace_level, keyword, message, value)                \
    if (TraceLoggingProviderEnabled(usersim_tracelog_provider, trace_level, keyword)) { \
        usersim_log_message_string(_##trace_level##, _##keyword##, message, value);     \
    }

    void
    usersim_log_message_utf8_string(
        usersim_tracelog_level_t trace_level,
        usersim_tracelog_keyword_t keyword,
        _In_z_ const char* message,
        _In_ const cxplat_utf8_string_t* string);
#define USERSIM_LOG_MESSAGE_UTF8_STRING(trace_level, keyword, message, value)            \
    if (TraceLoggingProviderEnabled(usersim_tracelog_provider, trace_level, keyword)) {  \
        usersim_log_message_utf8_string(_##trace_level##, _##keyword##, message, value); \
    }

    void
    usersim_log_message_ntstatus(
        usersim_tracelog_level_t trace_level,
        usersim_tracelog_keyword_t keyword,
        _In_z_ const char* message,
        NTSTATUS status);
#define USERSIM_LOG_MESSAGE_NTSTATUS(trace_level, keyword, message, status)             \
    if (TraceLoggingProviderEnabled(usersim_tracelog_provider, trace_level, keyword)) { \
        usersim_log_message_ntstatus(_##trace_level##, _##keyword##, message, status);  \
    }

    void
    usersim_log_message_uint64(
        usersim_tracelog_level_t trace_level, usersim_tracelog_keyword_t keyword, _In_z_ const char* message, uint64_t value);
#define USERSIM_LOG_MESSAGE_UINT64(trace_level, keyword, message, value)                \
    if (TraceLoggingProviderEnabled(usersim_tracelog_provider, trace_level, keyword)) { \
        usersim_log_message_uint64(_##trace_level##, _##keyword##, message, value);     \
    }

    void
    usersim_log_message_uint64_uint64(
        usersim_tracelog_level_t trace_level,
        usersim_tracelog_keyword_t keyword,
        _In_z_ const char* message,
        uint64_t value1,
        uint64_t value2);
#define USERSIM_LOG_MESSAGE_UINT64_UINT64(trace_level, keyword, message, value1, value2)            \
    if (TraceLoggingProviderEnabled(usersim_tracelog_provider, trace_level, keyword)) {             \
        usersim_log_message_uint64_uint64(_##trace_level##, _##keyword##, message, value1, value2); \
    }

    void
    usersim_log_message_error(
        usersim_tracelog_level_t trace_level,
        usersim_tracelog_keyword_t keyword,
        _In_z_ const char* message,
        usersim_result_t error);
#define USERSIM_LOG_MESSAGE_ERROR(trace_level, keyword, message, error)                 \
    if (TraceLoggingProviderEnabled(usersim_tracelog_provider, trace_level, keyword)) { \
        usersim_log_message_error(_##trace_level##, _##keyword##, message, error);      \
    }

    void
    usersim_log_message_wstring(
        usersim_tracelog_level_t trace_level,
        usersim_tracelog_keyword_t keyword,
        _In_z_ const char* message,
        _In_z_ const wchar_t* wstring);
#define USERSIM_LOG_MESSAGE_WSTRING(trace_level, keyword, message, wstring)             \
    if (TraceLoggingProviderEnabled(usersim_tracelog_provider, trace_level, keyword)) { \
        usersim_log_message_wstring(_##trace_level##, _##keyword##, message, wstring);  \
    }

    void
    usersim_log_message_guid_guid_string(
        usersim_tracelog_level_t trace_level,
        usersim_tracelog_keyword_t keyword,
        _In_z_ const char* message,
        _In_z_ const char* string,
        _In_ const GUID* guid1,
        _In_ const GUID* guid2);
#define USERSIM_LOG_MESSAGE_GUID_GUID_STRING(trace_level, keyword, message, string, guid1, guid2)            \
    if (TraceLoggingProviderEnabled(usersim_tracelog_provider, trace_level, keyword)) {                      \
        usersim_log_message_guid_guid_string(_##trace_level##, _##keyword##, message, string, guid1, guid2); \
    }

    void
    usersim_log_message_guid_guid(
        usersim_tracelog_level_t trace_level,
        usersim_tracelog_keyword_t keyword,
        _In_z_ const char* message,
        _In_ const GUID* guid1,
        _In_ const GUID* guid2);
#define USERSIM_LOG_MESSAGE_GUID_GUID(trace_level, keyword, message, guid1, guid2)            \
    if (TraceLoggingProviderEnabled(usersim_tracelog_provider, trace_level, keyword)) {       \
        usersim_log_message_guid_guid(_##trace_level##, _##keyword##, message, guid1, guid2); \
    }

    void
    usersim_log_message_guid(
        usersim_tracelog_level_t trace_level,
        usersim_tracelog_keyword_t keyword,
        _In_z_ const char* message,
        _In_ const GUID* guid);
#define USERSIM_LOG_MESSAGE_GUID(trace_level, keyword, message, guid)                   \
    if (TraceLoggingProviderEnabled(usersim_tracelog_provider, trace_level, keyword)) { \
        usersim_log_message_guid(_##trace_level##, _##keyword##, message, guid);        \
    }

    void
    usersim_log_ntstatus_wstring_api(
        usersim_tracelog_keyword_t keyword, _In_z_ const wchar_t* wstring, _In_z_ const char* api, NTSTATUS status);
#define USERSIM_LOG_NTSTATUS_WSTRING_API(keyword, wstring, api, status)        \
    if (TraceLoggingProviderEnabled(usersim_tracelog_provider, 0, keyword)) {  \
        usersim_log_ntstatus_wstring_api(_##keyword##, wstring, #api, status); \
    }

    /////////////////////////////////////////////////////////
    // Macros built on top of the above primary trace macros.
    /////////////////////////////////////////////////////////

#define USERSIM_RETURN_VOID() \
    do {                   \
        USERSIM_LOG_EXIT();   \
        return;            \
    } while (false);

#define USERSIM_RETURN_RESULT(status)                 \
    do {                                           \
        usersim_result_t local_result = (status);     \
        if (local_result == STATUS_SUCCESS) {        \
            USERSIM_LOG_FUNCTION_SUCCESS();           \
        } else {                                   \
            USERSIM_LOG_FUNCTION_ERROR(local_result); \
        }                                          \
        return local_result;                       \
    } while (false);

#define USERSIM_RETURN_NTSTATUS(status)               \
    do {                                           \
        NTSTATUS local_status = (status);          \
        if (NT_SUCCESS(local_status)) {            \
            USERSIM_LOG_FUNCTION_SUCCESS();           \
        } else {                                   \
            USERSIM_LOG_FUNCTION_ERROR(local_status); \
        }                                          \
        return local_status;                       \
    } while (false);

#define USERSIM_RETURN_POINTER(type, pointer)                   \
    do {                                                     \
        type local_result = (type)(pointer);                 \
        TraceLoggingWrite(                                   \
            usersim_tracelog_provider,                          \
            USERSIM_TRACELOG_EVENT_RETURN,                      \
            TraceLoggingLevel(WINEVENT_LEVEL_VERBOSE),       \
            TraceLoggingKeyword(USERSIM_TRACELOG_KEYWORD_BASE), \
            TraceLoggingString(__FUNCTION__ " returned"),    \
            TraceLoggingPointer(local_result, #pointer));    \
        return local_result;                                 \
    } while (false);

#define USERSIM_RETURN_BOOL(flag)                               \
    do {                                                     \
        bool local_result = (flag);                          \
        TraceLoggingWrite(                                   \
            usersim_tracelog_provider,                          \
            USERSIM_TRACELOG_EVENT_RETURN,                      \
            TraceLoggingLevel(WINEVENT_LEVEL_VERBOSE),       \
            TraceLoggingKeyword(USERSIM_TRACELOG_KEYWORD_BASE), \
            TraceLoggingString(__FUNCTION__ " returned"),    \
            TraceLoggingBool(!!local_result, #flag));        \
        return local_result;                                 \
    } while (false);

#define USERSIM_RETURN_FD(fd)                                   \
    do {                                                     \
        fd_t local_fd = (fd);                                \
        TraceLoggingWrite(                                   \
            usersim_tracelog_provider,                          \
            USERSIM_TRACELOG_EVENT_RETURN,                      \
            TraceLoggingLevel(WINEVENT_LEVEL_VERBOSE),       \
            TraceLoggingKeyword(USERSIM_TRACELOG_KEYWORD_BASE), \
            TraceLoggingString(__FUNCTION__ " returned"),    \
            TraceLoggingInt32(local_fd, #fd));               \
        return local_fd;                                     \
    } while (false)

#define USERSIM_LOG_WIN32_STRING_API_FAILURE(keyword, message, api) \
    do {                                                         \
        unsigned long last_error = GetLastError();               \
        TraceLoggingWrite(                                       \
            usersim_tracelog_provider,                              \
            USERSIM_TRACELOG_EVENT_API_ERROR,                       \
            TraceLoggingLevel(USERSIM_TRACELOG_LEVEL_ERROR),        \
            TraceLoggingKeyword((keyword)),                      \
            TraceLoggingString(message, "Message"),              \
            TraceLoggingString(#api, "Api"),                     \
            TraceLoggingWinError(last_error));                   \
    } while (false);

#define USERSIM_LOG_WIN32_WSTRING_API_FAILURE(keyword, wstring, api) \
    do {                                                          \
        unsigned long last_error = GetLastError();                \
        TraceLoggingWrite(                                        \
            usersim_tracelog_provider,                               \
            USERSIM_TRACELOG_EVENT_API_ERROR,                        \
            TraceLoggingLevel(USERSIM_TRACELOG_LEVEL_ERROR),         \
            TraceLoggingKeyword((keyword)),                       \
            TraceLoggingWideString(wstring, "Message"),           \
            TraceLoggingString(#api, "Api"),                      \
            TraceLoggingWinError(last_error));                    \
    } while (false);

//
#define USERSIM_LOG_WIN32_GUID_API_FAILURE(keyword, guid, api) \
    do {                                                    \
        unsigned long last_error = GetLastError();          \
        TraceLoggingWrite(                                  \
            usersim_tracelog_provider,                         \
            USERSIM_TRACELOG_EVENT_API_ERROR,                  \
            TraceLoggingLevel(USERSIM_TRACELOG_LEVEL_ERROR),   \
            TraceLoggingKeyword((keyword)),                 \
            TraceLoggingGuid((*guid), (#guid)),             \
            TraceLoggingString(#api, "Api"),                \
            TraceLoggingWinError(last_error));              \
    } while (false);

#define USERSIM_LOG_WIN32_API_FAILURE(keyword, api)          \
    do {                                                  \
        unsigned long last_error = GetLastError();        \
        TraceLoggingWrite(                                \
            usersim_tracelog_provider,                       \
            USERSIM_TRACELOG_EVENT_API_ERROR,                \
            TraceLoggingLevel(USERSIM_TRACELOG_LEVEL_ERROR), \
            TraceLoggingKeyword((keyword)),               \
            TraceLoggingString(#api, "Api"),              \
            TraceLoggingWinError(last_error));            \
    } while (false);

#define USERSIM_LOG_MESSAGE_POINTER_ENUM(trace_level, keyword, message, pointer, enum) \
    TraceLoggingWrite(                                                              \
        usersim_tracelog_provider,                                                     \
        USERSIM_TRACELOG_EVENT_GENERIC_MESSAGE,                                        \
        TraceLoggingLevel((trace_level)),                                           \
        TraceLoggingKeyword((keyword)),                                             \
        TraceLoggingString((message), "Message"),                                   \
        TraceLoggingPointer(pointer, #pointer),                                     \
        TraceLoggingUInt32((enum), (#enum)));

#ifdef __cplusplus
}
#endif
