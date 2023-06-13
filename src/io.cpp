// Copyright (c) Microsoft Corporation
// SPDX-License-Identifier: MIT

#include "platform.h"
#include "kernel_um.h"
#include "io.h"

// Io* functions.

typedef unsigned long PFN_NUMBER;
#define PAGE_SIZE 4096
#define PAGE_SHIFT 12L

#define PAGE_ALIGN(Va) ((void*)((ULONG_PTR)(Va) & ~(PAGE_SIZE - 1)))
#define BYTE_OFFSET(Va) ((unsigned long)((LONG_PTR)(Va) & (PAGE_SIZE - 1)))
#define ADDRESS_AND_SIZE_TO_SPAN_PAGES(Va, size)                                                                \
    (((((size)-1) >> PAGE_SHIFT) +                                                                              \
      (((((unsigned long)(size - 1) & (PAGE_SIZE - 1)) + (PtrToUlong(Va) & (PAGE_SIZE - 1)))) >> PAGE_SHIFT)) + \
     1L)

typedef struct _IO_WORKITEM
{
    DEVICE_OBJECT* device;
    PTP_WORK work_item;
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

    mdl = reinterpret_cast<MDL*>(usersim_allocate(sizeof(MDL)));
    if (mdl == NULL) {
        return mdl;
    }
#pragma warning(push)
#pragma warning(disable : 26451)
    MmInitializeMdl(mdl, virtual_address, length);
#pragma warning(pop)

    return mdl;
}

void NTAPI
io_work_item_wrapper(_Inout_ PTP_CALLBACK_INSTANCE instance, _Inout_opt_ void* context, _Inout_ PTP_WORK work)
{
    UNREFERENCED_PARAMETER(instance);
    UNREFERENCED_PARAMETER(work);
    auto work_item = reinterpret_cast<const IO_WORKITEM*>(context);
    if (work_item) {
        work_item->routine(work_item->device, work_item->context);
    }
}

PIO_WORKITEM
IoAllocateWorkItem(_In_ DEVICE_OBJECT* device_object)
{
    // Skip Fault Injection as it is already added in usersim_allocate.
    auto work_item = reinterpret_cast<IO_WORKITEM*>(usersim_allocate(sizeof(IO_WORKITEM)));
    if (!work_item) {
        return nullptr;
    }
    work_item->device = device_object;
    work_item->work_item = CreateThreadpoolWork(io_work_item_wrapper, work_item, nullptr);
    if (work_item->work_item == nullptr) {
        usersim_free(work_item);
        work_item = nullptr;
    }
    return work_item;
}

void
IoQueueWorkItem(
    _Inout_ __drv_aliasesMem IO_WORKITEM* io_workitem,
    _In_ IO_WORKITEM_ROUTINE* worker_routine,
    _In_ WORK_QUEUE_TYPE queue_type,
    _In_opt_ __drv_aliasesMem void* context)
{
    UNREFERENCED_PARAMETER(queue_type);
    io_workitem->routine = worker_routine;
    io_workitem->context = context;
    SubmitThreadpoolWork(io_workitem->work_item);
}

void
IoFreeWorkItem(_In_ __drv_freesMem(Mem) PIO_WORKITEM io_workitem)
{
    if (io_workitem) {
        CloseThreadpoolWork(io_workitem->work_item);
        usersim_free(io_workitem);
    }
}

void
IoFreeMdl(MDL* mdl)
{
    usersim_free(mdl);
}
