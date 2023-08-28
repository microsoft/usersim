// Copyright (c) Microsoft Corporation
// SPDX-License-Identifier: MIT

#include "platform.h"
#include "usersim/ke.h"
#include "utilities.h"

#include <format>
#include <sstream>
#include <vector>
#undef ASSERT
#define ASSERT(x) \
    if (!(x))     \
    KeBugCheckCPP(0)

#pragma comment(lib, "mincore.lib")

// Ke* functions.

#pragma region irqls

thread_local KIRQL _usersim_current_irql = PASSIVE_LEVEL;
thread_local GROUP_AFFINITY _usersim_dispatch_previous_affinity;

static uint32_t _usersim_original_priority_class;
static std::vector<std::mutex> _usersim_dispatch_locks;

static NTSTATUS
_wait_for_kevent(_Inout_ KEVENT* event, _In_opt_ PLARGE_INTEGER timeout);

usersim_result_t
usersim_initialize_irql()
{
    usersim_result_t result;

    _usersim_original_priority_class = GetPriorityClass(GetCurrentProcess());
    if (_usersim_original_priority_class == 0) {
        result = win32_error_to_usersim_error(GetLastError());
        goto Exit;
    }

    if (!SetPriorityClass(GetCurrentProcess(), REALTIME_PRIORITY_CLASS)) {
        result = win32_error_to_usersim_error(GetLastError());
        goto Exit;
    }

    _usersim_dispatch_locks = std::move(std::vector<std::mutex>(GetMaximumProcessorCount(ALL_PROCESSOR_GROUPS)));

    result = STATUS_SUCCESS;

Exit:

    return result;
}

void
usersim_clean_up_irql()
{
    if (_usersim_original_priority_class != 0) {
        if (!SetPriorityClass(GetCurrentProcess(), _usersim_original_priority_class)) {
            ASSERT(FALSE);
        }

        _usersim_original_priority_class = 0;
    }
}

KIRQL
KeGetCurrentIrql() { return _usersim_current_irql; }

VOID
KeRaiseIrql(_In_ KIRQL new_irql, _Out_ PKIRQL old_irql)
{
    *old_irql = KfRaiseIrql(new_irql);
}

_IRQL_requires_max_(HIGH_LEVEL) _IRQL_raises_(new_irql) _IRQL_saves_ KIRQL KfRaiseIrql(_In_ KIRQL new_irql)
{
    KIRQL old_irql = KeGetCurrentIrql();
    _usersim_current_irql = new_irql;
    BOOL result = SetThreadPriority(GetCurrentThread(), new_irql);
    ASSERT(result);

    if (new_irql >= DISPATCH_LEVEL && old_irql < DISPATCH_LEVEL) {
        PROCESSOR_NUMBER processor;
        uint32_t processor_index = KeGetCurrentProcessorNumberEx(&processor);

        GROUP_AFFINITY new_affinity = {0};
        new_affinity.Group = processor.Group;
        new_affinity.Mask = (ULONG_PTR)1 << processor.Number;
        result = SetThreadGroupAffinity(GetCurrentThread(), &new_affinity, nullptr);
        ASSERT(result);

        _usersim_dispatch_locks[processor_index].lock();
    }

    return old_irql;
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
    BOOL result;
    if (_usersim_current_irql >= DISPATCH_LEVEL && new_irql < DISPATCH_LEVEL) {
        uint32_t processor_index = KeGetCurrentProcessorNumberEx(nullptr);
        _usersim_dispatch_locks[processor_index].unlock();
        GROUP_AFFINITY new_affinity;
        usersim_get_current_thread_group_affinity(&new_affinity);
        result = SetThreadGroupAffinity(GetCurrentThread(), &new_affinity, nullptr);
        ASSERT(result);
    }
    _usersim_current_irql = new_irql;
    result = SetThreadPriority(GetCurrentThread(), new_irql);
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

#pragma region threads

thread_local GROUP_AFFINITY _usersim_thread_affinity;

ULONG
KeQueryMaximumProcessorCount() { return GetMaximumProcessorCount(ALL_PROCESSOR_GROUPS); }

ULONG
KeQueryMaximumProcessorCountEx(_In_ USHORT group_number) { return GetMaximumProcessorCount(group_number); }

KAFFINITY
KeSetSystemAffinityThreadEx(KAFFINITY affinity)
{
    GROUP_AFFINITY old_affinity;
    usersim_get_current_thread_group_affinity(&old_affinity);
    _usersim_thread_affinity.Group = old_affinity.Group;
    _usersim_thread_affinity.Mask = affinity;
    if (KeGetCurrentIrql() < DISPATCH_LEVEL && SetThreadAffinityMask(GetCurrentThread(), affinity) == 0) {
        unsigned long error = GetLastError();
        KeBugCheckEx(0, error, 0, 0, 0);
    }
    return (KAFFINITY)old_affinity.Mask;
}

_IRQL_requires_min_(PASSIVE_LEVEL) _IRQL_requires_max_(APC_LEVEL) NTKERNELAPI VOID
    KeRevertToUserAffinityThreadEx(_In_ KAFFINITY affinity)
{
    KeSetSystemAffinityThreadEx(affinity);
}

PKTHREAD
NTAPI
KeGetCurrentThread(VOID) { return (PKTHREAD)usersim_get_current_thread_id(); }

void
usersim_get_current_thread_group_affinity(_Out_ GROUP_AFFINITY* affinity)
{
    if (_usersim_thread_affinity.Mask != 0) {
        *affinity = _usersim_thread_affinity;
    } else {
        // The thread's current group affinity has never been explicitly set. Report the
        // current affinity is all processors in the current group.
        PROCESSOR_NUMBER processor;
        KeGetCurrentProcessorNumberEx(&processor);
        RtlZeroMemory(affinity, sizeof(*affinity));
        affinity->Group = processor.Group;
        affinity->Mask = ((ULONG_PTR)1 << GetMaximumProcessorCount(affinity->Group)) - 1;
    }
}

#pragma endregion threads

#pragma region semaphores

static std::vector<HANDLE>* g_usersim_semaphore_handles = nullptr;

_IRQL_requires_max_(DISPATCH_LEVEL) NTKERNELAPI VOID
    KeInitializeSemaphore(_Out_ PRKSEMAPHORE semaphore, _In_ LONG count, _In_ LONG limit)
{
    semaphore->handle = CreateSemaphore(nullptr, count, limit, nullptr);
    ASSERT(semaphore->handle != INVALID_HANDLE_VALUE);
    semaphore->object_type = USERSIM_OBJECT_TYPE_SEMAPHORE;

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
        delete g_usersim_semaphore_handles;
        g_usersim_semaphore_handles = nullptr;
    }
}

_When_(wait == 0, _IRQL_requires_max_(DISPATCH_LEVEL))
    _When_(wait == 1, _IRQL_requires_max_(APC_LEVEL)) NTKERNELAPI LONG KeReleaseSemaphore(
        _Inout_ PRKSEMAPHORE semaphore, _In_ KPRIORITY increment, _In_ LONG adjustment, _In_ _Literal_ BOOLEAN wait)
{
    UNREFERENCED_PARAMETER(increment);
    UNREFERENCED_PARAMETER(wait);

    ASSERT(semaphore->object_type == USERSIM_OBJECT_TYPE_SEMAPHORE);
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

    // Get handle from object.
    usersim_object_type_t type = *(usersim_object_type_t*)object;
    switch (type) {
    case USERSIM_OBJECT_TYPE_SEMAPHORE: {
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
    case USERSIM_OBJECT_TYPE_EVENT: {
        return _wait_for_kevent((KEVENT*)object, timeout);
    }
    default:
        ASSERT(FALSE);
        return STATUS_INVALID_PARAMETER;
    }
}

// Returns 0 for non-signaled, non-zero for signaled.
LONG
KeReadStateSemaphore(_In_ PRKSEMAPHORE semaphore)
{
    ASSERT(semaphore->object_type == USERSIM_OBJECT_TYPE_SEMAPHORE);
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

void
usersim_throw_exception(_In_ std::string message)
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
        "*** STOP {:#010x} ({:#018x},{:#018x},{:#018x},{:#018x})",
        bug_check_code,
        bug_check_parameter1,
        bug_check_parameter2,
        bug_check_parameter3,
        bug_check_parameter4);
    usersim_throw_exception(ss.str());
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

#pragma region dpcs

class _usersim_emulated_dpc;
static std::vector<std::shared_ptr<_usersim_emulated_dpc>> _usersim_emulated_dpcs;

/**
 * @brief This class emulates kernel mode DPCs by maintaining a per-CPU thread running at maximum priority.
 * Work items can be queued to this thread, which then executes them without being interrupted by lower
 * priority threads.
 */
class _usersim_emulated_dpc
{
  public:
    _usersim_emulated_dpc() = delete;

    /**
     * @brief Construct a new emulated dpc object for CPU i.
     *
     * @param[in] i CPU to run on.
     */
    _usersim_emulated_dpc(size_t i) : head({}), terminate(false)
    {
        usersim_list_initialize(&head);
        usersim_list_initialize(&flush_entry);
        thread = std::thread([i, this]() {
            SetThreadPriority(GetCurrentThread(), DISPATCH_LEVEL);
            KeSetSystemAffinityThreadEx((ULONG_PTR)1 << i);
            std::unique_lock<std::mutex> l(mutex);
            for (;;) {
                if (terminate) {
                    SetThreadPriority(GetCurrentThread(), PASSIVE_LEVEL);
                    return;
                }

                if (!usersim_list_is_empty(&head)) {
                    auto entry = usersim_list_remove_head_entry(&head);
                    if (entry == &flush_entry) {
                        usersim_list_initialize(&flush_entry);
                        condition_variable.notify_all();
                    } else {
                        // Snapshot the arguments under the lock, to make sure
                        // they are atomic, in case KeInsertQueueDpc() gets called
                        // in parallel with different arguments.
                        KDPC* work_item = reinterpret_cast<KDPC*>(entry);
                        void* context = work_item->context;
                        void* parameter_1 = work_item->parameter_1;
                        void* parameter_2 = work_item->parameter_2;
                        usersim_list_initialize(entry);

                        l.unlock();
                        _usersim_dispatch_locks[i].lock();
                        _usersim_current_irql = DISPATCH_LEVEL;
                        work_item->work_item_routine(work_item, context, parameter_1, parameter_2);
                        _usersim_dispatch_locks[i].unlock();
                        _usersim_current_irql = PASSIVE_LEVEL;
                        l.lock();
                    }
                }
                condition_variable.wait(l, [this]() { return terminate || !usersim_list_is_empty(&head); });
            }
        });
    }

    /**
     * @brief Destroy the emulated dpc object.
     *
     */
    ~_usersim_emulated_dpc()
    {
        SetThreadPriority(GetCurrentThread(), PASSIVE_LEVEL);
        // Set the flag to terminate the thread while holding the lock, to make
        // sure the thread is not in the middle of a wait.
        {
            std::unique_lock<std::mutex> l(mutex);
            terminate = true;
        }
        condition_variable.notify_all();
        thread.join();

        // On process termination, the system may have killed the thread before
        // it could release the mutex. If that happens, the kernel will have
        // released the mutex, but the user-mode class won't know that yet.
        // Work around this issue by trying to take the mutex here (forcing
        // user-mode to be reconciled with the actual kernel-mode state), to
        // avoid the mutex destructor asserting due to destruction without releasing.
        std::unique_lock<std::mutex> l(mutex);
    }

    /**
     * @brief Wait for all currently queued work items to complete.
     *
     */
    void
    flush_queue()
    {
        std::unique_lock<std::mutex> l(mutex);
        // Insert a marker in the queue.
        usersim_list_initialize(&flush_entry);
        usersim_list_insert_tail(&head, &flush_entry);
        condition_variable.notify_all();
        // Wait until the marker is processed.
        condition_variable.wait(l, [this]() { return terminate || usersim_list_is_empty(&flush_entry); });
    }

    /**
     * @brief Insert a work item into its associated queue.
     *
     * @param[in, out] work_item Work item to be enqueued.
     * @param[in] parameter_1 Parameter to pass to worker function.
     * @param[in] parameter_2 Parameter to pass to worker function.
     * @retval true Work item wasn't already queued.
     * @retval false Work item is already queued.
     */
    static bool
    insert(_Inout_ KDPC* work_item, _Inout_opt_ void* parameter_1, _Inout_opt_ void* parameter_2)
    {
        _usersim_emulated_dpc& dpc_queue = *_usersim_emulated_dpcs[work_item->cpu_id].get();
        std::unique_lock<std::mutex> l(dpc_queue.mutex);
        if (!usersim_list_is_empty(&work_item->entry)) {
            return false;
        } else {
            work_item->parameter_1 = parameter_1;
            work_item->parameter_2 = parameter_2;
            usersim_list_insert_tail(&dpc_queue.head, &work_item->entry);
            dpc_queue.condition_variable.notify_all();
            return true;
        }
    }

    /**
     * @brief Remove a work item from its associated queue, if any.
     *
     * @param[in, out] work_item Work item to be dequeued.
     * @retval false Work item wasn't queued.
     * @retval true Work item was queued.
     */
    static bool
    remove(_Inout_ KDPC* work_item)
    {
        _usersim_emulated_dpc& dpc_queue = *_usersim_emulated_dpcs[work_item->cpu_id].get();
        std::unique_lock<std::mutex> l(dpc_queue.mutex);
        if (usersim_list_is_empty(&work_item->entry)) {
            return false;
        }
        usersim_list_remove_entry(&work_item->entry);
        return true;
    }

  private:
    usersim_list_entry_t flush_entry;
    usersim_list_entry_t head;
    std::thread thread;
    std::mutex mutex;
    std::condition_variable condition_variable;
    bool terminate;
};

void
KeInitializeDpc(
    _Out_ __drv_aliasesMem PRKDPC dpc,
    _In_ PKDEFERRED_ROUTINE deferred_routine,
    _In_opt_ __drv_aliasesMem PVOID deferred_context)
{
    usersim_list_initialize(&dpc->entry);
    dpc->cpu_id = 0;
    dpc->work_item_routine = deferred_routine;
    dpc->context = deferred_context;
}

void
KeSetTargetProcessorDpc(_Inout_ PRKDPC dpc, CCHAR number)
{
    dpc->cpu_id = number;
}

BOOLEAN
KeInsertQueueDpc(_Inout_ PRKDPC dpc, _In_opt_ PVOID system_argument1, _In_opt_ __drv_aliasesMem PVOID system_argument2)
{
    return _usersim_emulated_dpc::insert(dpc, system_argument1, system_argument2);
}

BOOLEAN
KeRemoveQueueDpc(_Inout_ PRKDPC dpc) { return _usersim_emulated_dpc::remove(dpc); }

void
KeFlushQueuedDpcs()
{
    for (auto& dpc : _usersim_emulated_dpcs) {
        dpc->flush_queue();
    }
}

void
usersim_initialize_dpcs()
{
    for (size_t i = 0; i < usersim_get_cpu_count(); i++) {
        _usersim_emulated_dpcs.push_back(std::make_shared<_usersim_emulated_dpc>(i));
    }
}

void
usersim_clean_up_dpcs()
{
    _usersim_emulated_dpcs.resize(0);
}

#pragma endregion dpcs
#pragma region timers

// The following mutex currently protects two things that can contain a pointer to a TP_TIMER:
// 1) the g_usersim_threadpool_timers list, and
// 2) each KTIMER's threadpool_timer member.
static std::mutex g_usersim_threadpool_mutex;

// The following is a list of all TP_TIMER objects that are in use.
static std::vector<TP_TIMER*>* g_usersim_threadpool_timers = nullptr;

void
KeInitializeTimer(_Out_ PKTIMER timer)
{
    memset(timer, 0, sizeof(*timer));
    timer->object_type = USERSIM_OBJECT_TYPE_TIMER;
}

VOID CALLBACK
_usersim_timer_callback(
    _Inout_ PTP_CALLBACK_INSTANCE instance, _Inout_opt_ PVOID context, _Inout_ PTP_TIMER threadpool_timer)
{
    UNREFERENCED_PARAMETER(instance);
    UNREFERENCED_PARAMETER(threadpool_timer);

    PKTIMER timer = (PKTIMER)context;
    if (timer == nullptr) {
        return;
    }
    ASSERT(timer->object_type == USERSIM_OBJECT_TYPE_TIMER);
    timer->signaled = TRUE;

    if (timer->dpc) {
        KIRQL old_irql = KeRaiseIrqlToDpcLevel();
        timer->dpc->work_item_routine(
            timer->dpc, timer->dpc->context, timer->dpc->parameter_1, timer->dpc->parameter_2);
        KeLowerIrql(old_irql);
    }
}

BOOLEAN
KeSetTimer(_Inout_ PKTIMER timer, LARGE_INTEGER due_time, _In_opt_ PKDPC dpc)
{
    return KeSetCoalescableTimer(timer, due_time, 0, 0, dpc);
}

BOOLEAN
KeSetTimerEx(_Inout_ PKTIMER timer, LARGE_INTEGER due_time, ULONG period, _In_opt_ PKDPC dpc)
{
    return KeSetCoalescableTimer(timer, due_time, period, 0, dpc);
}

BOOLEAN
KeSetCoalescableTimer(
    _Inout_ PKTIMER timer, LARGE_INTEGER due_time, ULONG period, ULONG tolerable_delay, _In_opt_ PKDPC dpc)
{
    ASSERT(dpc != nullptr);
    ASSERT(timer->object_type == USERSIM_OBJECT_TYPE_TIMER);

    // We currently only support relative expiration times.
    ASSERT(due_time.QuadPart < 0);

    std::unique_lock<std::mutex> l(g_usersim_threadpool_mutex);
    BOOLEAN running = (timer->threadpool_timer != nullptr);
    if (!running) {
        timer->threadpool_timer = CreateThreadpoolTimer(_usersim_timer_callback, timer, nullptr);
        if (timer->threadpool_timer == nullptr) {
            KeBugCheck(0);
            return FALSE; // Keep code analysis happy.
        }

        // There is no kernel function to clean up a timer, but there is in user mode,
        // so add the handle to a list we can clean up later.
        if (g_usersim_threadpool_timers == nullptr) {
            g_usersim_threadpool_timers = new std::vector<TP_TIMER*>();
        }
        g_usersim_threadpool_timers->push_back(timer->threadpool_timer);
    }

    timer->signaled = FALSE;
    timer->dpc = dpc;
    SetThreadpoolTimer(timer->threadpool_timer, (FILETIME*)&due_time, period, tolerable_delay);

    return running;
}

void
usersim_free_threadpool_timers()
{
    std::unique_lock<std::mutex> l(g_usersim_threadpool_mutex);
    if (g_usersim_threadpool_timers) {
        for (TP_TIMER* threadpool_timer : *g_usersim_threadpool_timers) {
            CloseThreadpoolTimer(threadpool_timer);
        }
        delete g_usersim_threadpool_timers;
        g_usersim_threadpool_timers = nullptr;
    }
}

BOOLEAN
KeCancelTimer(_Inout_ PKTIMER timer)
{
    ASSERT(timer->object_type == USERSIM_OBJECT_TYPE_TIMER);

    std::unique_lock<std::mutex> l(g_usersim_threadpool_mutex);
    if (timer->threadpool_timer == nullptr) {
        return FALSE;
    }

    // Stop generating new callbacks.
    SetThreadpoolTimer(timer->threadpool_timer, nullptr, 0, 0);

    // Cancel any queued callbacks that have not yet started to execute.
    WaitForThreadpoolTimerCallbacks(timer->threadpool_timer, TRUE);

    // Clean up timer.
    auto iterator =
        std::find(g_usersim_threadpool_timers->begin(), g_usersim_threadpool_timers->end(), timer->threadpool_timer);
    ASSERT(iterator != g_usersim_threadpool_timers->end());
    g_usersim_threadpool_timers->erase(iterator);
    CloseThreadpoolTimer(timer->threadpool_timer);
    timer->threadpool_timer = nullptr;

    // Return TRUE if the timer was running.
    BOOLEAN was_running = !timer->signaled;
    timer->signaled = FALSE;
    return was_running;
}

// Check whether the current state is signaled.
BOOLEAN
KeReadStateTimer(_In_ PKTIMER timer)
{
    ASSERT(timer->object_type == USERSIM_OBJECT_TYPE_TIMER);
    return timer->signaled;
}

#pragma endregion timers

#pragma region events
USERSIM_API
void
KeInitializeEvent(_Out_ PKEVENT event, _In_ EVENT_TYPE type, _In_ BOOLEAN initial_state)
{
    event->signaled = initial_state;
    event->type = type;
    event->object_type = USERSIM_OBJECT_TYPE_EVENT;
    KeInitializeSpinLock(&event->spin_lock);
}

USERSIM_API
LONG
KeSetEvent(_Inout_ PKEVENT event, _In_ KPRIORITY increment, _In_ _Literal_ BOOLEAN wait)
{
    UNREFERENCED_PARAMETER(increment);
    UNREFERENCED_PARAMETER(wait);

    ASSERT(event->object_type == USERSIM_OBJECT_TYPE_EVENT);
    KIRQL old_irql;
    LONG previous_state;
    KeAcquireSpinLock(&event->spin_lock, &old_irql);

    previous_state = event->signaled ? 1 : 0;
    event->signaled = TRUE;

    KeReleaseSpinLock(&event->spin_lock, old_irql);

    // Wake up any waiters.
    WakeByAddressAll(&event->signaled);
    return previous_state;
}

USERSIM_API
void
KeClearEvent(_Inout_ PKEVENT event)
{
    ASSERT(event->object_type == USERSIM_OBJECT_TYPE_EVENT);
    KIRQL old_irql;
    KeAcquireSpinLock(&event->spin_lock, &old_irql);

    event->signaled = FALSE;

    KeReleaseSpinLock(&event->spin_lock, old_irql);
}

/**
 * @brief Wait for an event to be signaled.
 *
 * @param[in,out] event The KEVENT to wait on.
 * @param[in,opt] timeout The timeout for the wait, or nullptr for no timeout.
 * @retval STATUS_SUCCESS The event was signaled.
 * @retval STATUS_TIMEOUT The wait timed out.
 */
static NTSTATUS
_wait_for_kevent(_Inout_ KEVENT* event, _In_opt_ PLARGE_INTEGER timeout)
{
    uint64_t qpc_time_stamp = 0;
    // Compute the end time for the wait, if any.
    uint64_t start_time = KeQueryInterruptTimePrecise(&qpc_time_stamp);
    uint64_t end_time;
    if (timeout == nullptr) {
        end_time = UINT64_MAX;
    } else if (timeout->QuadPart > 0) {
        end_time = timeout->QuadPart;
    } else {
        end_time = start_time - timeout->QuadPart;
    }

    for (;;) {
        // Compute the remaining time in milliseconds for the wait.
        DWORD timeout_ms = (DWORD)((end_time - start_time) / 10000);

        KIRQL old_irql;
        KeAcquireSpinLock(&event->spin_lock, &old_irql);
        // Check if the event is signaled.
        if (event->signaled) {
            // Clear the event if it is an auto-reset event.
            if (event->type == SynchronizationEvent) {
                event->signaled = FALSE;
            }
            KeReleaseSpinLock(&event->spin_lock, old_irql);
            return STATUS_SUCCESS;
        } else {
            // Capture the current state of event->signaled, so we can wait for it to change.
            uint64_t old_state = event->signaled;
            KeReleaseSpinLock(&event->spin_lock, old_irql);

            // Wait for event->signaled to change.
            bool wait_return = WaitOnAddress(&event->signaled, &old_state, sizeof(event->signaled), timeout_ms);
            if (!wait_return) {
                return STATUS_TIMEOUT;
            }
        }
        // Check if the wait timed out outside of the WaitOnAddress() call.
        start_time = KeQueryInterruptTimePrecise(&qpc_time_stamp);
        if (start_time >= end_time) {
            return STATUS_TIMEOUT;
        }
    }
}

#pragma endregion events