// Copyright (c) Microsoft Corporation
// SPDX-License-Identifier: MIT

#include "platform.h"
#include "usersim/ex.h"
#include "usersim/io.h"

// Io* functions.

typedef unsigned long PFN_NUMBER;
#define PAGE_SHIFT 12L

#define PAGE_ALIGN(Va) ((void*)((ULONG_PTR)(Va) & ~(PAGE_SIZE - 1)))
#define BYTE_OFFSET(Va) ((unsigned long)((LONG_PTR)(Va) & (PAGE_SIZE - 1)))
#define ADDRESS_AND_SIZE_TO_SPAN_PAGES(Va, size)                                                                \
    (((((size)-1) >> PAGE_SHIFT) +                                                                              \
      (((((unsigned long)(size - 1) & (PAGE_SIZE - 1)) + (PtrToUlong(Va) & (PAGE_SIZE - 1)))) >> PAGE_SHIFT)) + \
     1L)

typedef struct _IO_WORKITEM
{
    DEVICE_OBJECT* device_object;
    cxplat_preemptible_work_item_t* cxplat_work_item;
    IO_WORKITEM_ROUTINE* routine;
    void* context;
} IO_WORKITEM;

static GENERIC_MAPPING _mapping = {1, 1, 1};

PGENERIC_MAPPING
IoGetFileObjectGenericMapping() { return &_mapping; }

MDL*
IoAllocateMdl(
    _In_opt_ __drv_aliasesMem void* virtual_address,
    _In_ unsigned long length,
    _In_ BOOLEAN secondary_buffer,
    _In_ BOOLEAN charge_quota,
    _Inout_opt_ IRP* irp)
{
    // Skip Fault Injection as it is already added in usersim_allocate.
    PMDL mdl;

    UNREFERENCED_PARAMETER(secondary_buffer);
    UNREFERENCED_PARAMETER(charge_quota);
    UNREFERENCED_PARAMETER(irp);

    mdl = reinterpret_cast<MDL*>(cxplat_allocate(CxPlatNonPagedPoolNx, sizeof(MDL), USERSIM_TAG_MDL, true));
    if (mdl == NULL) {
        return mdl;
    }
#pragma warning(push)
#pragma warning(disable : 26451)
    MmInitializeMdl(mdl, virtual_address, length);
#pragma warning(pop)

    return mdl;
}

void
io_work_item_wrapper(_In_ cxplat_preemptible_work_item_t* work_item, _Inout_opt_ void* context)
{
    UNREFERENCED_PARAMETER(work_item);
    auto io_work_item = reinterpret_cast<const IO_WORKITEM*>(context);
    if (io_work_item) {
        io_work_item->routine(io_work_item->device_object, io_work_item->context);
    }
}

PIO_WORKITEM
IoAllocateWorkItem(_In_ DEVICE_OBJECT* device_object)
{
    auto io_work_item =
        (PIO_WORKITEM)cxplat_allocate(CxPlatNonPagedPoolNx, sizeof(IO_WORKITEM), USERSIM_TAG_IO_WORK_ITEM, true);
    if (!io_work_item) {
        return nullptr;
    }
    io_work_item->device_object = device_object;
    cxplat_status_t status = cxplat_allocate_preemptible_work_item(
        nullptr, &io_work_item->cxplat_work_item, io_work_item_wrapper, io_work_item);
    if (!CXPLAT_SUCCEEDED(status)) {
        cxplat_free(io_work_item, USERSIM_TAG_IO_WORK_ITEM);
        return nullptr;
    }
    return io_work_item;
}

void
IoQueueWorkItem(
    _Inout_ __drv_aliasesMem IO_WORKITEM* io_work_item,
    _In_ IO_WORKITEM_ROUTINE* worker_routine,
    _In_ WORK_QUEUE_TYPE queue_type,
    _In_opt_ __drv_aliasesMem void* context)
{
    UNREFERENCED_PARAMETER(queue_type);
    io_work_item->routine = worker_routine;
    io_work_item->context = context;
    cxplat_queue_preemptible_work_item(io_work_item->cxplat_work_item);
}

void
IoFreeWorkItem(_In_ __drv_freesMem(Mem) PIO_WORKITEM io_work_item)
{
    cxplat_free_preemptible_work_item(io_work_item->cxplat_work_item);
    cxplat_free(io_work_item, USERSIM_TAG_IO_WORK_ITEM);
}

void
IoFreeMdl(MDL* mdl)
{
    cxplat_free(mdl, USERSIM_TAG_MDL);
}

_IRQL_requires_max_(DISPATCH_LEVEL) NTKERNELAPI PEPROCESS IoGetCurrentProcess(VOID)
{
    return (PEPROCESS)GetCurrentProcess();
}

_IRQL_requires_max_(DISPATCH_LEVEL) VOID IofCompleteRequest(_In_ PIRP irp, _In_ CCHAR priority_boost)
{
    UNREFERENCED_PARAMETER(irp);
    UNREFERENCED_PARAMETER(priority_boost);
}