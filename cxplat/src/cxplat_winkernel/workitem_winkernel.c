// Copyright (c) Microsoft Corporation
// SPDX-License-Identifier: MIT

#include "../tags.h"
#include "cxplat.h"

#include <wdm.h>

typedef struct _cxplat_preemptible_work_item
{
    PIO_WORKITEM io_work_item;
    cxplat_work_item_routine_t work_item_routine;
    void* work_item_context;
} cxplat_preemptible_work_item_t;

IO_WORKITEM_ROUTINE _cxplat_preemptible_routine;

void
_cxplat_preemptible_routine(_In_ PDEVICE_OBJECT device_object, _In_opt_ void* context)
{
    UNREFERENCED_PARAMETER(device_object);
    if (context == NULL) {
        return;
    }
    cxplat_preemptible_work_item_t* work_item = (cxplat_preemptible_work_item_t*)context;
    work_item->work_item_routine(work_item, work_item->work_item_context);
}

_Must_inspect_result_ cxplat_status_t
cxplat_allocate_preemptible_work_item(
    _In_opt_ void* caller_context,
    _Outptr_ cxplat_preemptible_work_item_t** work_item,
    _In_ cxplat_work_item_routine_t work_item_routine,
    _In_opt_ void* work_item_context)
{
    cxplat_status_t result = CXPLAT_STATUS_SUCCESS;

    *work_item = cxplat_allocate(
        CXPLAT_POOL_FLAG_NON_PAGED, sizeof(cxplat_preemptible_work_item_t), CXPLAT_TAG_PREEMPTIBLE_WORK_ITEM);
    if (*work_item == NULL) {
        result = CXPLAT_STATUS_NO_MEMORY;
        goto Done;
    }

    // We don't need to call ExAcquireRundownProtection() because
    // IoAllocateWorkItem takes care of rundown protection internally,
    // unlike ExInitializeWorkItem.

    (*work_item)->io_work_item = IoAllocateWorkItem(caller_context);
    if ((*work_item)->io_work_item == NULL) {
        result = CXPLAT_STATUS_NO_MEMORY;
        goto Done;
    }
    (*work_item)->work_item_routine = work_item_routine;
    (*work_item)->work_item_context = work_item_context;

Done:
    if (result != CXPLAT_STATUS_SUCCESS) {
        cxplat_free(*work_item, CXPLAT_POOL_FLAG_NON_PAGED, CXPLAT_TAG_PREEMPTIBLE_WORK_ITEM);
        *work_item = NULL;
    }
    return result;
}

void
cxplat_queue_preemptible_work_item(_Inout_ cxplat_preemptible_work_item_t* work_item)
{
    IoQueueWorkItem(work_item->io_work_item, _cxplat_preemptible_routine, DelayedWorkQueue, work_item);
}

void
cxplat_free_preemptible_work_item(_Frees_ptr_opt_ cxplat_preemptible_work_item_t* work_item)
{
    if (work_item) {
        IoFreeWorkItem(work_item->io_work_item);
        cxplat_free(work_item, CXPLAT_POOL_FLAG_NON_PAGED, CXPLAT_TAG_PREEMPTIBLE_WORK_ITEM);
    }
}

void
cxplat_wait_for_preemptible_work_items_complete()
{
    // We used IoAllocateWorkItem() above instead of doing rundown
    // protection ourselves, so we don't need to do anything here.
}
