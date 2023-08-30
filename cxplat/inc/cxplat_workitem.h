// Copyright (c) Microsoft Corporation
// SPDX-License-Identifier: MIT
#pragma once

CXPLAT_EXTERN_C_BEGIN

typedef struct _cxplat_preemptible_work_item cxplat_preemptible_work_item_t;

typedef void(*cxplat_work_item_routine_t)(
    _In_ cxplat_preemptible_work_item_t* work_item, _In_opt_ void* work_item_context);

/**
 * @brief Create a preemptible work item.
 *
 * @param[in] caller_context Caller context, such as a DEVICE_OBJECT used to
 *  create the work item.
 * @param[out] work_item Pointer to memory that will contain the pointer to
 *  the preemptible work item on success.
 * @param[in] work_item_routine Routine to execute as a work item.
 * @param[in, out] work_item_context Context to pass to the routine.
 * @retval CXPLAT_STATUS_SUCCESS The operation was successful.
 * @retval CXPLAT_STATUS_NO_MEMORY Unable to allocate resources for this
 *  work item.
 */
_Must_inspect_result_ cxplat_status_t
cxplat_allocate_preemptible_work_item(
    _In_opt_ void* caller_context,
    _Outptr_ cxplat_preemptible_work_item_t** work_item,
    _In_ cxplat_work_item_routine_t work_item_routine,
    _In_opt_ void* work_item_context);

/**
 * @brief Queue a preemptible work item for execution.  After execution,
 *  it will automatically be freed.
 *
 * @param[in] work_item Pointer to the work item to execute and free.
 */
void
cxplat_queue_preemptible_work_item(_Inout_ cxplat_preemptible_work_item_t* work_item);

/**
 * @brief Free a preemptible work item.
 *
 * @param[in] work_item Pointer to the work item to free.
 */
void
cxplat_free_preemptible_work_item(_Frees_ptr_opt_ cxplat_preemptible_work_item_t* work_item);

/**
 * @brief Wait for all preemptible work items to complete.
 */
void
cxplat_wait_for_preemptible_work_items_complete();

CXPLAT_EXTERN_C_END
