// Copyright (c) Microsoft Corporation
// SPDX-License-Identifier: MIT

#include "cxplat.h"
#include "winuser_internal.h"
#include <windows.h>

// Thread pool related globals.
static TP_CALLBACK_ENVIRON _callback_environment{};
static PTP_POOL _pool = nullptr;
static PTP_CLEANUP_GROUP _cleanup_group = nullptr;

typedef struct _cxplat_preemptible_work_item
{
    PTP_WORK work;
    void (*work_item_routine)(_Inout_opt_ void* work_item_context);
    void* work_item_context;
} cxplat_preemptible_work_item_t;

static cxplat_rundown_reference_t _cxplat_preemptible_work_items_rundown_reference;

static void
_cxplat_preemptible_routine(_Inout_ PTP_CALLBACK_INSTANCE instance, _In_opt_ void* parameter, _Inout_ PTP_WORK work)
{
    UNREFERENCED_PARAMETER(instance);
    UNREFERENCED_PARAMETER(work);

    if (parameter != nullptr) {
        cxplat_preemptible_work_item_t* work_item = (cxplat_preemptible_work_item_t*)parameter;
        work_item->work_item_routine(work_item->work_item_context);
    }
    cxplat_release_rundown_protection(&_cxplat_preemptible_work_items_rundown_reference);
}

_Must_inspect_result_ cxplat_status_t
cxplat_allocate_preemptible_work_item(
    _In_opt_ void* caller_context,
    _Outptr_ cxplat_preemptible_work_item_t** work_item,
    _In_ void (*work_item_routine)(_In_opt_ void* work_item_context),
    _In_opt_ void* work_item_context)
{
    UNREFERENCED_PARAMETER(caller_context);
    cxplat_status_t result = CXPLAT_STATUS_SUCCESS;

    *work_item = (cxplat_preemptible_work_item_t*)cxplat_allocate(sizeof(cxplat_preemptible_work_item_t));
    if (*work_item == nullptr) {
        result = CXPLAT_STATUS_NO_MEMORY;
        goto Done;
    }

    // It is required to use the InitializeThreadpoolEnvironment function to
    // initialize the _callback_environment structure before calling CreateThreadpoolWork.
    (*work_item)->work = CreateThreadpoolWork(_cxplat_preemptible_routine, *work_item, &_callback_environment);
    if ((*work_item)->work == nullptr) {
        result = CXPLAT_STATUS_FROM_WIN32(GetLastError());
        goto Done;
    }
    (*work_item)->work_item_routine = work_item_routine;
    (*work_item)->work_item_context = work_item_context;

Done:
    if (result != CXPLAT_STATUS_SUCCESS) {
        cxplat_free(*work_item);
        *work_item = nullptr;
    }
    return result;
}

void
cxplat_queue_preemptible_work_item(_Inout_ cxplat_preemptible_work_item_t* io_work_item)
{
    if (!cxplat_acquire_rundown_protection(&_cxplat_preemptible_work_items_rundown_reference)) {
        CXPLAT_RUNTIME_ASSERT(FALSE);
    }
    SubmitThreadpoolWork(io_work_item->work);
}

void
cxplat_free_preemptible_work_item(_Frees_ptr_opt_ cxplat_preemptible_work_item_t* work_item)
{
    if (!work_item) {
        return;
    }

    CloseThreadpoolWork(work_item->work);
    cxplat_free(work_item);
}

cxplat_status_t
cxplat_winuser_initialize_thread_pool()
{
    cxplat_status_t status = CXPLAT_STATUS_SUCCESS;
    bool cleanup_group_created = false;
    bool return_value;

    cxplat_initialize_rundown_protection(&_cxplat_preemptible_work_items_rundown_reference);

    // Initialize a callback environment for the thread pool.
    InitializeThreadpoolEnvironment(&_callback_environment);

    _pool = CreateThreadpool(nullptr);
    if (_pool == nullptr) {
        status = CXPLAT_STATUS_FROM_WIN32(GetLastError());
        goto Exit;
    }

    SetThreadpoolThreadMaximum(_pool, 1);
    return_value = SetThreadpoolThreadMinimum(_pool, 1);
    if (!return_value) {
        status = CXPLAT_STATUS_FROM_WIN32(GetLastError());
        goto Exit;
    }

    _cleanup_group = CreateThreadpoolCleanupGroup();
    if (_cleanup_group == nullptr) {
        status = CXPLAT_STATUS_FROM_WIN32(GetLastError());
        goto Exit;
    }
    cleanup_group_created = true;

    SetThreadpoolCallbackPool(&_callback_environment, _pool);
    SetThreadpoolCallbackCleanupGroup(&_callback_environment, _cleanup_group, nullptr);

Exit:
    if (!CXPLAT_SUCCEEDED(status)) {
        if (cleanup_group_created) {
            CloseThreadpoolCleanupGroup(_cleanup_group);
        }
        if (_pool) {
            CloseThreadpool(_pool);
            _pool = nullptr;
        }
    }
    return status;
}

void
cxplat_wait_for_preemptible_work_items_complete()
{
    cxplat_wait_for_rundown_protection_release(&_cxplat_preemptible_work_items_rundown_reference);
}

void
cxplat_winuser_clean_up_thread_pool()
{
    if (!_pool) {
        return;
    }

    if (_cleanup_group) {
        CloseThreadpoolCleanupGroupMembers(_cleanup_group, false, nullptr);
        CloseThreadpoolCleanupGroup(_cleanup_group);
        _cleanup_group = nullptr;
    }
    CloseThreadpool(_pool);
    _pool = nullptr;
}
