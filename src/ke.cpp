// Copyright (c) Microsoft Corporation
// SPDX-License-Identifier: MIT

#include "platform.h"
#include "usersim/ke.h"
#include <format>
#include <sstream>
#include <vector>
#undef ASSERT
#define ASSERT(x) if (!(x)) KeBugCheckCPP(0)

#pragma comment(lib, "mincore.lib")

// Ke* functions.

ULONG
KeQueryMaximumProcessorCount() { return GetMaximumProcessorCount(ALL_PROCESSOR_GROUPS); }

ULONG
KeQueryMaximumProcessorCountEx(_In_ USHORT group_number) { return GetMaximumProcessorCount(group_number); }

#pragma region irqls

thread_local KIRQL _usersim_current_irql = PASSIVE_LEVEL;

KIRQL
KeGetCurrentIrql() { return _usersim_current_irql; }

VOID
KeRaiseIrql(_In_ KIRQL new_irql, _Out_ PKIRQL old_irql)
{
    *old_irql = KeGetCurrentIrql();
    _usersim_current_irql = new_irql;
    BOOL result = SetThreadPriority(GetCurrentThread(), new_irql);
    ASSERT(result);
}

KIRQL
KeRaiseIrqlToDpcLevel()
{
    KIRQL old_irql;
    KeRaiseIrql(DISPATCH_LEVEL, &old_irql);
    return old_irql;
}

void
KeLowerIrql(_In_ KIRQL new_irql)
{
    _usersim_current_irql = new_irql;
    BOOL result = SetThreadPriority(GetCurrentThread(), new_irql);
    ASSERT(result);
}

_IRQL_requires_min_(DISPATCH_LEVEL) NTKERNELAPI LOGICAL KeShouldYieldProcessor(VOID) { return false; }

#pragma endregion irqls

void
KeEnterCriticalRegion(void)
{}

void
KeLeaveCriticalRegion(void)
{}

#pragma region spin_locks

void
KeInitializeSpinLock(_Out_ PKSPIN_LOCK spin_lock)
{
    auto lock = reinterpret_cast<SRWLOCK*>(spin_lock);
    *lock = SRWLOCK_INIT;
}

_Requires_lock_not_held_(*spin_lock) _Acquires_lock_(*spin_lock)
    _IRQL_requires_max_(DISPATCH_LEVEL) void KeAcquireSpinLock(_Inout_ PKSPIN_LOCK spin_lock, _Out_ PKIRQL OldIrql)
{
    *(OldIrql) = KeAcquireSpinLockRaiseToDpc(spin_lock);
}

_Requires_lock_not_held_(*spin_lock) _Acquires_lock_(*spin_lock) _IRQL_requires_max_(DISPATCH_LEVEL) KIRQL
    KeAcquireSpinLockRaiseToDpc(_Inout_ PKSPIN_LOCK spin_lock)
{
    KIRQL old_irql = KeRaiseIrqlToDpcLevel();
    KeAcquireSpinLockAtDpcLevel(spin_lock);
    return old_irql;
}

_Requires_lock_not_held_(*spin_lock) _Acquires_lock_(*spin_lock)
    _IRQL_requires_(DISPATCH_LEVEL) void KeAcquireSpinLockAtDpcLevel(_Inout_ PKSPIN_LOCK spin_lock)
{
    // Skip Fault Injection.
    auto lock = reinterpret_cast<SRWLOCK*>(spin_lock);
    AcquireSRWLockExclusive(lock);
}

_Requires_lock_held_(*spin_lock) _Releases_lock_(*spin_lock)
    _IRQL_requires_(DISPATCH_LEVEL) void KeReleaseSpinLockFromDpcLevel(_Inout_ PKSPIN_LOCK spin_lock)
{
    auto lock = reinterpret_cast<SRWLOCK*>(spin_lock);
    ReleaseSRWLockExclusive(lock);
}

_Requires_lock_held_(*spin_lock) _Releases_lock_(*spin_lock) _IRQL_requires_(DISPATCH_LEVEL) void KeReleaseSpinLock(
    _Inout_ PKSPIN_LOCK spin_lock, _In_ _IRQL_restores_ KIRQL new_irql)
{
    KeReleaseSpinLockFromDpcLevel(spin_lock);
    KeLowerIrql(new_irql);
}

#pragma endregion spin_locks

unsigned long long
KeQueryInterruptTime()
{
    unsigned long long time = 0;
    QueryInterruptTime(&time);

    return time;
}

_IRQL_requires_min_(PASSIVE_LEVEL) _IRQL_requires_max_(APC_LEVEL) NTKERNELAPI VOID
    KeRevertToUserAffinityThreadEx(_In_ KAFFINITY affinity)
{
    SetThreadAffinityMask(GetCurrentThread(), affinity);
}

PKTHREAD
NTAPI
KeGetCurrentThread(VOID) { return (PKTHREAD)usersim_get_current_thread_id(); }

#pragma region semaphores

static std::vector<HANDLE>* g_usersim_semaphore_handles = nullptr;

_IRQL_requires_max_(DISPATCH_LEVEL) NTKERNELAPI VOID
    KeInitializeSemaphore(_Out_ PRKSEMAPHORE semaphore, _In_ LONG count, _In_ LONG limit)
{
    semaphore->handle = CreateSemaphore(nullptr, count, limit, nullptr);
    ASSERT(semaphore->handle != INVALID_HANDLE_VALUE);

    // There is no kernel function to uninitialize a semaphore, but there is in user mode,
    // so add the handle to a list we can clean up later.
    if (g_usersim_semaphore_handles == nullptr) {
        g_usersim_semaphore_handles = new std::vector<HANDLE>();
    }
    g_usersim_semaphore_handles->push_back(semaphore->handle);
}

void
usersim_free_semaphores()
{
    if (g_usersim_semaphore_handles) {
        for (auto handle : *g_usersim_semaphore_handles) {
            ::CloseHandle(handle);
        }
        usersim_free(g_usersim_semaphore_handles);
        g_usersim_semaphore_handles = nullptr;
    }
}

_When_(wait == 0, _IRQL_requires_max_(DISPATCH_LEVEL))
    _When_(wait == 1, _IRQL_requires_max_(APC_LEVEL)) NTKERNELAPI LONG KeReleaseSemaphore(
        _Inout_ PRKSEMAPHORE semaphore, _In_ KPRIORITY increment, _In_ LONG adjustment, _In_ _Literal_ BOOLEAN wait)
{
    UNREFERENCED_PARAMETER(increment);
    UNREFERENCED_PARAMETER(wait);
    LONG previous_count;
    ReleaseSemaphore(semaphore->handle, adjustment, &previous_count);
    ASSERT(previous_count >= 0);
    return previous_count;
}

_IRQL_requires_min_(PASSIVE_LEVEL) _When_((timeout == NULL || timeout->QuadPart != 0), _IRQL_requires_max_(APC_LEVEL))
    _When_((timeout != NULL && timeout->QuadPart == 0), _IRQL_requires_max_(DISPATCH_LEVEL)) NTKERNELAPI NTSTATUS
    KeWaitForSingleObject(
        _In_ _Points_to_data_ PVOID object,
        _In_ _Strict_type_match_ KWAIT_REASON wait_reason,
        _In_ __drv_strictType(KPROCESSOR_MODE / enum _MODE, __drv_typeConst) KPROCESSOR_MODE wait_mode,
        _In_ BOOLEAN alertable,
        _In_opt_ PLARGE_INTEGER timeout)
{
    UNREFERENCED_PARAMETER(wait_reason);
    UNREFERENCED_PARAMETER(wait_mode);
    UNREFERENCED_PARAMETER(alertable);

    DWORD timeout_ms = INFINITE;
    if (timeout != nullptr) {
        // Timeout is in 100-ns units.
        // 1 ms == 1000000 ns == 10000 units of 100 ns.
        timeout_ms = (DWORD)(timeout->QuadPart / 10000);
    }

    PRKSEMAPHORE semaphore = (PRKSEMAPHORE)object;
    DWORD result = WaitForSingleObject(semaphore->handle, timeout_ms);
    switch (result) {
    case WAIT_OBJECT_0:
        return STATUS_SUCCESS;
    case WAIT_TIMEOUT:
        return STATUS_TIMEOUT;
    case WAIT_ABANDONED:
    case WAIT_FAILED:
    default:
        return STATUS_UNSUCCESSFUL;
    }
}

// Returns 0 for non-signaled, non-zero for signaled.
LONG
KeReadStateSemaphore(_In_ PRKSEMAPHORE semaphore)
{
    DWORD result = WaitForSingleObject(semaphore->handle, 0);
    if (result == WAIT_TIMEOUT) {
        return 0; // Not signaled.
    }

    // Release the reference we just acquired.
    ReleaseSemaphore(semaphore->handle, 1, nullptr);

    // Report that the semaphore is signaled.
    return 1;
}

#pragma endregion semaphores

_IRQL_requires_max_(APC_LEVEL) NTKERNELAPI VOID
    KeStackAttachProcess(_Inout_ PRKPROCESS process, _Out_ PRKAPC_STATE apc_state)
{
    // This is a no-op for the user mode implementation.
    UNREFERENCED_PARAMETER(process);
    *apc_state = nullptr;
}

_IRQL_requires_max_(APC_LEVEL) NTKERNELAPI VOID KeUnstackDetachProcess(_In_ PRKAPC_STATE apc_state)
{
    UNREFERENCED_PARAMETER(apc_state);
}

_IRQL_requires_same_ ULONG64
KeQueryUnbiasedInterruptTimePrecise(_Out_ PULONG64 qpc_time_stamp)
{
    QueryUnbiasedInterruptTimePrecise(qpc_time_stamp);
    return *qpc_time_stamp;
}

_IRQL_requires_same_ ULONG64
KeQueryInterruptTimePrecise(_Out_ PULONG64 qpc_time_stamp)
{
    QueryInterruptTimePrecise(qpc_time_stamp);
    return *qpc_time_stamp;
}

LARGE_INTEGER
KeQueryPerformanceCounter(_Out_opt_ PLARGE_INTEGER performance_frequency)
{
    LARGE_INTEGER counter;
    BOOL ok = QueryPerformanceCounter(&counter);
    ASSERT(ok);
    if (performance_frequency != nullptr) {
        ok = QueryPerformanceFrequency(performance_frequency);
        ASSERT(ok);
    }
    return counter;
}

void
KeBugCheck(ULONG bug_check_code)
{
    KeBugCheckEx(bug_check_code, 0, 0, 0, 0);
}

void
KeBugCheckCPP(ULONG bug_check_code)
{
    KeBugCheckExCPP(bug_check_code, 0, 0, 0, 0);
}

static void
_throw_exception(_In_ std::string message)
{
    throw std::exception(message.c_str());
}

void
KeBugCheckExCPP(
    ULONG bug_check_code,
    ULONG_PTR bug_check_parameter1,
    ULONG_PTR bug_check_parameter2,
    ULONG_PTR bug_check_parameter3,
    ULONG_PTR bug_check_parameter4)
{
    std::ostringstream ss;
    ss << std::format(
        "*** STOP {:#08x} ({:#016x},{:#016x},{:#016x},{:#016x})",
        bug_check_code,
        bug_check_parameter1,
        bug_check_parameter2,
        bug_check_parameter3,
        bug_check_parameter4);
    _throw_exception(ss.str());
}

void
KeBugCheckEx(
    ULONG bug_check_code,
    ULONG_PTR bug_check_parameter1,
    ULONG_PTR bug_check_parameter2,
    ULONG_PTR bug_check_parameter3,
    ULONG_PTR bug_check_parameter4)
{
    KeBugCheckExCPP(
        bug_check_code, bug_check_parameter1, bug_check_parameter2, bug_check_parameter3, bug_check_parameter4);
}