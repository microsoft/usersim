// Copyright (c) Microsoft Corporation
// SPDX-License-Identifier: MIT

#if !defined(CMAKE_NUGET)
#include <catch2/catch_all.hpp>
#else
#include <catch2/catch.hpp>
#endif
#include "usersim/ke.h"
#include "usersim/mm.h"

#include <thread>
#include <vector>

TEST_CASE("irql", "[ke]")
{
    REQUIRE(KeGetCurrentIrql() == PASSIVE_LEVEL);

    KIRQL old_irql;
    KeRaiseIrql(DISPATCH_LEVEL, &old_irql);
    REQUIRE(old_irql == PASSIVE_LEVEL);
    REQUIRE(KeGetCurrentIrql() == DISPATCH_LEVEL);

    KeRaiseIrql(DISPATCH_LEVEL, &old_irql);
    REQUIRE(old_irql == DISPATCH_LEVEL);
    REQUIRE(KeGetCurrentIrql() == DISPATCH_LEVEL);

    KeLowerIrql(DISPATCH_LEVEL);
    REQUIRE(KeGetCurrentIrql() == DISPATCH_LEVEL);

    KeLowerIrql(PASSIVE_LEVEL);
    REQUIRE(KeGetCurrentIrql() == PASSIVE_LEVEL);

    old_irql = KeRaiseIrqlToDpcLevel();
    REQUIRE(old_irql == PASSIVE_LEVEL);
    REQUIRE(KeGetCurrentIrql() == DISPATCH_LEVEL);

    KeLowerIrql(old_irql);
    REQUIRE(KeGetCurrentIrql() == PASSIVE_LEVEL);
}

TEST_CASE("irql_perf_override", "[ke]")
{
    REQUIRE(KeGetCurrentIrql() == PASSIVE_LEVEL);

    KIRQL old_irql;
    KeRaiseIrql(DISPATCH_LEVEL, &old_irql);
    REQUIRE(old_irql == PASSIVE_LEVEL);
    REQUIRE(KeGetCurrentIrql() == DISPATCH_LEVEL);

    KeRaiseIrql(DISPATCH_LEVEL, &old_irql);
    REQUIRE(old_irql == DISPATCH_LEVEL);
    REQUIRE(KeGetCurrentIrql() == DISPATCH_LEVEL);

    KeLowerIrql(DISPATCH_LEVEL);
    REQUIRE(KeGetCurrentIrql() == DISPATCH_LEVEL);

    KeLowerIrql(PASSIVE_LEVEL);
    REQUIRE(KeGetCurrentIrql() == PASSIVE_LEVEL);

    old_irql = KeRaiseIrqlToDpcLevel();
    REQUIRE(old_irql == PASSIVE_LEVEL);
    REQUIRE(KeGetCurrentIrql() == DISPATCH_LEVEL);

    KeLowerIrql(old_irql);
    REQUIRE(KeGetCurrentIrql() == PASSIVE_LEVEL);
}

TEST_CASE("KfRaiseIrql", "[ke]")
{
    REQUIRE(KeGetCurrentIrql() == PASSIVE_LEVEL);

    KIRQL old_irql;
    old_irql = KfRaiseIrql(DISPATCH_LEVEL);
    REQUIRE(old_irql == PASSIVE_LEVEL);
    REQUIRE(KeGetCurrentIrql() == DISPATCH_LEVEL);

    old_irql = KfRaiseIrql(DISPATCH_LEVEL);
    REQUIRE(old_irql == DISPATCH_LEVEL);
    REQUIRE(KeGetCurrentIrql() == DISPATCH_LEVEL);

    KeLowerIrql(PASSIVE_LEVEL);
    REQUIRE(KeGetCurrentIrql() == PASSIVE_LEVEL);
}

TEST_CASE("spin lock", "[ke]")
{
    KSPIN_LOCK lock;
    KeInitializeSpinLock(&lock);
    REQUIRE(KeGetCurrentIrql() == PASSIVE_LEVEL);

    // Test KeAcquireSpinLock.
    KIRQL old_irql;
    KeAcquireSpinLock(&lock, &old_irql);
    REQUIRE(old_irql == PASSIVE_LEVEL);
    REQUIRE(KeGetCurrentIrql() == DISPATCH_LEVEL);

    KeReleaseSpinLock(&lock, old_irql);
    REQUIRE(KeGetCurrentIrql() == PASSIVE_LEVEL);

    // Test KeAcquireSpinLockRaiseToDpc.
    old_irql = KeAcquireSpinLockRaiseToDpc(&lock);
    REQUIRE(old_irql == PASSIVE_LEVEL);
    REQUIRE(KeGetCurrentIrql() == DISPATCH_LEVEL);

    KeReleaseSpinLock(&lock, old_irql);
    REQUIRE(KeGetCurrentIrql() == PASSIVE_LEVEL);

    // Test KeAcquireSpinLockAtDpcLevel.
    old_irql = KeRaiseIrqlToDpcLevel();
    KeAcquireSpinLockAtDpcLevel(&lock);
    KeReleaseSpinLockFromDpcLevel(&lock);
    KeLowerIrql(old_irql);
}

TEST_CASE("processor count", "[ke]")
{
    ULONG count = KeQueryMaximumProcessorCount();
    REQUIRE(count > 0);

    REQUIRE(KeQueryMaximumProcessorCountEx(ALL_PROCESSOR_GROUPS) == count);
    REQUIRE(KeQueryActiveProcessorCount() == count);
    REQUIRE(KeQueryActiveProcessorCountEx(ALL_PROCESSOR_GROUPS) == count);

    for (uint32_t i = 0; i < count; i++) {
        PROCESSOR_NUMBER processor_number;
        REQUIRE(NT_SUCCESS(KeGetProcessorNumberFromIndex(i, &processor_number)));
        REQUIRE(KeGetProcessorIndexFromNumber(&processor_number) == i);
    }
}

typedef struct _processor_change_notification_record
{
    KE_PROCESSOR_CHANGE_NOTIFY_STATE state;
    ULONG processor_index;
} processor_change_notification_record_t;

typedef struct _processor_change_callback_context
{
    std::vector<processor_change_notification_record_t> notifications;
    ULONG invocation_count = 0;
    void* handle_to_deregister = nullptr;
} processor_change_callback_context_t;

static VOID
_record_processor_change_callback(
    _In_ void* callback_context,
    _In_ PKE_PROCESSOR_CHANGE_NOTIFY_CONTEXT change_context,
    _Inout_ PNTSTATUS operation_status)
{
    UNREFERENCED_PARAMETER(operation_status);

    auto* context = reinterpret_cast<processor_change_callback_context_t*>(callback_context);
    context->notifications.push_back({change_context->State, change_context->NtNumber});
    context->invocation_count++;
}

static VOID
_deregister_processor_change_callback(
    _In_ void* callback_context,
    _In_ PKE_PROCESSOR_CHANGE_NOTIFY_CONTEXT change_context,
    _Inout_ PNTSTATUS operation_status)
{
    UNREFERENCED_PARAMETER(change_context);
    UNREFERENCED_PARAMETER(operation_status);

    auto* context = reinterpret_cast<processor_change_callback_context_t*>(callback_context);
    context->invocation_count++;
    if (context->handle_to_deregister != nullptr) {
        KeDeregisterProcessorChangeCallback(context->handle_to_deregister);
    }
}

TEST_CASE("processor change callback add existing", "[ke]")
{
    processor_change_callback_context_t existing_callback_context = {};
    processor_change_callback_context_t add_existing_callback_context = {};
    void* existing_handle =
        KeRegisterProcessorChangeCallback(_record_processor_change_callback, &existing_callback_context, 0);
    REQUIRE(existing_handle != nullptr);

    void* add_existing_handle = KeRegisterProcessorChangeCallback(
        _record_processor_change_callback, &add_existing_callback_context, KE_PROCESSOR_CHANGE_ADD_EXISTING);
    REQUIRE(add_existing_handle != nullptr);

    ULONG active_processor_count = KeQueryActiveProcessorCountEx(ALL_PROCESSOR_GROUPS);
    REQUIRE(existing_callback_context.invocation_count == 0);
    REQUIRE(add_existing_callback_context.notifications.size() == active_processor_count * 2);
    for (ULONG processor_index = 0; processor_index < active_processor_count; processor_index++) {
        const auto& start_notification = add_existing_callback_context.notifications[processor_index];
        REQUIRE(start_notification.state == KeProcessorAddStartNotify);
        REQUIRE(start_notification.processor_index == processor_index);

        const auto& complete_notification =
            add_existing_callback_context.notifications[processor_index + active_processor_count];
        REQUIRE(complete_notification.state == KeProcessorAddCompleteNotify);
        REQUIRE(complete_notification.processor_index == processor_index);
    }

    KeDeregisterProcessorChangeCallback(existing_handle);
    KeDeregisterProcessorChangeCallback(add_existing_handle);
}

TEST_CASE("processor change callback deregistration is deferred until notifications finish", "[ke]")
{
    if (KeQueryMaximumProcessorCountEx(ALL_PROCESSOR_GROUPS) < 2) {
        return;
    }

    processor_change_callback_context_t deregistering_callback_context = {};
    processor_change_callback_context_t removed_callback_context = {};

    void* deregistering_handle =
        KeRegisterProcessorChangeCallback(_deregister_processor_change_callback, &deregistering_callback_context, 0);
    REQUIRE(deregistering_handle != nullptr);

    void* removed_handle =
        KeRegisterProcessorChangeCallback(_record_processor_change_callback, &removed_callback_context, 0);
    REQUIRE(removed_handle != nullptr);
    deregistering_callback_context.handle_to_deregister = removed_handle;

    REQUIRE(usersim_set_active_processor_count(1) == STATUS_SUCCESS);
    REQUIRE(usersim_notify_processor_add_start(1) == STATUS_SUCCESS);
    REQUIRE(deregistering_callback_context.invocation_count == 1);
    REQUIRE(removed_callback_context.invocation_count == 1);

    REQUIRE(usersim_notify_processor_add_failure(1) == STATUS_SUCCESS);
    REQUIRE(deregistering_callback_context.invocation_count == 2);
    REQUIRE(removed_callback_context.invocation_count == 1);

    KeDeregisterProcessorChangeCallback(removed_handle);
    KeDeregisterProcessorChangeCallback(deregistering_handle);
    usersim_reset_active_processor_count();
}

TEST_CASE("processor add notifications are serialized", "[ke]")
{
    if (KeQueryMaximumProcessorCountEx(ALL_PROCESSOR_GROUPS) < 2) {
        return;
    }

    REQUIRE(usersim_set_active_processor_count(1) == STATUS_SUCCESS);

    REQUIRE(usersim_notify_processor_add_start(2) == STATUS_INVALID_PARAMETER);
    REQUIRE(usersim_notify_processor_add_start(1) == STATUS_SUCCESS);
    REQUIRE(usersim_notify_processor_add_start(1) == STATUS_INVALID_PARAMETER);
    REQUIRE(usersim_notify_processor_add_complete(2) == STATUS_INVALID_PARAMETER);
    REQUIRE(usersim_notify_processor_add_complete(1) == STATUS_SUCCESS);
    REQUIRE(KeQueryActiveProcessorCountEx(ALL_PROCESSOR_GROUPS) == 2);

    REQUIRE(usersim_notify_processor_add_complete(1) == STATUS_INVALID_PARAMETER);
    REQUIRE(usersim_notify_processor_add_start(2) == STATUS_SUCCESS);
    REQUIRE(usersim_notify_processor_add_failure(2) == STATUS_SUCCESS);
    REQUIRE(KeQueryActiveProcessorCountEx(ALL_PROCESSOR_GROUPS) == 2);

    usersim_reset_active_processor_count();
}

TEST_CASE("semaphore", "[ke]")
{
    KSEMAPHORE semaphore;

    // Create a semaphore that is ready to be signaled twice.
    KeInitializeSemaphore(&semaphore, 2, 2);

    // Verify that it is signaled.
    REQUIRE(KeReadStateSemaphore(&semaphore) != 0);

    // Verify that it is still signaled after calling KeReadStateSemaphore.
    REQUIRE(KeReadStateSemaphore(&semaphore) != 0);

    // Verify we can take a reference.
    LARGE_INTEGER timeout = {0};
    REQUIRE(KeWaitForSingleObject(&semaphore, Executive, KernelMode, TRUE, &timeout) == STATUS_SUCCESS);

    // Verify that it is still signaled, since the count should now be 1.
    REQUIRE(KeReadStateSemaphore(&semaphore) != 0);

    // Verify we can take a second reference.
    REQUIRE(KeWaitForSingleObject(&semaphore, Executive, KernelMode, TRUE, &timeout) == STATUS_SUCCESS);

    // The semaphore should now be not-signaled.
    REQUIRE(KeReadStateSemaphore(&semaphore) == 0);

    // Release one reference.
    KeReleaseSemaphore(&semaphore, 1, 1, FALSE);

    // It should now be signaled again.
    REQUIRE(KeReadStateSemaphore(&semaphore) != 0);

    // Release a second reference.
    KeReleaseSemaphore(&semaphore, 1, 1, FALSE);

    // It should still be signaled.
    REQUIRE(KeReadStateSemaphore(&semaphore) != 0);
}

TEST_CASE("threads", "[ke]")
{
    PKTHREAD thread = KeGetCurrentThread();
    REQUIRE(thread != nullptr);

    ULONG processor_count = KeQueryActiveProcessorCount();
    REQUIRE(processor_count > 0);

    KAFFINITY new_affinity = ((ULONG_PTR)1 << processor_count) - 1;
    KAFFINITY old_affinity = KeSetSystemAffinityThreadEx(new_affinity);
    // No old affinity was set.
    REQUIRE(old_affinity == 0);

    KeRevertToUserAffinityThreadEx(old_affinity);

    for (ULONG i = 0; i < processor_count; i++) {
        PROCESSOR_NUMBER processor_number = {};
        GROUP_AFFINITY old_group_affinity = {};
        GROUP_AFFINITY new_group_affinity = {};

        REQUIRE(KeGetProcessorNumberFromIndex(i, &processor_number) == STATUS_SUCCESS);

        new_group_affinity.Group = processor_number.Group;
        new_group_affinity.Mask = (ULONG_PTR)1 << processor_number.Number;
        KeSetSystemGroupAffinityThread(&new_group_affinity, &old_group_affinity);
        KAFFINITY new_affinity = ((ULONG_PTR)1 << processor_number.Number);

        REQUIRE(KeGetCurrentProcessorNumberEx(NULL) == i);

        KeRevertToUserGroupAffinityThread(&old_group_affinity);
    }
}

static void
_dpc_routine(
    _In_ PRKDPC dpc, _In_opt_ void* deferred_context, _In_opt_ void* system_argument1, _In_opt_ void* system_argument2)
{
    uint64_t argument1 = (uintptr_t)system_argument1;
    uint64_t argument2 = (uintptr_t)system_argument2;
    (*(uint64_t*)deferred_context) += argument1 + argument2;
}

static void
_dpc_stall_routine(
    _In_ PRKDPC dpc, _In_opt_ void* deferred_context, _In_opt_ void* system_argument1, _In_opt_ void* system_argument2)
{
    uint64_t delay_100ns = (uintptr_t)system_argument1;
    ULONG64 time = KeQueryInterruptTime();
    while (KeQueryInterruptTime() < time + delay_100ns)
        ;
}

TEST_CASE("dpcs", "[ke]")
{
    uint64_t context = 1;
    uint64_t expected_context = context;
    KDPC dpc;
    const uintptr_t delay_in_100ns = 1000 * 1000 * 10;
    KeInitializeDpc(&dpc, _dpc_routine, &context);
    REQUIRE(KeRemoveQueueDpc(&dpc) == FALSE);
    KeSetTargetProcessorDpc(&dpc, 0);

    REQUIRE(KeInsertQueueDpc(&dpc, (void*)(uintptr_t)0, (void*)(uintptr_t)1) == TRUE);

    // Force it to be removed. In parallel, the DPC may be executing, but after the
    // following call it should no longer be queued either way.
    BOOLEAN removed = KeRemoveQueueDpc(&dpc);
    if (!removed) {
        expected_context += 1;
    }

    // Verify that trying to remove it again reports that it was not queued.
    REQUIRE(KeRemoveQueueDpc(&dpc) == FALSE);

    // Wait for the DPC to complete, if it was running.
    KeFlushQueuedDpcs();
    REQUIRE(context == expected_context);

    REQUIRE(KeInsertQueueDpc(&dpc, (void*)(uintptr_t)10, (void*)(uintptr_t)20) == TRUE);

    // Try adding it again. In parallel, the DPC may be executing.
    BOOLEAN again = KeInsertQueueDpc(&dpc, (void*)(uintptr_t)100, (void*)(uintptr_t)200);

    // Wait for the DPC to finish.
    KeFlushQueuedDpcs();
    expected_context += 30;
    if (again) {
        expected_context += 300;
    }
    REQUIRE(context == expected_context);

    // Verify KeFlushQueuedDpcs() can be called a second time.
    KeFlushQueuedDpcs();

    // Set the current thread affinity to the DPC target processor
    // and raise IRQL to dispatch level. This must prevent the
    // DPC executing.
    KAFFINITY user_affinity = KeSetSystemAffinityThreadEx(1);
    KIRQL old_irql = KeRaiseIrqlToDpcLevel();
    REQUIRE(KeGetCurrentProcessorNumberEx(NULL) == 0);

    // Insert the DPC. It should sit in the queue.
    REQUIRE(KeInsertQueueDpc(&dpc, (void*)(uintptr_t)1000, (void*)(uintptr_t)2000) == TRUE);

    // Revert to the original user affinity. This does not take effect until IRQL drops below DISPATCH_LEVEL.
    KeRevertToUserAffinityThreadEx(user_affinity);

    // Busy loop for 1 second (in 100-ns interrupt time units)
    ULONG64 time = KeQueryInterruptTime();
    while (KeQueryInterruptTime() < time + delay_in_100ns)
        ;

    REQUIRE(context == expected_context);

    // Lower IRQL and wait for the DPC to execute.
    KeLowerIrql(old_irql);
    KeFlushQueuedDpcs();

    expected_context += 3000;
    REQUIRE(context == expected_context);

    // Insert a DPC that preempts this passive level thread. Ensure this
    // thread does not run until the DPC completes.
    user_affinity = KeSetSystemAffinityThreadEx(1);
    KeInitializeDpc(&dpc, _dpc_stall_routine, &context);
    KeSetTargetProcessorDpc(&dpc, 0);
    time = KeQueryInterruptTime();
    REQUIRE(KeInsertQueueDpc(&dpc, (void*)delay_in_100ns, NULL) == TRUE);
    REQUIRE(KeQueryInterruptTime() - time >= delay_in_100ns);

    KeRevertToUserAffinityThreadEx(user_affinity);
    KeFlushQueuedDpcs();
}

static void
_timer_routine(
    _In_ PRKDPC dpc, _In_opt_ void* deferred_context, _In_opt_ void* system_argument1, _In_opt_ void* system_argument2)
{
    UNREFERENCED_PARAMETER(system_argument1);
    UNREFERENCED_PARAMETER(system_argument2);
    (*(uint64_t*)deferred_context)++;
}

TEST_CASE("one-shot timers", "[ke]")
{
    uint64_t context = 1;
    KTIMER timer;
    KeInitializeTimer(&timer);
    KDPC dpc;
    KeInitializeDpc(&dpc, _timer_routine, &context);

    // Test canceling a non-running timer.
    REQUIRE(KeCancelTimer(&timer) == FALSE);

    // Verify canceling also works at dispatch level.
    KIRQL old_irql = KeRaiseIrqlToDpcLevel();
    REQUIRE(KeCancelTimer(&timer) == FALSE);
    KeLowerIrql(old_irql);

    // Test a timer expiring immediately.
    LARGE_INTEGER due_time = {.QuadPart = -1};
    REQUIRE(KeSetTimer(&timer, due_time, &dpc) == FALSE);
    Sleep(1000);                               // Wait 1 second to make sure it has time to expire.
    REQUIRE(KeReadStateTimer(&timer) == TRUE); // Verify signaled.
    REQUIRE(context == 2);

    // Test canceling an expired timer.
    REQUIRE(KeCancelTimer(&timer) == FALSE);
    REQUIRE(KeReadStateTimer(&timer) == FALSE);
    REQUIRE(context == 2);

    // Test canceling a long-running timer.
    due_time.QuadPart = -10000 * 1000 * 30ll; // 30 seconds.
    REQUIRE(KeSetTimer(&timer, due_time, &dpc) == FALSE);
    REQUIRE(KeReadStateTimer(&timer) == FALSE); // Verify not yet signaled.
    REQUIRE(KeCancelTimer(&timer) == TRUE);
    REQUIRE(KeReadStateTimer(&timer) == FALSE);
    REQUIRE(context == 2);

    // Test restarting a long-timer to make it expire immediately.
    REQUIRE(KeSetTimer(&timer, due_time, &dpc) == FALSE);
    due_time.QuadPart = -1;
    REQUIRE(KeSetTimer(&timer, due_time, &dpc) == TRUE);
    Sleep(1000); // Wait 1 second to make sure it has time to expire.
    REQUIRE(KeReadStateTimer(&timer) == TRUE);
    REQUIRE(context == 3);
}

TEST_CASE("periodic timers", "[ke]")
{
    uint64_t context = 1;
    KTIMER timer;
    KeInitializeTimer(&timer);
    KDPC dpc;
    KeInitializeDpc(&dpc, _timer_routine, &context);

    // Set timer to expire immediately and then every second thereafter.
    LARGE_INTEGER due_time = {.QuadPart = -1};
    REQUIRE(KeSetTimerEx(&timer, due_time, 1000, &dpc) == FALSE);
    Sleep(2500); // 2.5 seconds
    KeCancelTimer(&timer);
    REQUIRE(context == 4);
    Sleep(1300); // 1.3 seconds.
    REQUIRE(context == 4);
}

TEST_CASE("KeBugCheck", "[ke]")
{
    try {
        KeBugCheckCPP(17);
        REQUIRE(FALSE);
    } catch (std::exception e) {
        REQUIRE(
            strcmp(
                e.what(),
                "*** STOP 0x00000011 (0x0000000000000000,0x0000000000000000,0x0000000000000000,0x0000000000000000)") ==
            0);
    }
}

TEST_CASE("KeBugCheckEx", "[ke]")
{
    try {
        KeBugCheckExCPP(17, 1, 2, 3, 4);
        REQUIRE(FALSE);
    } catch (std::exception e) {
        REQUIRE(
            strcmp(
                e.what(),
                "*** STOP 0x00000011 (0x0000000000000001,0x0000000000000002,0x0000000000000003,0x0000000000000004)") ==
            0);
    }
}

TEST_CASE("event", "[ke]")
{
    KEVENT event;
    LARGE_INTEGER timeout = {0};

    // Create an event that is initially not signaled.
    KeInitializeEvent(&event, NotificationEvent, FALSE);

    // Verify that it is not signaled.
    REQUIRE(KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, &timeout) == STATUS_TIMEOUT);

    // Verify that it is still not signaled after calling KeWaitForSingleObject.
    REQUIRE(KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, &timeout) == STATUS_TIMEOUT);

    // Verify we can set it to signaled.
    REQUIRE(KeSetEvent(&event, 0, FALSE) == 0);

    // Verify signaling it again does not change the state.
    REQUIRE(KeSetEvent(&event, 0, FALSE) == 1);

    // Verify that it is signaled.
    REQUIRE(KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, &timeout) == STATUS_SUCCESS);

    // Verify that it is still signaled after calling KeWaitForSingleObject.
    REQUIRE(KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, &timeout) == STATUS_SUCCESS);

    // Verify we can reset it to not signaled.
    KeClearEvent(&event);

    // Verify that it is not signaled.
    REQUIRE(KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, &timeout) == STATUS_TIMEOUT);

    // Verify that it is still not signaled after calling KeWaitForSingleObject.
    REQUIRE(KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, &timeout) == STATUS_TIMEOUT);

    NTSTATUS wait_status = STATUS_SUCCESS;
    // Wait on the event, signal it, and verify that the wait completes.
    auto thread =
        std::jthread([&]() { wait_status = KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, nullptr); });

    // Delay to ensure the thread is waiting.
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Signal the event.
    KeSetEvent(&event, 0, FALSE);

    // Wait for thread to join
    thread.join();

    REQUIRE(wait_status == STATUS_SUCCESS);

    // Create a synchronization event that is initially signaled.
    KeInitializeEvent(&event, SynchronizationEvent, TRUE);

    // Verify that it is signaled.
    REQUIRE(KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, &timeout) == STATUS_SUCCESS);

    // Verify that it is not signaled after calling KeWaitForSingleObject.
    REQUIRE(KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, &timeout) == STATUS_TIMEOUT);

    // Verify we can reset it to signaled.
    REQUIRE(KeSetEvent(&event, 0, FALSE) == 0);

    // Verify signaling it again does not change the state.
    REQUIRE(KeSetEvent(&event, 0, FALSE) == 1);

    // Verify that it is signaled.
    REQUIRE(KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, &timeout) == STATUS_SUCCESS);

    // Verify that it is not signaled after calling KeWaitForSingleObject.
    REQUIRE(KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, &timeout) == STATUS_TIMEOUT);

    // Test event timeout
    timeout.QuadPart = -1000 * 10ll; // 1ms.

    // Query the current time.
    uint64_t qpc_time;
    uint64_t start_time = KeQueryUnbiasedInterruptTimePrecise(&qpc_time);

    // Wait on the event, and verify that the wait times out.
    wait_status = KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, &timeout);

    // Query the current time.
    uint64_t end_time = KeQueryUnbiasedInterruptTimePrecise(&qpc_time);

    REQUIRE(wait_status == STATUS_TIMEOUT);
    REQUIRE(end_time - start_time >= 1000);
}
