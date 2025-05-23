// Copyright (c) Microsoft Corporation
// SPDX-License-Identifier: MIT

#pragma once
#include "..\src\platform.h"

CXPLAT_EXTERN_C_BEGIN

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
    USERSIM_OBJECT_TYPE_EVENT,
} usersim_object_type_t;

typedef enum
{
    IRQL_NOT_LESS_OR_EQUAL = 0x0A,
    TIMER_OR_DPC_INVALID = 0xC7,
    DRIVER_UNMAPPING_INVALID_VIEW = 0xD7,
} usersim_bug_check_code_t;

USERSIM_API
void
KeEnterCriticalRegion(void);

USERSIM_API
void
KeLeaveCriticalRegion(void);

USERSIM_API
ULONG
KeGetCurrentProcessorNumberEx(_Out_opt_ PPROCESSOR_NUMBER ProcNumber);

USERSIM_API
NTSTATUS
KeGetProcessorNumberFromIndex(ULONG ProcessorIndex, _Out_ PPROCESSOR_NUMBER ProcNumber);

USERSIM_API
ULONG
KeGetProcessorIndexFromNumber(_In_ PPROCESSOR_NUMBER ProcNumber);

#pragma region irqls
typedef uint8_t KIRQL;
typedef KIRQL* PKIRQL;

USERSIM_API
KIRQL
KeGetCurrentIrql();

USERSIM_API
VOID
KeRaiseIrql(_In_ KIRQL new_irql, _Out_ PKIRQL old_irql);

USERSIM_API
_IRQL_requires_max_(HIGH_LEVEL) _IRQL_raises_(new_irql) _IRQL_saves_ KIRQL KfRaiseIrql(_In_ KIRQL new_irql);

USERSIM_API
KIRQL
KeRaiseIrqlToDpcLevel();

USERSIM_API
void
KeLowerIrql(_In_ KIRQL new_irql);

USERSIM_API
void
KfLowerIrql(_In_ KIRQL new_irql);

USERSIM_API
_IRQL_requires_min_(DISPATCH_LEVEL) NTKERNELAPI LOGICAL KeShouldYieldProcessor(VOID);

usersim_result_t
usersim_initialize_irql();

void
usersim_clean_up_irql();

USERSIM_API
bool
usersim_set_current_thread_priority(int priority, int* old_priority);

USERSIM_API
bool
usersim_set_current_thread_affinity(const GROUP_AFFINITY* new_affinity, GROUP_AFFINITY* old_affinity);

#pragma endregion irqls

#pragma region spin_locks

USERSIM_API
void
KeInitializeSpinLock(_Out_ PKSPIN_LOCK spin_lock);

USERSIM_API
_Requires_lock_not_held_(*spin_lock) _Acquires_lock_(*spin_lock)
    _IRQL_requires_max_(DISPATCH_LEVEL) void KeAcquireSpinLock(_Inout_ PKSPIN_LOCK spin_lock, _Out_ PKIRQL old_irql);

USERSIM_API
_Requires_lock_not_held_(*spin_lock) _Acquires_lock_(*spin_lock) _IRQL_requires_max_(DISPATCH_LEVEL) KIRQL
    KeAcquireSpinLockRaiseToDpc(_Inout_ PKSPIN_LOCK spin_lock);

USERSIM_API
_Requires_lock_not_held_(*spin_lock) _Acquires_lock_(*spin_lock)
    _IRQL_requires_(DISPATCH_LEVEL) void KeAcquireSpinLockAtDpcLevel(_Inout_ PKSPIN_LOCK spin_lock);

USERSIM_API
_Requires_lock_held_(*spin_lock) _Releases_lock_(*spin_lock)
    _IRQL_requires_(DISPATCH_LEVEL) void KeReleaseSpinLockFromDpcLevel(_Inout_ PKSPIN_LOCK spin_lock);

USERSIM_API
_Requires_lock_held_(*spin_lock) _Releases_lock_(*spin_lock) _IRQL_requires_(DISPATCH_LEVEL) void KeReleaseSpinLock(
    _Inout_ PKSPIN_LOCK spin_lock, _In_ _IRQL_restores_ KIRQL new_irql);

#pragma endregion spin_locks

USERSIM_API
unsigned long long
KeQueryInterruptTime();

#pragma region threads

USERSIM_API
ULONG
KeQueryMaximumProcessorCount();

USERSIM_API
ULONG
KeQueryMaximumProcessorCountEx(_In_ USHORT group_number);

USERSIM_API
ULONG
KeQueryActiveProcessorCount();

USERSIM_API
ULONG
KeQueryActiveProcessorCountEx(_In_ USHORT group_number);

USERSIM_API
KAFFINITY
KeSetSystemAffinityThreadEx(KAFFINITY affinity);

USERSIM_API
_IRQL_requires_min_(PASSIVE_LEVEL) _IRQL_requires_max_(APC_LEVEL) NTKERNELAPI VOID
    KeRevertToUserAffinityThreadEx(_In_ KAFFINITY affinity);

USERSIM_API
void KeSetSystemGroupAffinityThread(
  _In_            PGROUP_AFFINITY Affinity,
  _Out_opt_       PGROUP_AFFINITY PreviousAffinity
);

USERSIM_API
void KeRevertToUserGroupAffinityThread(
  PGROUP_AFFINITY PreviousAffinity
);

typedef struct _kthread* PKTHREAD;

USERSIM_API
PKTHREAD
NTAPI
KeGetCurrentThread(VOID);

void
usersim_get_current_thread_group_affinity(_Out_ GROUP_AFFINITY* Affinity);

#pragma endregion threads

#pragma region semaphores

typedef struct _ksemaphore
{
    usersim_object_type_t object_type;
    HANDLE handle;
} KSEMAPHORE;
typedef KSEMAPHORE* PKSEMAPHORE;
typedef KSEMAPHORE* PRKSEMAPHORE;

USERSIM_API
_IRQL_requires_max_(DISPATCH_LEVEL) NTKERNELAPI
    void KeInitializeSemaphore(_Out_ PRKSEMAPHORE semaphore, _In_ LONG count, _In_ LONG limit);

typedef ULONG KPRIORITY;

USERSIM_API
_When_(wait == 0, _IRQL_requires_max_(DISPATCH_LEVEL))
    _When_(wait == 1, _IRQL_requires_max_(APC_LEVEL)) NTKERNELAPI LONG KeReleaseSemaphore(
        _Inout_ PRKSEMAPHORE semaphore, _In_ KPRIORITY increment, _In_ LONG adjustment, _In_ _Literal_ BOOLEAN wait);

USERSIM_API
LONG
KeReadStateSemaphore(_In_ PRKSEMAPHORE semaphore);

void
usersim_free_semaphores();

#pragma endregion semaphores

#pragma region events

typedef enum _EVENT_TYPE
{
    NotificationEvent,
    SynchronizationEvent
} EVENT_TYPE;

typedef struct _kevent
{
    usersim_object_type_t object_type;
    EVENT_TYPE type;
    KSPIN_LOCK spin_lock;
    BOOLEAN signaled;
} KEVENT;
typedef KEVENT* PKEVENT;
typedef KEVENT* PRKEVENT;

USERSIM_API
void
KeInitializeEvent(_Out_ PKEVENT event, _In_ EVENT_TYPE type, _In_ BOOLEAN initial_state);

USERSIM_API
LONG
KeSetEvent(_Inout_ PRKEVENT event, _In_ KPRIORITY increment, _In_ _Literal_ BOOLEAN wait);

USERSIM_API
void
KeClearEvent(_Inout_ PRKEVENT event);

#pragma endregion events

USERSIM_API
_IRQL_requires_max_(APC_LEVEL) NTKERNELAPI VOID
    KeStackAttachProcess(_Inout_ PRKPROCESS process, _Out_ PRKAPC_STATE apc_state);

USERSIM_API
_IRQL_requires_max_(APC_LEVEL) NTKERNELAPI VOID KeUnstackDetachProcess(_In_ PRKAPC_STATE api_state);

USERSIM_API
_IRQL_requires_min_(PASSIVE_LEVEL) _When_((timeout == NULL || timeout->QuadPart != 0), _IRQL_requires_max_(APC_LEVEL))
    _When_((timeout != NULL && timeout->QuadPart == 0), _IRQL_requires_max_(DISPATCH_LEVEL)) NTKERNELAPI NTSTATUS
    KeWaitForSingleObject(
        _In_ _Points_to_data_ PVOID object,
        _In_ _Strict_type_match_ KWAIT_REASON wait_reason,
        _In_ __drv_strictType(KPROCESSOR_MODE / enum _MODE, __drv_typeConst) KPROCESSOR_MODE wait_mode,
        _In_ BOOLEAN alertable,
        _In_opt_ PLARGE_INTEGER timeout);

USERSIM_API
_IRQL_requires_same_ ULONG64
KeQueryUnbiasedInterruptTimePrecise(_Out_ PULONG64 qpc_time_stamp);

USERSIM_API
_IRQL_requires_same_ ULONG64
KeQueryInterruptTimePrecise(_Out_ PULONG64 qpc_time_stamp);

#pragma region dpcs

typedef struct _KDPC KDPC;
typedef KDPC* PKDPC;
typedef KDPC* PRKDPC;

typedef void(KDEFERRED_ROUTINE)(
    _In_ KDPC* dpc, _In_opt_ void* deferred_context, _In_opt_ void* system_argument1, _In_opt_ void* system_argument2);
typedef KDEFERRED_ROUTINE* PKDEFERRED_ROUTINE;

struct _KDPC
{
    usersim_list_entry_t entry;
    void* context;
    uint32_t cpu_id;
    void* parameter_1;
    void* parameter_2;
    PKDEFERRED_ROUTINE work_item_routine;
};

USERSIM_API
void
KeInitializeDpc(
    _Out_ __drv_aliasesMem PRKDPC dpc,
    _In_ PKDEFERRED_ROUTINE deferred_routine,
    _In_opt_ __drv_aliasesMem PVOID deferred_context);

USERSIM_API
BOOLEAN
KeInsertQueueDpc(_Inout_ PRKDPC dpc, _In_opt_ PVOID system_argument1, _In_opt_ __drv_aliasesMem PVOID system_argument2);

USERSIM_API
BOOLEAN
KeRemoveQueueDpc(_Inout_ PRKDPC dpc);

USERSIM_API
void
KeFlushQueuedDpcs();

USERSIM_API
void
KeSetTargetProcessorDpc(_Inout_ PRKDPC dpc, CCHAR number);

USERSIM_API
NTSTATUS
KeSetTargetProcessorDpcEx(_Inout_ PRKDPC dpc, PPROCESSOR_NUMBER proc_number);

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

USERSIM_API
void
KeInitializeTimer(_Out_ PKTIMER timer);

USERSIM_API
BOOLEAN
KeSetTimer(_Inout_ PKTIMER timer, LARGE_INTEGER due_time, _In_opt_ PKDPC dpc);

USERSIM_API
BOOLEAN
KeSetTimerEx(_Inout_ PKTIMER timer, LARGE_INTEGER due_time, ULONG period, _In_opt_ PKDPC dpc);

USERSIM_API
BOOLEAN
KeSetCoalescableTimer(
    _Inout_ PKTIMER timer, LARGE_INTEGER due_time, ULONG period, ULONG tolerable_delay, _In_opt_ PKDPC dpc);

USERSIM_API
BOOLEAN
KeCancelTimer(_Inout_ PKTIMER timer);

USERSIM_API
BOOLEAN
KeReadStateTimer(_In_ PKTIMER timer);

void
usersim_free_threadpool_timers();

#pragma endregion timers

USERSIM_API
LARGE_INTEGER
KeQueryPerformanceCounter(_Out_opt_ PLARGE_INTEGER performance_frequency);

USERSIM_API
void
KeBugCheck(ULONG bug_check_code);

USERSIM_API
void
KeBugCheckEx(
    ULONG bug_check_code,
    ULONG_PTR bug_check_parameter1,
    ULONG_PTR bug_check_parameter2,
    ULONG_PTR bug_check_parameter3,
    ULONG_PTR bug_check_parameter4);

typedef
_IRQL_requires_same_
_Function_class_(EXPAND_STACK_CALLOUT)
VOID
(NTAPI EXPAND_STACK_CALLOUT) (
    _In_opt_ PVOID Parameter
    );

typedef EXPAND_STACK_CALLOUT *PEXPAND_STACK_CALLOUT;

USERSIM_API
NTSTATUS
KeExpandKernelStackAndCalloutEx (
    _In_ PEXPAND_STACK_CALLOUT Callout,
    _In_opt_ PVOID Parameter,
    _In_ SIZE_T Size,
    _In_ BOOLEAN Wait,
    _In_opt_ PVOID Context
    );

CXPLAT_EXTERN_C_END

#if defined(__cplusplus)
#include <string>

// The functions below throw C++ exceptions so tests can catch them to verify error behavior.
USERSIM_API void
KeBugCheckCPP(ULONG bug_check_code);

USERSIM_API void
KeBugCheckExCPP(
    ULONG bug_check_code,
    ULONG_PTR bug_check_parameter1,
    ULONG_PTR bug_check_parameter2,
    ULONG_PTR bug_check_parameter3,
    ULONG_PTR bug_check_parameter4);

void
usersim_throw_exception(_In_ std::string message);

#endif
