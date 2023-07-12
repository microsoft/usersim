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

    typedef enum
    {
        IRQL_NOT_LESS_OR_EQUAL = 0x0A,
        TIMER_OR_DPC_INVALID = 0xC7,
        DRIVER_UNMAPPING_INVALID_VIEW = 0xD7,
    } usersim_bug_check_code_t;

    __declspec(dllexport)
    void
    KeEnterCriticalRegion(void);

    __declspec(dllexport)
    void
    KeLeaveCriticalRegion(void);

    __declspec(dllexport)
    ULONG
    KeGetCurrentProcessorNumberEx(_Out_opt_ PPROCESSOR_NUMBER ProcNumber);

#pragma region irqls
    typedef uint8_t KIRQL;
    typedef KIRQL* PKIRQL;

#undef PASSIVE_LEVEL
#undef APC_LEVEL
#undef DISPATCH_LEVEL
#define PASSIVE_LEVEL THREAD_PRIORITY_NORMAL   // Passive release level.
#define APC_LEVEL THREAD_PRIORITY_ABOVE_NORMAL // APC interrupt level.
#define DISPATCH_LEVEL THREAD_PRIORITY_TIME_CRITICAL // Dispatcher level.

    __declspec(dllexport)
    KIRQL
    KeGetCurrentIrql();

    __declspec(dllexport)
    VOID
    KeRaiseIrql(_In_ KIRQL new_irql, _Out_ PKIRQL old_irql);

    __declspec(dllexport)
    KIRQL
    KeRaiseIrqlToDpcLevel();

    __declspec(dllexport)
    void
    KeLowerIrql(_In_ KIRQL new_irql);

    __declspec(dllexport)
    _IRQL_requires_min_(DISPATCH_LEVEL) NTKERNELAPI LOGICAL KeShouldYieldProcessor(VOID);

#pragma endregion irqls

#pragma region spin_locks

    __declspec(dllexport)
    void
    KeInitializeSpinLock(_Out_ PKSPIN_LOCK spin_lock);

    __declspec(dllexport)
    _Requires_lock_not_held_(*spin_lock) _Acquires_lock_(*spin_lock) _IRQL_requires_max_(
        DISPATCH_LEVEL) void KeAcquireSpinLock(_Inout_ PKSPIN_LOCK spin_lock, _Out_ PKIRQL old_irql);

    __declspec(dllexport)
    _Requires_lock_not_held_(*spin_lock) _Acquires_lock_(*spin_lock) _IRQL_requires_max_(DISPATCH_LEVEL) KIRQL
        KeAcquireSpinLockRaiseToDpc(_Inout_ PKSPIN_LOCK spin_lock);

    __declspec(dllexport)
    _Requires_lock_not_held_(*spin_lock) _Acquires_lock_(*spin_lock)
        _IRQL_requires_(DISPATCH_LEVEL) void KeAcquireSpinLockAtDpcLevel(_Inout_ PKSPIN_LOCK spin_lock);

    __declspec(dllexport)
    _Requires_lock_held_(*spin_lock) _Releases_lock_(*spin_lock)
        _IRQL_requires_(DISPATCH_LEVEL) void KeReleaseSpinLockFromDpcLevel(_Inout_ PKSPIN_LOCK spin_lock);

    __declspec(dllexport)
    _Requires_lock_held_(*spin_lock) _Releases_lock_(*spin_lock) _IRQL_requires_(DISPATCH_LEVEL) void KeReleaseSpinLock(
        _Inout_ PKSPIN_LOCK spin_lock, _In_ _IRQL_restores_ KIRQL new_irql);

#pragma endregion spin_locks

    __declspec(dllexport)
    unsigned long long
    KeQueryInterruptTime();

#pragma region threads

    __declspec(dllexport)
    ULONG
    KeQueryMaximumProcessorCount();

    __declspec(dllexport)
    ULONG
    KeQueryMaximumProcessorCountEx(_In_ USHORT group_number);

#define KeQueryActiveProcessorCount KeQueryMaximumProcessorCount
#define KeQueryActiveProcessorCountEx KeQueryMaximumProcessorCountEx

    __declspec(dllexport)
    KAFFINITY
    KeSetSystemAffinityThreadEx(KAFFINITY affinity);

    __declspec(dllexport)
    _IRQL_requires_min_(PASSIVE_LEVEL) _IRQL_requires_max_(APC_LEVEL) NTKERNELAPI VOID
        KeRevertToUserAffinityThreadEx(_In_ KAFFINITY affinity);

    typedef struct _kthread* PKTHREAD;

    __declspec(dllexport)
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

    __declspec(dllexport)
    _IRQL_requires_max_(DISPATCH_LEVEL) NTKERNELAPI
        void KeInitializeSemaphore(_Out_ PRKSEMAPHORE semaphore, _In_ LONG count, _In_ LONG limit);

    typedef ULONG KPRIORITY;

    __declspec(dllexport)
    _When_(wait == 0, _IRQL_requires_max_(DISPATCH_LEVEL))
        _When_(wait == 1, _IRQL_requires_max_(APC_LEVEL)) NTKERNELAPI LONG KeReleaseSemaphore(
            _Inout_ PRKSEMAPHORE semaphore,
            _In_ KPRIORITY increment,
            _In_ LONG adjustment,
            _In_ _Literal_ BOOLEAN wait);

    __declspec(dllexport)
    LONG
    KeReadStateSemaphore(_In_ PRKSEMAPHORE semaphore);

    void
    usersim_free_semaphores();

#pragma endregion semaphores

    __declspec(dllexport)
    _IRQL_requires_max_(APC_LEVEL) NTKERNELAPI VOID
        KeStackAttachProcess(_Inout_ PRKPROCESS process, _Out_ PRKAPC_STATE apc_state);

    __declspec(dllexport)
    _IRQL_requires_max_(APC_LEVEL) NTKERNELAPI VOID KeUnstackDetachProcess(_In_ PRKAPC_STATE api_state);

    __declspec(dllexport)
    _IRQL_requires_min_(PASSIVE_LEVEL)
        _When_((timeout == NULL || timeout->QuadPart != 0), _IRQL_requires_max_(APC_LEVEL))
            _When_((timeout != NULL && timeout->QuadPart == 0), _IRQL_requires_max_(DISPATCH_LEVEL))
                NTKERNELAPI NTSTATUS KeWaitForSingleObject(
                    _In_ _Points_to_data_ PVOID object,
                    _In_ _Strict_type_match_ KWAIT_REASON wait_reason,
                    _In_ __drv_strictType(KPROCESSOR_MODE / enum _MODE, __drv_typeConst) KPROCESSOR_MODE wait_mode,
                    _In_ BOOLEAN alertable,
                    _In_opt_ PLARGE_INTEGER timeout);

    __declspec(dllexport)
    _IRQL_requires_same_ ULONG64
    KeQueryUnbiasedInterruptTimePrecise(_Out_ PULONG64 qpc_time_stamp);

    __declspec(dllexport)
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

    __declspec(dllexport)
    void
    KeInitializeDpc(_Out_ __drv_aliasesMem PRKDPC dpc, _In_ PKDEFERRED_ROUTINE deferred_routine, _In_opt_ __drv_aliasesMem PVOID deferred_context);

    __declspec(dllexport)
    BOOLEAN KeInsertQueueDpc(_Inout_ PRKDPC dpc, _In_opt_ PVOID system_argument1, _In_opt_ __drv_aliasesMem PVOID system_argument2);

    __declspec(dllexport)
    BOOLEAN KeRemoveQueueDpc(_Inout_ PRKDPC dpc);

    __declspec(dllexport)
    void
    KeFlushQueuedDpcs();

    __declspec(dllexport)
    void KeSetTargetProcessorDpc(_Inout_ PRKDPC dpc, CCHAR number);

    void
    usersim_initialize_dpcs();

    void
    usersim_clean_up_dpcs();

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

    __declspec(dllexport)
    void
    KeInitializeTimer(_Out_ PKTIMER timer);

    __declspec(dllexport)
    BOOLEAN
    KeSetTimer(_Inout_ PKTIMER timer, LARGE_INTEGER due_time, _In_opt_ PKDPC dpc);

    __declspec(dllexport)
    BOOLEAN
    KeSetTimerEx(_Inout_ PKTIMER timer, LARGE_INTEGER due_time, ULONG period, _In_opt_ PKDPC dpc);

    __declspec(dllexport)
    BOOLEAN
    KeSetCoalescableTimer(_Inout_ PKTIMER timer, LARGE_INTEGER due_time, ULONG period, ULONG tolerable_delay, _In_opt_ PKDPC dpc);

    __declspec(dllexport)
    BOOLEAN KeCancelTimer(_Inout_ PKTIMER timer);

    __declspec(dllexport)
    BOOLEAN
    KeReadStateTimer(_In_ PKTIMER timer);

    void
    usersim_free_threadpool_timers();

#pragma endregion timers

    __declspec(dllexport)
    LARGE_INTEGER KeQueryPerformanceCounter(_Out_opt_ PLARGE_INTEGER performance_frequency);

    __declspec(dllexport)
    void
    KeBugCheck(ULONG bug_check_code);

    __declspec(dllexport)
    void
    KeBugCheckEx(
        ULONG bug_check_code,
        ULONG_PTR bug_check_parameter1,
        ULONG_PTR bug_check_parameter2,
        ULONG_PTR bug_check_parameter3,
        ULONG_PTR bug_check_parameter4);

#if defined(__cplusplus)
}

// The bug check functions below throw C++ exceptions so tests can catch them to verify error behavior.
__declspec(dllexport) void
KeBugCheckCPP(ULONG bug_check_code);

__declspec(dllexport) void
KeBugCheckExCPP(
    ULONG bug_check_code,
    ULONG_PTR bug_check_parameter1,
    ULONG_PTR bug_check_parameter2,
    ULONG_PTR bug_check_parameter3,
    ULONG_PTR bug_check_parameter4);

#endif
