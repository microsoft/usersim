// Copyright (c) Microsoft Corporation
// SPDX-License-Identifier: MIT

#pragma once
#include <sal.h>
#include "ke.h"
#include "mm.h"

#if defined(__cplusplus)
extern "C"
{
#endif

    typedef _Enum_is_bitflag_ enum _WORK_QUEUE_TYPE {
        CriticalWorkQueue,
        DelayedWorkQueue,
        HyperCriticalWorkQueue,
        NormalWorkQueue,
        BackgroundWorkQueue,
        RealTimeWorkQueue,
        SuperCriticalWorkQueue,
        MaximumWorkQueue,
        CustomPriorityWorkQueue = 32
    } WORK_QUEUE_TYPE;

    typedef struct _DEVICE_OBJECT DEVICE_OBJECT;

    typedef struct _DRIVER_OBJECT DRIVER_OBJECT;

    typedef struct _IO_WORKITEM IO_WORKITEM, *PIO_WORKITEM;

    typedef void
    IO_WORKITEM_ROUTINE(_In_ DEVICE_OBJECT* device_object, _In_opt_ void* context);

    typedef struct _IRP IRP;

    MDL*
    IoAllocateMdl(
        _In_opt_ __drv_aliasesMem void* virtual_address,
        _In_ unsigned long length,
        _In_ BOOLEAN secondary_buffer,
        _In_ BOOLEAN charge_quota,
        _Inout_opt_ IRP* irp);

    PIO_WORKITEM
    IoAllocateWorkItem(_In_ DEVICE_OBJECT* device_object);

    void
    IoQueueWorkItem(
        _Inout_ __drv_aliasesMem IO_WORKITEM* io_work_item,
        _In_ IO_WORKITEM_ROUTINE* worker_routine,
        _In_ WORK_QUEUE_TYPE queue_type,
        _In_opt_ __drv_aliasesMem void* context);

    void
    IoFreeWorkItem(_In_ __drv_freesMem(Mem) PIO_WORKITEM io_work_item);

    void
    IoFreeMdl(MDL* mdl);

    PGENERIC_MAPPING
    IoGetFileObjectGenericMapping();

    _IRQL_requires_max_(DISPATCH_LEVEL) NTKERNELAPI PEPROCESS IoGetCurrentProcess(VOID);

#if defined(__cplusplus)
}
#endif
