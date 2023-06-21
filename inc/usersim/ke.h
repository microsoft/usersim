// Copyright (c) Microsoft Corporation
// SPDX-License-Identifier: MIT

#pragma once
#include "..\src\platform.h"

#if defined(__cplusplus)
extern "C"
{
#endif

    typedef CCHAR KPROCESSOR_MODE;
    typedef struct _ksemaphore
    {
        HANDLE handle;
    } KSEMAPHORE;
    typedef KSEMAPHORE* PKSEMAPHORE;
    typedef KSEMAPHORE* PRKSEMAPHORE;
    typedef struct _kprocess KPROCESS;
    typedef KPROCESS* PRKPROCESS;
    typedef KPROCESS* PEPROCESS;
    typedef HANDLE KAPC_STATE;
    typedef KAPC_STATE* PRKAPC_STATE;
    typedef uint8_t KIRQL;
    typedef KIRQL* PKIRQL;
    typedef enum _KWAIT_REASON
    {
        Executive,
    } KWAIT_REASON;

    _Requires_lock_not_held_(*spin_lock) _Acquires_lock_(*spin_lock) _IRQL_requires_max_(DISPATCH_LEVEL)
    void KeAcquireSpinLock(_Inout_ PKSPIN_LOCK spin_lock, _Out_ PKIRQL old_irql);

    void
    KeEnterCriticalRegion(void);

    void
    KeLeaveCriticalRegion(void);

    void
    KeInitializeSpinLock(_Out_ PKSPIN_LOCK spin_lock);

    _Requires_lock_not_held_(*spin_lock) _Acquires_lock_(*spin_lock) _IRQL_requires_max_(DISPATCH_LEVEL) KIRQL
    KeAcquireSpinLockRaiseToDpc(_Inout_ PKSPIN_LOCK spin_lock);

    _Requires_lock_held_(*spin_lock) _Releases_lock_(*spin_lock) _IRQL_requires_(DISPATCH_LEVEL)
    void KeReleaseSpinLock(
        _Inout_ PKSPIN_LOCK spin_lock, _In_ _IRQL_restores_ KIRQL new_irql);

    KIRQL
    KeGetCurrentIrql();

    VOID
    KeRaiseIrql(_In_ KIRQL new_irql, _Out_ PKIRQL old_irql);

    KIRQL
    KeRaiseIrqlToDpcLevel();

    void
    KeLowerIrql(_In_ KIRQL new_irql);

    NTKERNELAPI
    ULONG
    KeQueryMaximumProcessorCountEx(_In_ USHORT group_number);

    unsigned long long
    KeQueryInterruptTime();

    _IRQL_requires_min_(PASSIVE_LEVEL) _IRQL_requires_max_(APC_LEVEL) NTKERNELAPI VOID
        KeRevertToUserAffinityThreadEx(_In_ KAFFINITY affinity);

    typedef struct _kthread* PKTHREAD;

    PKTHREAD
    NTAPI
    KeGetCurrentThread(VOID);

    _IRQL_requires_min_(DISPATCH_LEVEL) NTKERNELAPI LOGICAL KeShouldYieldProcessor(VOID);

    _IRQL_requires_max_(DISPATCH_LEVEL) NTKERNELAPI VOID
        KeInitializeSemaphore(_Out_ PRKSEMAPHORE semaphore, _In_ LONG count, _In_ LONG limit);

    typedef ULONG KPRIORITY;

    _When_(wait == 0, _IRQL_requires_max_(DISPATCH_LEVEL))
    _When_(wait == 1, _IRQL_requires_max_(APC_LEVEL))
    NTKERNELAPI LONG
    KeReleaseSemaphore(
        _Inout_ PRKSEMAPHORE semaphore,
        _In_ KPRIORITY increment,
        _In_ LONG adjustment,
        _In_ _Literal_ BOOLEAN wait);

    _IRQL_requires_max_(APC_LEVEL) NTKERNELAPI VOID
        KeStackAttachProcess(_Inout_ PRKPROCESS process, _Out_ PRKAPC_STATE apc_state);

    _IRQL_requires_max_(APC_LEVEL) NTKERNELAPI VOID KeUnstackDetachProcess(_In_ PRKAPC_STATE api_state);

    _IRQL_requires_min_(PASSIVE_LEVEL)
    _When_((timeout == NULL || timeout->QuadPart != 0), _IRQL_requires_max_(APC_LEVEL))
    _When_((timeout != NULL && timeout->QuadPart == 0), _IRQL_requires_max_(DISPATCH_LEVEL))
    NTKERNELAPI NTSTATUS KeWaitForSingleObject(
        _In_ _Points_to_data_ PVOID object,
        _In_ _Strict_type_match_ KWAIT_REASON wait_reason,
        _In_ __drv_strictType(KPROCESSOR_MODE / enum _MODE, __drv_typeConst) KPROCESSOR_MODE wait_mode,
        _In_ BOOLEAN alertable,
        _In_opt_ PLARGE_INTEGER timeout);

_IRQL_requires_same_ ULONG64
    KeQueryUnbiasedInterruptTimePrecise(_Out_ PULONG64 qpc_time_stamp);

_IRQL_requires_same_ ULONG64
KeQueryInterruptTimePrecise(_Out_ PULONG64 qpc_time_stamp);

#if defined(__cplusplus)
}
#endif