// Copyright (c) Microsoft Corporation
// SPDX-License-Identifier: MIT

#pragma once
#include "..\src\platform.h"

#if defined(__cplusplus)
extern "C"
{
#endif

    typedef CCHAR KPROCESSOR_MODE;
    typedef struct _kprocess KPROCESS;
    typedef KPROCESS* PRKPROCESS;
    typedef KPROCESS* PEPROCESS;
    typedef HANDLE KAPC_STATE;
    typedef KAPC_STATE* PRKAPC_STATE;

    typedef enum _KWAIT_REASON
    {
        Executive,
    } KWAIT_REASON;

    typedef enum _usersim_object_type
    {
        USERSIM_OBJECT_TYPE_UNKNOWN,
        USERSIM_OBJECT_TYPE_SEMAPHORE,
        USERSIM_OBJECT_TYPE_TIMER,
    } usersim_object_type_t;

    void
    KeEnterCriticalRegion(void);

    void
    KeLeaveCriticalRegion(void);

#pragma region irqls
    typedef uint8_t KIRQL;
    typedef KIRQL* PKIRQL;

#undef PASSIVE_LEVEL
#undef APC_LEVEL
#undef DISPATCH_LEVEL
#define PASSIVE_LEVEL THREAD_PRIORITY_NORMAL   // Passive release level.
#define APC_LEVEL THREAD_PRIORITY_ABOVE_NORMAL // APC interrupt level.
#define DISPATCH_LEVEL THREAD_PRIORITY_HIGHEST // Dispatcher level.

    KIRQL
    KeGetCurrentIrql();

    VOID
    KeRaiseIrql(_In_ KIRQL new_irql, _Out_ PKIRQL old_irql);

    KIRQL
    KeRaiseIrqlToDpcLevel();

    void
    KeLowerIrql(_In_ KIRQL new_irql);

    _IRQL_requires_min_(DISPATCH_LEVEL) NTKERNELAPI LOGICAL KeShouldYieldProcessor(VOID);

#pragma endregion irqls

#pragma region spin_locks

    void
    KeInitializeSpinLock(_Out_ PKSPIN_LOCK spin_lock);

    _Requires_lock_not_held_(*spin_lock) _Acquires_lock_(*spin_lock) _IRQL_requires_max_(
        DISPATCH_LEVEL) void KeAcquireSpinLock(_Inout_ PKSPIN_LOCK spin_lock, _Out_ PKIRQL old_irql);

    _Requires_lock_not_held_(*spin_lock) _Acquires_lock_(*spin_lock) _IRQL_requires_max_(DISPATCH_LEVEL) KIRQL
        KeAcquireSpinLockRaiseToDpc(_Inout_ PKSPIN_LOCK spin_lock);

    _Requires_lock_not_held_(*spin_lock) _Acquires_lock_(*spin_lock)
        _IRQL_requires_(DISPATCH_LEVEL) void KeAcquireSpinLockAtDpcLevel(_Inout_ PKSPIN_LOCK spin_lock);

    _Requires_lock_held_(*spin_lock) _Releases_lock_(*spin_lock)
        _IRQL_requires_(DISPATCH_LEVEL) void KeReleaseSpinLockFromDpcLevel(_Inout_ PKSPIN_LOCK spin_lock);

    _Requires_lock_held_(*spin_lock) _Releases_lock_(*spin_lock) _IRQL_requires_(DISPATCH_LEVEL) void KeReleaseSpinLock(
        _Inout_ PKSPIN_LOCK spin_lock, _In_ _IRQL_restores_ KIRQL new_irql);

#pragma endregion spin_locks

    unsigned long long
    KeQueryInterruptTime();

#pragma region threads

    ULONG
    KeQueryMaximumProcessorCount();

    ULONG
    KeQueryMaximumProcessorCountEx(_In_ USHORT group_number);

#define KeQueryActiveProcessorCount KeQueryMaximumProcessorCount
#define KeQueryActiveProcessorCountEx KeQueryMaximumProcessorCountEx

    KAFFINITY
    KeSetSystemAffinityThreadEx(KAFFINITY affinity);

    _IRQL_requires_min_(PASSIVE_LEVEL) _IRQL_requires_max_(APC_LEVEL) NTKERNELAPI VOID
        KeRevertToUserAffinityThreadEx(_In_ KAFFINITY affinity);

    typedef struct _kthread* PKTHREAD;

    PKTHREAD
    NTAPI
    KeGetCurrentThread(VOID);

#pragma endregion threads

#pragma region semaphores

    typedef struct _ksemaphore
    {
        usersim_object_type_t object_type;
        HANDLE handle;
    } KSEMAPHORE;
    typedef KSEMAPHORE* PKSEMAPHORE;
    typedef KSEMAPHORE* PRKSEMAPHORE;

    _IRQL_requires_max_(DISPATCH_LEVEL) NTKERNELAPI
        void KeInitializeSemaphore(_Out_ PRKSEMAPHORE semaphore, _In_ LONG count, _In_ LONG limit);

    typedef ULONG KPRIORITY;

    _When_(wait == 0, _IRQL_requires_max_(DISPATCH_LEVEL))
        _When_(wait == 1, _IRQL_requires_max_(APC_LEVEL)) NTKERNELAPI LONG KeReleaseSemaphore(
            _Inout_ PRKSEMAPHORE semaphore,
            _In_ KPRIORITY increment,
            _In_ LONG adjustment,
            _In_ _Literal_ BOOLEAN wait);

    LONG
    KeReadStateSemaphore(_In_ PRKSEMAPHORE semaphore);

    void
    usersim_free_semaphores();

#pragma endregion semaphores

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

#pragma region dpcs

    typedef struct _KDPC KDPC;
    typedef KDPC* PKDPC;
    typedef KDPC* PRKDPC;

    typedef void (KDEFERRED_ROUTINE)(
        _In_ KDPC* dpc,
        _In_opt_ void* deferred_context,
        _In_opt_ void* system_argument1,
        _In_opt_ void* system_argument2);
    typedef KDEFERRED_ROUTINE* PKDEFERRED_ROUTINE;

    struct _KDPC
    {
        usersim_list_entry_t entry;
        void* context;
        CCHAR cpu_id;
        void* parameter_1;
        void* parameter_2;
        PKDEFERRED_ROUTINE work_item_routine;
    };

    void
    KeInitializeDpc(_Out_ __drv_aliasesMem PRKDPC dpc, _In_ PKDEFERRED_ROUTINE deferred_routine, _In_opt_ __drv_aliasesMem PVOID deferred_context);

    BOOLEAN KeInsertQueueDpc(_Inout_ PRKDPC dpc, _In_opt_ PVOID system_argument1, _In_opt_ __drv_aliasesMem PVOID system_argument2);

    BOOLEAN KeRemoveQueueDpc(_Inout_ PRKDPC dpc);

    void
    KeFlushQueuedDpcs();

    void KeSetTargetProcessorDpc(_Inout_ PRKDPC dpc, CCHAR number);

#pragma endregion dpcs
#pragma region timers

    typedef struct _ktimer
    {
        usersim_object_type_t object_type;
        TP_TIMER* threadpool_timer;
        KDPC* dpc;
        BOOLEAN signaled;
    } KTIMER;
    typedef KTIMER* PKTIMER;

    void
    KeInitializeTimer(_Out_ PKTIMER timer);

    BOOLEAN
    KeSetTimer(_Inout_ PKTIMER timer, LARGE_INTEGER due_time, _In_opt_ PKDPC dpc);

    BOOLEAN
    KeSetTimerEx(_Inout_ PKTIMER timer, LARGE_INTEGER due_time, ULONG period, _In_opt_ PKDPC dpc);

    BOOLEAN
    KeSetCoalescableTimer(_Inout_ PKTIMER timer, LARGE_INTEGER due_time, ULONG period, ULONG tolerable_delay, _In_opt_ PKDPC dpc);

    BOOLEAN KeCancelTimer(_Inout_ PKTIMER timer);

    BOOLEAN
    KeReadStateTimer(_In_ PKTIMER timer);

    void
    usersim_free_threadpool_timers();

#pragma endregion timers

    LARGE_INTEGER KeQueryPerformanceCounter(_Out_opt_ PLARGE_INTEGER performance_frequency);

    void
    KeBugCheck(ULONG bug_check_code);

    void
    KeBugCheckEx(
        ULONG bug_check_code,
        ULONG_PTR bug_check_parameter1,
        ULONG_PTR bug_check_parameter2,
        ULONG_PTR bug_check_parameter3,
        ULONG_PTR bug_check_parameter4);

#if defined(__cplusplus)
}
#endif

// The bug check functions below throw C++ exceptions so tests can catch them to verify error behavior.
void
KeBugCheckCPP(ULONG bug_check_code);

void
KeBugCheckExCPP(
    ULONG bug_check_code,
    ULONG_PTR bug_check_parameter1,
    ULONG_PTR bug_check_parameter2,
    ULONG_PTR bug_check_parameter3,
    ULONG_PTR bug_check_parameter4);
