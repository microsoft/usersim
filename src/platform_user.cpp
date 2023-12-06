// Copyright (c) Microsoft Corporation
// SPDX-License-Identifier: MIT

#include "cxplat.h"
#include "cxplat_fault_injection.h"
#include "tracelog.h"
#include "usersim/ex.h"
#include "usersim/ke.h"
#include "usersim/mm.h"
#include "usersim/se.h"
#include "usersim/wdf.h"
#include "utilities.h"

#include "../inc/TraceLoggingProvider.h"
#include <functional>
#include <intsafe.h>
#include <map>
#include <mutex>
#include <queue>
#include <random>
#include <set>
#include <stdbool.h>
#include <stdint.h>
#include <string>
#include <vector>

// Global variables used to override behavior for testing.
// Permit the test to simulate both Hyper-V Code Integrity.
bool _usersim_platform_code_integrity_enabled = false;
// Permit the test to simulate non-preemptible execution.
bool _usersim_platform_is_preemptible = true;

// Global variable to track the number of times usersim_platform has been
// initialized. In user mode it is possible for usersim_platform_{initiate|terminate}
// to be called multiple times.
int32_t _usersim_platform_initiate_count = 0;

static uint32_t _usersim_platform_maximum_group_count = 0;
static uint32_t _usersim_platform_maximum_processor_count = 0;

static bool _cxplat_initialized = false;

// The starting index of the first processor in each group.
// Used to compute the current CPU index.
static std::vector<uint32_t> _usersim_platform_group_to_index_map;

_Must_inspect_result_ usersim_result_t
usersim_platform_initiate()
{
    usersim_result_t result;

    int32_t count = InterlockedIncrement((volatile long*)&_usersim_platform_initiate_count);
    if (count > 1) {
        // Usersim library already initialized, return.
        return STATUS_SUCCESS;
    }

    int cxplat_error = cxplat_initialize();
    if (cxplat_error != 0) {
        result = STATUS_NO_MEMORY;
        goto Exit;
    }

    _cxplat_initialized = true;

    try {
        _usersim_platform_maximum_group_count = GetMaximumProcessorGroupCount();
        _usersim_platform_maximum_processor_count = GetMaximumProcessorCount(ALL_PROCESSOR_GROUPS);

        result = usersim_initialize_irql();
        if (result != STATUS_SUCCESS) {
            goto Exit;
        }

        usersim_initialize_dpcs();

        // Compute the starting index of each processor group.
        _usersim_platform_group_to_index_map.resize(_usersim_platform_maximum_group_count);
        uint32_t base_index = 0;
        for (size_t i = 0; i < _usersim_platform_group_to_index_map.size(); i++) {
            _usersim_platform_group_to_index_map[i] = base_index;
            base_index += GetMaximumProcessorCount((uint16_t)i);
        }

        usersim_initialize_se();
        usersim_initialize_wdf();
    } catch (const std::bad_alloc&) {
        result = STATUS_NO_MEMORY;
        goto Exit;
    }

Exit:
    if (result != STATUS_SUCCESS) {
        // Clean up since usersim_platform_terminate() will not be called by the caller.
        usersim_platform_terminate();
    }
    return result;
}

void
usersim_platform_terminate()
{
    cxplat_wait_for_preemptible_work_items_complete();

    usersim_free_semaphores();
    usersim_free_threadpool_timers();
    usersim_clean_up_dpcs();
    usersim_clean_up_irql();
    if (_cxplat_initialized) {
        cxplat_cleanup();
        _cxplat_initialized = false;
    }

    int32_t count = InterlockedDecrement((volatile long*)&_usersim_platform_initiate_count);
    if (count < 0) {
        KeBugCheckCPP(0);
    }
}

_Must_inspect_result_ usersim_result_t
usersim_get_code_integrity_state(_Out_ usersim_code_integrity_state_t* state)
{
    USERSIM_LOG_ENTRY();
    if (_usersim_platform_code_integrity_enabled) {
        USERSIM_LOG_MESSAGE(USERSIM_TRACELOG_LEVEL_INFO, USERSIM_TRACELOG_KEYWORD_BASE, "Code integrity enabled");
        *state = USERSIM_CODE_INTEGRITY_HYPERVISOR_KERNEL_MODE;
    } else {
        USERSIM_LOG_MESSAGE(USERSIM_TRACELOG_LEVEL_INFO, USERSIM_TRACELOG_KEYWORD_BASE, "Code integrity disabled");
        *state = USERSIM_CODE_INTEGRITY_DEFAULT;
    }
    USERSIM_RETURN_RESULT(STATUS_SUCCESS);
}

struct usersim_ring_descriptor_t
{
    void* primary_view;
    void* secondary_view;
    size_t length;
};
typedef struct usersim_ring_descriptor_t usersim_ring_descriptor_t;

// This code is derived from the sample at:
// https://docs.microsoft.com/en-us/windows/win32/api/memoryapi/nf-memoryapi-virtualalloc2

_Ret_maybenull_ usersim_ring_descriptor_t*
usersim_allocate_ring_buffer_memory(size_t length)
{
    USERSIM_LOG_ENTRY();
    bool result = false;
    HANDLE section = nullptr;
    SYSTEM_INFO sysInfo;
    uint8_t* placeholder1 = nullptr;
    uint8_t* placeholder2 = nullptr;
    void* view1 = nullptr;
    void* view2 = nullptr;

    // Skip fault injection for this VirtualAlloc2 OS API, as usersim_allocate already does that.
    GetSystemInfo(&sysInfo);

    if (length == 0) {
        USERSIM_LOG_MESSAGE(USERSIM_TRACELOG_LEVEL_ERROR, USERSIM_TRACELOG_KEYWORD_BASE, "Ring buffer length is zero");
        return nullptr;
    }

    if ((length % sysInfo.dwAllocationGranularity) != 0) {
        USERSIM_LOG_MESSAGE_UINT64(
            USERSIM_TRACELOG_LEVEL_ERROR,
            USERSIM_TRACELOG_KEYWORD_BASE,
            "Ring buffer length doesn't match allocation granularity",
            length);
        return nullptr;
    }

    usersim_ring_descriptor_t* descriptor = (usersim_ring_descriptor_t*)cxplat_allocate(
        CXPLAT_POOL_FLAG_NON_PAGED, sizeof(usersim_ring_descriptor_t), USERSIM_TAG_RING_DESCRIPTOR);
    if (!descriptor) {
        goto Exit;
    }
    descriptor->length = length;

    //
    // Reserve a placeholder region where the buffer will be mapped.
    //
    placeholder1 = reinterpret_cast<uint8_t*>(
        VirtualAlloc2(nullptr, nullptr, 2 * length, MEM_RESERVE | MEM_RESERVE_PLACEHOLDER, PAGE_NOACCESS, nullptr, 0));

    if (placeholder1 == nullptr) {
        USERSIM_LOG_WIN32_API_FAILURE(USERSIM_TRACELOG_KEYWORD_BASE, VirtualAlloc2);
        goto Exit;
    }

#pragma warning(push)
#pragma warning(disable : 6333)  // Invalid parameter:  passing MEM_RELEASE and a non-zero dwSize parameter to
                                 // 'VirtualFree' is not allowed.  This causes the call to fail.
#pragma warning(disable : 28160) // Passing MEM_RELEASE and a non-zero dwSize parameter to VirtualFree is not allowed.
                                 // This results in the failure of this call.
    //
    // Split the placeholder region into two regions of equal size.
    //
    result = VirtualFree(placeholder1, length, MEM_RELEASE | MEM_PRESERVE_PLACEHOLDER);
    if (result == FALSE) {
        USERSIM_LOG_WIN32_API_FAILURE(USERSIM_TRACELOG_KEYWORD_BASE, VirtualFree);
        goto Exit;
    }
#pragma warning(pop)
    placeholder2 = placeholder1 + length;

    //
    // Create a pagefile-backed section for the buffer.
    //

    section = CreateFileMapping(
        INVALID_HANDLE_VALUE, nullptr, PAGE_READWRITE, 0, static_cast<unsigned long>(length), nullptr);
    if (section == nullptr) {
        USERSIM_LOG_WIN32_API_FAILURE(USERSIM_TRACELOG_KEYWORD_BASE, CreateFileMapping);
        goto Exit;
    }

    //
    // Map the section into the first placeholder region.
    //
    view1 =
        MapViewOfFile3(section, nullptr, placeholder1, 0, length, MEM_REPLACE_PLACEHOLDER, PAGE_READWRITE, nullptr, 0);
    if (view1 == nullptr) {
        USERSIM_LOG_WIN32_API_FAILURE(USERSIM_TRACELOG_KEYWORD_BASE, MapViewOfFile3);
        goto Exit;
    }

    //
    // Ownership transferred, don't free this now.
    //
    placeholder1 = nullptr;

    //
    // Map the section into the second placeholder region.
    //
    view2 =
        MapViewOfFile3(section, nullptr, placeholder2, 0, length, MEM_REPLACE_PLACEHOLDER, PAGE_READWRITE, nullptr, 0);
    if (view2 == nullptr) {
        USERSIM_LOG_WIN32_API_FAILURE(USERSIM_TRACELOG_KEYWORD_BASE, MapViewOfFile3);
        goto Exit;
    }

    result = true;

    //
    // Success, return both mapped views to the caller.
    //
    descriptor->primary_view = view1;
    descriptor->secondary_view = view2;

    placeholder2 = nullptr;
    view1 = nullptr;
    view2 = nullptr;
Exit:
    if (!result) {
        cxplat_free(descriptor, CXPLAT_POOL_FLAG_NON_PAGED, USERSIM_TAG_RING_DESCRIPTOR);
        descriptor = nullptr;
    }

    if (section != nullptr) {
        CloseHandle(section);
    }

    if (placeholder1 != nullptr) {
        VirtualFree(placeholder1, 0, MEM_RELEASE);
    }

    if (placeholder2 != nullptr) {
        VirtualFree(placeholder2, 0, MEM_RELEASE);
    }

    if (view1 != nullptr) {
        UnmapViewOfFileEx(view1, 0);
    }

    if (view2 != nullptr) {
        UnmapViewOfFileEx(view2, 0);
    }

    USERSIM_RETURN_POINTER(usersim_ring_descriptor_t*, descriptor);
}

void
usersim_free_ring_buffer_memory(_Frees_ptr_opt_ usersim_ring_descriptor_t* ring)
{
    USERSIM_LOG_ENTRY();
    if (ring) {
        UnmapViewOfFile(ring->primary_view);
        UnmapViewOfFile(ring->secondary_view);
        cxplat_free(ring, CXPLAT_POOL_FLAG_NON_PAGED, USERSIM_TAG_RING_DESCRIPTOR);
    }
    USERSIM_RETURN_VOID();
}

void*
usersim_ring_descriptor_get_base_address(_In_ const usersim_ring_descriptor_t* ring_descriptor)
{
    return ring_descriptor->primary_view;
}

_Ret_maybenull_ void*
usersim_ring_map_readonly_user(_In_ const usersim_ring_descriptor_t* ring)
{
    USERSIM_LOG_ENTRY();
    USERSIM_RETURN_POINTER(void*, usersim_ring_descriptor_get_base_address(ring));
}

void
usersim_lock_create(_Out_ usersim_lock_t* lock)
{
    InitializeSRWLock(reinterpret_cast<PSRWLOCK>(lock));
}

void
usersim_lock_destroy(_In_ _Post_invalid_ usersim_lock_t* lock)
{
    UNREFERENCED_PARAMETER(lock);
}

_Requires_lock_not_held_(*lock) _Acquires_lock_(*lock) _IRQL_requires_max_(DISPATCH_LEVEL) _IRQL_saves_
    _IRQL_raises_(DISPATCH_LEVEL) usersim_lock_state_t usersim_lock_lock(_Inout_ usersim_lock_t* lock)
{
    AcquireSRWLockExclusive(reinterpret_cast<PSRWLOCK>(lock));
    return 0;
}

_Requires_lock_held_(*lock) _Releases_lock_(*lock) _IRQL_requires_(DISPATCH_LEVEL) void usersim_lock_unlock(
    _Inout_ usersim_lock_t* lock, _IRQL_restores_ usersim_lock_state_t state)
{
    UNREFERENCED_PARAMETER(state);
    ReleaseSRWLockExclusive(reinterpret_cast<PSRWLOCK>(lock));
}

uint32_t
usersim_random_uint32()
{
    std::random_device rd;
    std::mt19937 mt(rd());
    return mt();
}

uint64_t
usersim_query_time_since_boot(bool include_suspended_time)
{
    uint64_t interrupt_time;
    if (include_suspended_time) {
        // QueryUnbiasedInterruptTimePrecise returns A pointer to a ULONGLONG in which to receive the interrupt-time
        // count in system time units of 100 nanoseconds.
        // Unbiased Interrupt time is the total time since boot including time spent suspended.
        // https://docs.microsoft.com/en-us/windows/win32/api/realtimeapiset/nf-realtimeapiset-queryunbiasedinterrupttimeprecise.
        QueryUnbiasedInterruptTimePrecise(&interrupt_time);
    } else {
        // QueryInterruptTimePrecise returns A pointer to a ULONGLONG in which to receive the interrupt-time count in
        // system time units of 100 nanoseconds.
        // (Biased) Interrupt time is the total time since boot excluding time spent suspended.
        // https://docs.microsoft.com/en-us/windows/win32/api/realtimeapiset/nf-realtimeapiset-queryinterrupttimeprecise.
        QueryInterruptTimePrecise(&interrupt_time);
    }

    return interrupt_time;
}

_Ret_range_(>, 0) uint32_t usersim_get_cpu_count() { return _usersim_platform_maximum_processor_count; }

bool
usersim_is_preemptible()
{
    KIRQL irql = KeGetCurrentIrql();
    return irql < DISPATCH_LEVEL;
}

ULONG
KeGetCurrentProcessorNumberEx(_Out_opt_ PPROCESSOR_NUMBER ProcNumber)
{
    PROCESSOR_NUMBER processor_number;
    GetCurrentProcessorNumberEx(&processor_number);

    if (ProcNumber != nullptr) {
        *ProcNumber = processor_number;
    }

    // Compute the CPU index from the group and number.
    return _usersim_platform_group_to_index_map[processor_number.Group] + processor_number.Number;
}

uint64_t
usersim_get_current_thread_id()
{
    return GetCurrentThreadId();
}

int32_t
usersim_log_function(_In_ void* context, _In_z_ const char* format_string, ...)
{
    UNREFERENCED_PARAMETER(context);

    va_list arg_start;
    va_start(arg_start, format_string);

    vprintf(format_string, arg_start);

    va_end(arg_start);
    return 0;
}

_Must_inspect_result_ NTSTATUS
usersim_access_check(
    _In_ const usersim_security_descriptor_t* security_descriptor,
    usersim_security_access_mask_t request_access,
    _In_ const usersim_security_generic_mapping_t* generic_mapping)
{
    if (cxplat_fault_injection_inject_fault()) {
        return STATUS_ACCESS_DENIED;
    }

    usersim_result_t result;
    HANDLE token = INVALID_HANDLE_VALUE;

    // Using BOOL to pass "AccessCheck" defined in Windows "securitybaseapi.h" file
    BOOL access_status = FALSE;
    unsigned long granted_access;
    PRIVILEGE_SET privilege_set;
    unsigned long privilege_set_size = sizeof(privilege_set);
    bool is_impersonating = false;

    if (!ImpersonateSelf(SecurityImpersonation)) {
        result = STATUS_ACCESS_DENIED;
        goto Done;
    }
    is_impersonating = true;
    if (!OpenThreadToken(GetCurrentThread(), TOKEN_QUERY, TRUE, &token)) {
        result = STATUS_ACCESS_DENIED;
        goto Done;
    }

    if (!AccessCheck(
            const_cast<_SECURITY_DESCRIPTOR*>(security_descriptor),
            token,
            request_access,
            const_cast<GENERIC_MAPPING*>(generic_mapping),
            &privilege_set,
            &privilege_set_size,
            &granted_access,
            &access_status)) {
        unsigned long err = GetLastError();
        printf("LastError: %d\n", err);
        result = STATUS_ACCESS_DENIED;
    } else {
        result = access_status ? STATUS_SUCCESS : STATUS_ACCESS_DENIED;
    }

Done:
    if (token != INVALID_HANDLE_VALUE) {
        CloseHandle(token);
    }

    if (is_impersonating) {
        RevertToSelf();
    }
    return result;
}

_Must_inspect_result_ NTSTATUS
usersim_validate_security_descriptor(
    _In_ const usersim_security_descriptor_t* security_descriptor, size_t security_descriptor_length)
{
    if (cxplat_fault_injection_inject_fault()) {
        return STATUS_NO_MEMORY;
    }

    usersim_result_t result;
    SECURITY_DESCRIPTOR_CONTROL security_descriptor_control;
    unsigned long version;
    unsigned long length;
    if (!IsValidSecurityDescriptor(const_cast<_SECURITY_DESCRIPTOR*>(security_descriptor))) {
        result = STATUS_INVALID_PARAMETER;
        goto Done;
    }

    if (!GetSecurityDescriptorControl(
            const_cast<_SECURITY_DESCRIPTOR*>(security_descriptor), &security_descriptor_control, &version)) {
        result = STATUS_INVALID_PARAMETER;
        goto Done;
    }

    if ((security_descriptor_control & SE_SELF_RELATIVE) == 0) {
        result = STATUS_INVALID_PARAMETER;
        goto Done;
    }

    length = GetSecurityDescriptorLength(const_cast<_SECURITY_DESCRIPTOR*>(security_descriptor));
    if (length != security_descriptor_length) {
        result = STATUS_INVALID_PARAMETER;
        goto Done;
    }

    result = STATUS_SUCCESS;

Done:
    return result;
}

uint32_t
usersim_result_to_win32_error_code(NTSTATUS result)
{
    static uint32_t (*RtlNtStatusToDosError)(NTSTATUS Status) = nullptr;
    if (!RtlNtStatusToDosError) {
        HMODULE ntdll = LoadLibrary(L"ntdll.dll");
        if (!ntdll) {
            return ERROR_OUTOFMEMORY;
        }
        RtlNtStatusToDosError =
            reinterpret_cast<decltype(RtlNtStatusToDosError)>(GetProcAddress(ntdll, "RtlNtStatusToDosError"));
    }
    return RtlNtStatusToDosError(result);
}

_IRQL_requires_max_(PASSIVE_LEVEL) _Must_inspect_result_ NTSTATUS
    usersim_platform_get_authentication_id(_Out_ uint64_t* authentication_id)
{
    // GetTokenInformation OS API can fail with return value of zero.
    if (cxplat_fault_injection_inject_fault()) {
        return STATUS_NO_MEMORY;
    }

    usersim_result_t return_value = STATUS_SUCCESS;
    uint32_t error;
    TOKEN_GROUPS_AND_PRIVILEGES* privileges = nullptr;
    uint32_t size = 0;
    HANDLE thread_token_handle = GetCurrentThreadEffectiveToken();

    bool result = GetTokenInformation(thread_token_handle, TokenGroupsAndPrivileges, nullptr, 0, (unsigned long*)&size);
    error = GetLastError();
    if (error != ERROR_INSUFFICIENT_BUFFER) {
        USERSIM_LOG_WIN32_API_FAILURE(USERSIM_TRACELOG_KEYWORD_BASE, GetTokenInformation);
        return win32_error_to_usersim_error(GetLastError());
    }

    privileges = (TOKEN_GROUPS_AND_PRIVILEGES*)cxplat_allocate(
        CXPLAT_POOL_FLAG_NON_PAGED, size, USERSIM_TAG_TOKEN_GROUPS_AND_PRIVILEGES);
    if (privileges == nullptr) {
        return STATUS_NO_MEMORY;
    }

    result =
        GetTokenInformation(thread_token_handle, TokenGroupsAndPrivileges, privileges, size, (unsigned long*)&size);
    if (result == false) {
        USERSIM_LOG_WIN32_API_FAILURE(USERSIM_TRACELOG_KEYWORD_BASE, GetTokenInformation);
        return_value = win32_error_to_usersim_error(GetLastError());
        goto Exit;
    }

    *authentication_id = *(uint64_t*)&privileges->AuthenticationId;

Exit:
    cxplat_free(privileges, CXPLAT_POOL_FLAG_NON_PAGED, USERSIM_TAG_TOKEN_GROUPS_AND_PRIVILEGES);
    return return_value;
}

void
usersim_enter_critical_region()
{
    // This is a no-op for the user mode implementation.
}

void
usersim_leave_critical_region()
{
    // This is a no-op for the user mode implementation.
}

usersim_result_t
usersim_utf8_string_to_unicode(_In_ const cxplat_utf8_string_t* input, _Outptr_ wchar_t** output)
{
    wchar_t* unicode_string = NULL;
    usersim_result_t retval;

    // Compute the size needed to hold the unicode string.
    int result = MultiByteToWideChar(CP_UTF8, 0, (const char*)input->value, (int)input->length, NULL, 0);

    if (result <= 0) {
        retval = STATUS_INVALID_PARAMETER;
        goto Done;
    }

    result++;

    unicode_string =
        (wchar_t*)cxplat_allocate(CXPLAT_POOL_FLAG_NON_PAGED, result * sizeof(wchar_t), USERSIM_TAG_UNICODE_STRING);
    if (unicode_string == NULL) {
        retval = STATUS_NO_MEMORY;
        goto Done;
    }

    result = MultiByteToWideChar(CP_UTF8, 0, (const char*)input->value, (int)input->length, unicode_string, result);

    if (result == 0) {
        retval = STATUS_INVALID_PARAMETER;
        goto Done;
    }

    *output = unicode_string;
    unicode_string = NULL;
    retval = STATUS_SUCCESS;

Done:
    cxplat_free(unicode_string, CXPLAT_POOL_FLAG_NON_PAGED, USERSIM_TAG_UNICODE_STRING);
    return retval;
}

intptr_t
usersim_platform_reference_process()
{
    HANDLE process = GetCurrentProcess();
    return (intptr_t)process;
}

void
usersim_platform_dereference_process(intptr_t process_handle)
{
    // This is a no-op for the user mode implementation.
    UNREFERENCED_PARAMETER(process_handle);
}

void
usersim_platform_attach_process(intptr_t process_handle, _Inout_ usersim_process_state_t* state)
{
    // This is a no-op for the user mode implementation.
    UNREFERENCED_PARAMETER(process_handle);
    UNREFERENCED_PARAMETER(state);
}

void
usersim_platform_detach_process(_In_ usersim_process_state_t* state)
{
    // This is a no-op for the user mode implementation.
    UNREFERENCED_PARAMETER(state);
}

#define HANDLE_VARIABLE_TYPE(type, fmt)                    \
    {                                                      \
        int count = va_arg(valist, int);                   \
        i++;                                               \
        if (count > 0) {                                   \
            type value = va_arg(valist, type);             \
            printf("," fmt, value);                        \
        }                                                  \
        for (int j = 1; j < count; j++) {                  \
            const char* str = va_arg(valist, const char*); \
            printf(",\"%s\"", str);                        \
        }                                                  \
        i += count;                                        \
    }

static bool _usersim_trace_logging_enabled = false;
static UCHAR _usersim_trace_logging_event_level = 0;
static ULONGLONG _usersim_trace_logging_event_keyword = 0;

BOOLEAN
usersim_trace_logging_provider_enabled(
    _In_ const TraceLoggingHProvider hProvider, UCHAR event_level, ULONGLONG event_keyword)
{
    UNREFERENCED_PARAMETER(hProvider);
    return (event_level <= _usersim_trace_logging_event_level) &&
           (_usersim_trace_logging_event_keyword & event_keyword);
}

void
usersim_trace_logging_set_enabled(bool enabled, UCHAR event_level, ULONGLONG event_keyword)
{
    _usersim_trace_logging_enabled = enabled;
    _usersim_trace_logging_event_level = event_level;
    _usersim_trace_logging_event_keyword = event_keyword;
}

void
usersim_trace_logging_write(_In_ const TraceLoggingHProvider hProvider, _In_z_ const char* eventName, size_t argc, ...)
{
    UNREFERENCED_PARAMETER(hProvider);

    if (!_usersim_trace_logging_enabled) {
        return;
    }

    printf("{%s", eventName);

    va_list valist;
    va_start(valist, argc);
    int level = 0;
    int keyword = 0;
    int opcode = 0;
    for (size_t i = 0; i < argc; i++) {
        usersim_tlg_type_t type = va_arg(valist, usersim_tlg_type_t);
        switch (type) {
        case _tlgLevel:
            level = va_arg(valist, int);
            i++;
            break;
        case _tlgKeyword:
            keyword = va_arg(valist, int);
            i++;
            break;
        case _tlgOpcode:
            opcode = va_arg(valist, int);
            i++;
            break;
        case _tlgCountedUtf8String: {
            const char* value = va_arg(valist, const char*);
            i++;
            int size = va_arg(valist, int);
            i++;
            std::string result(value, size);
            printf(",%s", result.c_str());

            int count = va_arg(valist, int);
            i++;
            for (int j = 0; j < count; j++) {
                const char* str = va_arg(valist, const char*);
                printf(",\"%s\"", str);
            }
            i += count;
            break;
        }
        case _tlgPsz:
            HANDLE_VARIABLE_TYPE(const char*, "\"%s\"");
            break;
        case _tlgPwsz:
            HANDLE_VARIABLE_TYPE(const WCHAR*, "\"%ls\"");
            break;
        case _tlgPointer:
            HANDLE_VARIABLE_TYPE(const void*, "%p");
            break;
        case _tlgUInt64:
            HANDLE_VARIABLE_TYPE(uint64_t, "%I64u");
            break;
        case _tlgUInt32:
            HANDLE_VARIABLE_TYPE(uint32_t, "%u");
            break;
        case _tlgUInt16:
            HANDLE_VARIABLE_TYPE(uint16_t, "%u");
            break;
        case _tlgNTStatus:
            HANDLE_VARIABLE_TYPE(uint32_t, "%x");
            break;
        case _tlgWinError:
        case _tlgInt32:
        case _tlgLong:
        case _tlgBool:
            HANDLE_VARIABLE_TYPE(int32_t, "%d");
            break;
        case _tlgGuid: {
            int count = va_arg(valist, int);
            i++;
            if (count > 0) {
                GUID value = va_arg(valist, GUID);
                char* guid_string = nullptr;
                if (UuidToStringA(&value, (RPC_CSTR*)&guid_string) == RPC_S_OK) {
                    printf(",%s", guid_string);
                    RpcStringFreeA((RPC_CSTR*)&guid_string);
                }
            }
            for (int j = 1; j < count; j++) {
                const char* str = va_arg(valist, const char*);
                printf(",\"%s\"", str);
            }
            i += count;
            break;
        }
        case _tlgIPv4Address: {
            int count = va_arg(valist, int);
            i++;
            if (count > 0) {
                uint32_t ipv4 = va_arg(valist, uint32_t);
                char buffer[50];
                RtlIpv4AddressToStringA((const in_addr*)&ipv4, buffer);
                printf(",\"%s\"", buffer);
            }
            for (int j = 1; j < count; j++) {
                const char* str = va_arg(valist, const char*);
                printf(",\"%s\"", str);
            }
            i += count;
            break;
        }
        case _tlgIPv6Address: {
            int count = va_arg(valist, int);
            i++;
            if (count > 0) {
                uint32_t* ipv6 = va_arg(valist, uint32_t*);
                char buffer[50];
                RtlIpv6AddressToStringA((const in6_addr*)ipv6, buffer);
                printf(",\"%s\"", buffer);
            }
            for (int j = 1; j < count; j++) {
                const char* str = va_arg(valist, const char*);
                printf(",\"%s\"", str);
            }
            i += count;
            break;
        }
        default:
            printf("<type %x>", type);
            break;
        }
    }
    va_end(valist);

    printf("}\n");
}
