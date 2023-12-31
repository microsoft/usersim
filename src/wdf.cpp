// Copyright (c) Microsoft Corporation
// SPDX-License-Identifier: MIT

#include "cxplat_fault_injection.h"
#include "framework.h"
#include "platform.h"
#include "usersim/ex.h"
#include "usersim/wdf.h"

WDF_DRIVER_GLOBALS g_UsersimWdfDriverGlobals = {0};

static WdfDriverCreate_t _WdfDriverCreate;
static WdfDeviceCreate_t _WdfDeviceCreate;
static WdfControlDeviceInitAllocate_t _WdfControlDeviceInitAllocate;
static WdfControlFinishInitializing_t _WdfControlFinishInitializing;
static WdfDeviceInitFree_t _WdfDeviceInitFree;
static WdfDeviceInitSetDeviceType_t _WdfDeviceInitSetDeviceType;
static WdfDeviceInitSetCharacteristics_t _WdfDeviceInitSetCharacteristics;
static WdfDeviceInitAssignName_t _WdfDeviceInitAssignName;
static WdfDeviceInitSetFileObjectConfig_t _WdfDeviceInitSetFileObjectConfig;
static WdfDeviceInitAssignWdmIrpPreprocessCallback_t _WdfDeviceInitAssignWdmIrpPreprocessCallback;
static WdfDeviceCreateSymbolicLink_t _WdfDeviceCreateSymbolicLink;
static WdfIoQueueCreate_t _WdfIoQueueCreate;
static WdfIoQueueGetDevice_t _WdfIoQueueGetDevice;
static WdfDeviceWdmGetDeviceObject_t _WdfDeviceWdmGetDeviceObject;
static WdfObjectDelete_t _WdfObjectDelete;
static WdfRequestCompleteWithInformation_t _WdfRequestCompleteWithInformation;

static NTSTATUS
_WdfDriverCreate(
    _In_ WDF_DRIVER_GLOBALS* driver_globals,
    _In_ PDRIVER_OBJECT driver_object,
    _In_ PCUNICODE_STRING registry_path,
    _In_opt_ PWDF_OBJECT_ATTRIBUTES driver_attributes,
    _In_ PWDF_DRIVER_CONFIG driver_config,
    _Out_opt_ WDFDRIVER* driver)
{
    UNREFERENCED_PARAMETER(registry_path);
    UNREFERENCED_PARAMETER(driver_attributes);

    if (driver_globals != &g_UsersimWdfDriverGlobals) {
        return STATUS_INVALID_PARAMETER;
    }
    if (driver_globals->Driver != nullptr) {
        return STATUS_DRIVER_INTERNAL_ERROR;
    }
    g_UsersimWdfDriverGlobals.Driver = driver_object;
    driver_object->config = *driver_config;
    if (driver != nullptr) {
        *driver = driver_object;
    }
    if (driver_object->config.EvtDriverDeviceAdd) {
        // The driver has registered for the device addition event,
        // so go ahead and signal it immediately.
        PWDFDEVICE_INIT device_init = _WdfControlDeviceInitAllocate(driver_globals, driver_globals->Driver, nullptr);
        if (!device_init) {
            return STATUS_NO_MEMORY;
        }
        size_t original_device_count = driver_object->devices.size();
        NTSTATUS status = driver_object->config.EvtDriverDeviceAdd(driver_globals->Driver, device_init);
        if (!NT_SUCCESS(status)) {
            // https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/wdfdriver/nc-wdfdriver-evt_wdf_driver_device_add
            // explains: "If a driver's EvtDriverDeviceAdd callback function creates a device object
            // but does not return STATUS_SUCCESS, the framework deletes the device object and its
            // child devices.
            if (driver_object->devices.size() > original_device_count) {
                while (driver_object->devices.size() > original_device_count) {
                    auto device_object = driver_object->devices.back();
                    _WdfObjectDelete(driver_globals, device_object);
                }
            } else {
                // We never got far enough to create a device, so free the initialization object.
                _WdfDeviceInitFree(driver_globals, device_init);
            }
        }
        return status;
    }
    return STATUS_SUCCESS;
}

#define IRP_MJ_MAXIMUM_FUNCTION 0x1b

typedef struct _WDFDEVICE_INIT
{
    DEVICE_TYPE device_type;
    ULONG device_characteristics;
    UNICODE_STRING device_name;
    PWDF_FILEOBJECT_CONFIG file_object_config;
    PWDF_OBJECT_ATTRIBUTES file_object_attributes;
    PFN_WDFDEVICE_WDM_IRP_PREPROCESS evt_device_wdm_irp_preprocess;
    PUCHAR minor_functions[IRP_MJ_MAXIMUM_FUNCTION];
    ULONG num_minor_functions[IRP_MJ_MAXIMUM_FUNCTION];
    WDF_IO_QUEUE_CONFIG io_queue_config;
} WDFDEVICE_INIT;

struct _DEVICE_OBJECT
{
    WDFDEVICE_INIT init;
};

static NTSTATUS
_WdfDeviceCreate(
    _In_ WDF_DRIVER_GLOBALS* driver_globals,
    _Inout_ PWDFDEVICE_INIT* device_init,
    _In_opt_ PWDF_OBJECT_ATTRIBUTES device_attributes,
    _Out_ WDFDEVICE* device)
{
    UNREFERENCED_PARAMETER(device_attributes);

    if (driver_globals != &g_UsersimWdfDriverGlobals) {
        return STATUS_INVALID_PARAMETER;
    }

    DEVICE_OBJECT* device_object = (DEVICE_OBJECT*)cxplat_allocate(
        CXPLAT_POOL_FLAG_NON_PAGED, sizeof(*device_object), USERSIM_TAG_WDF_DEVICE_OBJECT);
    if (!device_object) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    DRIVER_OBJECT* driver_object = (DRIVER_OBJECT*)driver_globals->Driver;
    driver_object->devices.push_back(device_object);

    *device = device_object;

    // https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/wdfdevice/nf-wdfdevice-wdfdevicecreate
    // explains: "After the driver calls WdfDeviceCreate, it can no longer access the WDFDEVICE_INIT structure."
    // https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/wdfdevice/nf-wdfdevice-wdfdeviceinitfree
    // explains: "Your driver must not call WdfDeviceInitFree after it calls WdfDeviceCreate successfully."
    // https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/wdfdevice/nf-wdfdevice-wdfdeviceinitfree
    // also explains: "If your driver receives a WDFDEVICE_INIT structure from a call to WdfPdoInitAllocate or
    // WdfControlDeviceInitAllocate, and if the driver subsequently encounters an error when it calls a
    // device object initialization method or WdfDeviceCreate, the driver must call WdfDeviceInitFree."
    //
    // Thus, on success, WdfDeviceCreate causes WdfDeviceInitFree to be called by the framework
    // (whether immediately, or when the device is eventually deleted).
    // If we return an error, the caller has to call WdfDeviceInitFree() itself.
    device_object->init = **device_init;
    _WdfDeviceInitFree(driver_globals, *device_init);

    // https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/wdfdevice/nf-wdfdevice-wdfdevicecreate
    // explains: "If WdfDeviceCreate encounters no errors, it sets the pointer to NULL."
    *device_init = nullptr;

    return STATUS_SUCCESS;
}

static _Must_inspect_result_ _IRQL_requires_max_(PASSIVE_LEVEL) PWDFDEVICE_INIT _WdfControlDeviceInitAllocate(
    _In_ PWDF_DRIVER_GLOBALS driver_globals, _In_ WDFDRIVER driver, _In_ CONST UNICODE_STRING* sddl_string)
{
    UNREFERENCED_PARAMETER(driver_globals);
    UNREFERENCED_PARAMETER(driver);
    UNREFERENCED_PARAMETER(sddl_string);

    PWDFDEVICE_INIT device_init = (PWDFDEVICE_INIT)cxplat_allocate(
        CXPLAT_POOL_FLAG_NON_PAGED, sizeof(*device_init), USERSIM_TAG_WDF_DEVICE_INIT);
    return device_init;
}

static _IRQL_requires_max_(DISPATCH_LEVEL) VOID
    _WdfDeviceInitFree(_In_ PWDF_DRIVER_GLOBALS driver_globals, _In_ PWDFDEVICE_INIT device_init)
{
    UNREFERENCED_PARAMETER(driver_globals);
    cxplat_free(device_init, CXPLAT_POOL_FLAG_NON_PAGED, USERSIM_TAG_WDF_DEVICE_INIT);
}

static _IRQL_requires_max_(DISPATCH_LEVEL) VOID _WdfDeviceInitSetDeviceType(
    _In_ PWDF_DRIVER_GLOBALS driver_globals, _In_ PWDFDEVICE_INIT device_init, DEVICE_TYPE device_type)
{
    UNREFERENCED_PARAMETER(driver_globals);
    device_init->device_type = device_type;
}

static _IRQL_requires_max_(DISPATCH_LEVEL) VOID _WdfDeviceInitSetCharacteristics(
    _In_ PWDF_DRIVER_GLOBALS driver_globals,
    _In_ PWDFDEVICE_INIT device_init,
    ULONG device_characteristics,
    BOOLEAN or_in_values)
{
    UNREFERENCED_PARAMETER(driver_globals);
    if (!or_in_values) {
        device_init->device_characteristics = 0;
    }
    device_init->device_characteristics |= device_characteristics;
}

static _Must_inspect_result_ _IRQL_requires_max_(PASSIVE_LEVEL) NTSTATUS _WdfDeviceInitAssignName(
    _In_ PWDF_DRIVER_GLOBALS driver_globals, _In_ PWDFDEVICE_INIT device_init, _In_opt_ PCUNICODE_STRING device_name)
{
    if (driver_globals != &g_UsersimWdfDriverGlobals) {
        return STATUS_INVALID_PARAMETER;
    }
    if (cxplat_fault_injection_inject_fault()) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    if (device_name != nullptr) {
        device_init->device_name = *device_name;
    }
    return STATUS_SUCCESS;
}

static _IRQL_requires_max_(DISPATCH_LEVEL) VOID _WdfDeviceInitSetFileObjectConfig(
    _In_ PWDF_DRIVER_GLOBALS driver_globals,
    _In_ PWDFDEVICE_INIT device_init,
    _In_ PWDF_FILEOBJECT_CONFIG file_object_config,
    _In_opt_ PWDF_OBJECT_ATTRIBUTES file_object_attributes)
{
    UNREFERENCED_PARAMETER(driver_globals);
    device_init->file_object_config = file_object_config;
    device_init->file_object_attributes = file_object_attributes;
}

static _Must_inspect_result_ _IRQL_requires_max_(DISPATCH_LEVEL) NTSTATUS _WdfDeviceInitAssignWdmIrpPreprocessCallback(
    _In_ PWDF_DRIVER_GLOBALS driver_globals,
    _In_ PWDFDEVICE_INIT device_init,
    _In_ PFN_WDFDEVICE_WDM_IRP_PREPROCESS evt_device_wdm_irp_preprocess,
    UCHAR major_function,
    _When_(num_minor_functions > 0, _In_reads_bytes_(num_minor_functions)) _When_(num_minor_functions == 0, _In_opt_)
        PUCHAR minor_functions,
    ULONG num_minor_functions)
{
    if (driver_globals != &g_UsersimWdfDriverGlobals) {
        return STATUS_INVALID_PARAMETER;
    }
    if (device_init->minor_functions[major_function] != nullptr) {
        return STATUS_INVALID_DEVICE_REQUEST;
    }
    if (major_function >= IRP_MJ_MAXIMUM_FUNCTION) {
        return STATUS_INVALID_PARAMETER;
    }
    if (cxplat_fault_injection_inject_fault()) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    device_init->evt_device_wdm_irp_preprocess = evt_device_wdm_irp_preprocess;
    device_init->minor_functions[major_function] = minor_functions;
    device_init->num_minor_functions[major_function] = num_minor_functions;
    return STATUS_SUCCESS;
}

static _Must_inspect_result_ _IRQL_requires_max_(PASSIVE_LEVEL) NTSTATUS _WdfDeviceCreateSymbolicLink(
    _In_ PWDF_DRIVER_GLOBALS driver_globals, _In_ WDFDEVICE device, _In_ PCUNICODE_STRING symbolic_link_name)
{
    UNREFERENCED_PARAMETER(device);
    UNREFERENCED_PARAMETER(symbolic_link_name);

    if (driver_globals != &g_UsersimWdfDriverGlobals) {
        return STATUS_INVALID_PARAMETER;
    }
    if (cxplat_fault_injection_inject_fault()) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    return STATUS_SUCCESS;
}

static _Must_inspect_result_ _IRQL_requires_max_(DISPATCH_LEVEL) NTSTATUS _WdfIoQueueCreate(
    _In_ PWDF_DRIVER_GLOBALS driver_globals,
    _In_ WDFDEVICE device,
    _In_ PWDF_IO_QUEUE_CONFIG config,
    _In_opt_ PWDF_OBJECT_ATTRIBUTES queue_attributes,
    _Out_opt_ WDFQUEUE* queue)
{
    UNREFERENCED_PARAMETER(queue_attributes);

    PWDFDEVICE_INIT device_object = (PWDFDEVICE_INIT)device;
    device_object->io_queue_config = *config;

    if (driver_globals != &g_UsersimWdfDriverGlobals) {
        return STATUS_INVALID_PARAMETER;
    }
    if (cxplat_fault_injection_inject_fault()) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    if (queue != nullptr) {
        *queue = device;
    }
    return STATUS_SUCCESS;
}

static
_IRQL_requires_max_(DISPATCH_LEVEL)
WDFDEVICE _WdfIoQueueGetDevice(_In_ WDFQUEUE queue)
{
    return (WDFDEVICE)queue;
}

static _IRQL_requires_max_(DISPATCH_LEVEL) VOID
_WdfControlFinishInitializing(_In_ PWDF_DRIVER_GLOBALS driver_globals, _In_ WDFDEVICE device)
{
    UNREFERENCED_PARAMETER(driver_globals);
    UNREFERENCED_PARAMETER(device);
}

_IRQL_requires_max_(DISPATCH_LEVEL) PDEVICE_OBJECT
    _WdfDeviceWdmGetDeviceObject(_In_ PWDF_DRIVER_GLOBALS driver_globals, _In_ WDFDEVICE device)
{
    UNREFERENCED_PARAMETER(driver_globals);
    return (PDEVICE_OBJECT)device;
}

static
_IRQL_requires_max_(DISPATCH_LEVEL) VOID
    _WdfObjectDelete(_In_ PWDF_DRIVER_GLOBALS driver_globals, _In_ WDFOBJECT object)
{
    DRIVER_OBJECT* driver_object = (DRIVER_OBJECT*)driver_globals->Driver;
    if (driver_object) {
        for (auto it = driver_object->devices.begin(); it != driver_object->devices.end(); it++) {
            if (*it == object) {
                driver_object->devices.erase(it);
                break;
            }
        }
    }

    cxplat_free(object, CXPLAT_POOL_FLAG_NON_PAGED, 0);
}

typedef struct _wdfrequest
{
    NTSTATUS status;
    ULONG_PTR information;
    size_t buffer_size;
    _Field_size_(buffer_size) char buffer[0];
} wdfrequest_t;

static _IRQL_requires_max_(DISPATCH_LEVEL) VOID _WdfRequestCompleteWithInformation(
    _In_ PWDF_DRIVER_GLOBALS driver_globals, _In_ WDFREQUEST request, _In_ NTSTATUS status, _In_ ULONG_PTR information)
{
    UNREFERENCED_PARAMETER(driver_globals);

    wdfrequest_t* internal_request = (wdfrequest_t*)request;
    internal_request->status = status;
    internal_request->information = information;
}

static _Must_inspect_result_ _IRQL_requires_max_(DISPATCH_LEVEL) NTSTATUS
_WdfRequestRetrieveInputBuffer(
    _In_ PWDF_DRIVER_GLOBALS driver_globals,
    _In_ WDFREQUEST request,
    _In_ size_t minimum_required_length,
    _Outptr_result_bytebuffer_(*length) PVOID* buffer,
    _Out_opt_ size_t* length)
{
    UNREFERENCED_PARAMETER(driver_globals);

    wdfrequest_t* internal_request = (wdfrequest_t*)request;
    if (internal_request->buffer_size < minimum_required_length) {
        return STATUS_BUFFER_TOO_SMALL;
    }
    *buffer = internal_request->buffer;
    if (length != NULL) {
        *length = internal_request->buffer_size;
    }
    return STATUS_SUCCESS;
}

static _Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
NTSTATUS
_WdfRequestRetrieveOutputBuffer(
    _In_ PWDF_DRIVER_GLOBALS driver_globals,
    _In_ WDFREQUEST request,
    _In_ size_t minimum_required_size,
    _Outptr_result_bytebuffer_(*length) PVOID* buffer,
    _Out_opt_ size_t* length)
{
    UNREFERENCED_PARAMETER(driver_globals);

    wdfrequest_t* internal_request = (wdfrequest_t*)request;
    *buffer = internal_request->buffer;
    if (length != NULL) {
        *length = internal_request->buffer_size;
    }
    return (*length < minimum_required_size) ? STATUS_BUFFER_TOO_SMALL : STATUS_SUCCESS;
}

WDFFUNC g_UsersimWdfFunctions[WdfFunctionTableNumEntries];

void
usersim_initialize_wdf()
{
    g_UsersimWdfFunctions[WdfControlDeviceInitAllocateTableIndex] = (WDFFUNC)_WdfControlDeviceInitAllocate;
    g_UsersimWdfFunctions[WdfControlFinishInitializingTableIndex] = (WDFFUNC)_WdfControlFinishInitializing;
    g_UsersimWdfFunctions[WdfDeviceWdmGetDeviceObjectTableIndex] = (WDFFUNC)_WdfDeviceWdmGetDeviceObject;
    g_UsersimWdfFunctions[WdfDeviceInitFreeTableIndex] = (WDFFUNC)_WdfDeviceInitFree;
    g_UsersimWdfFunctions[WdfDeviceInitSetDeviceTypeTableIndex] = (WDFFUNC)_WdfDeviceInitSetDeviceType;
    g_UsersimWdfFunctions[WdfDeviceInitAssignNameTableIndex] = (WDFFUNC)_WdfDeviceInitAssignName;
    g_UsersimWdfFunctions[WdfDeviceInitSetCharacteristicsTableIndex] = (WDFFUNC)_WdfDeviceInitSetCharacteristics;
    g_UsersimWdfFunctions[WdfDeviceInitSetFileObjectConfigTableIndex] = (WDFFUNC)_WdfDeviceInitSetFileObjectConfig;
    g_UsersimWdfFunctions[WdfDeviceInitAssignWdmIrpPreprocessCallbackTableIndex] =
        (WDFFUNC)_WdfDeviceInitAssignWdmIrpPreprocessCallback;
    g_UsersimWdfFunctions[WdfDeviceCreateTableIndex] = (WDFFUNC)_WdfDeviceCreate;
    g_UsersimWdfFunctions[WdfDeviceCreateSymbolicLinkTableIndex] = (WDFFUNC)_WdfDeviceCreateSymbolicLink;
    g_UsersimWdfFunctions[WdfDriverCreateTableIndex] = (WDFFUNC)_WdfDriverCreate;
    g_UsersimWdfFunctions[WdfIoQueueCreateTableIndex] = (WDFFUNC)_WdfIoQueueCreate;
    g_UsersimWdfFunctions[WdfIoQueueGetDeviceTableIndex] = (WDFFUNC)_WdfIoQueueGetDevice;
    g_UsersimWdfFunctions[WdfObjectDeleteTableIndex] = (WDFFUNC)_WdfObjectDelete;
    g_UsersimWdfFunctions[WdfRequestCompleteWithInformationTableIndex] = (WDFFUNC)_WdfRequestCompleteWithInformation;
    g_UsersimWdfFunctions[WdfRequestRetrieveInputBufferTableIndex] = (WDFFUNC)_WdfRequestRetrieveInputBuffer;
    g_UsersimWdfFunctions[WdfRequestRetrieveOutputBufferTableIndex] = (WDFFUNC)_WdfRequestRetrieveOutputBuffer;
}

extern "C"
{
    USERSIM_API
    PWDF_DRIVER_GLOBALS UsersimWdfDriverGlobals = &g_UsersimWdfDriverGlobals;

    USERSIM_API const WDFFUNC* UsersimWdfFunctions = g_UsersimWdfFunctions;
}

WDFDRIVER
WdfGetDriver() { return g_UsersimWdfDriverGlobals.Driver; }

void
WDF_DRIVER_CONFIG_INIT(_Out_ PWDF_DRIVER_CONFIG config, _In_opt_ PFN_WDF_DRIVER_DEVICE_ADD evt_driver_device_add)
{
    *config = {.EvtDriverDeviceAdd = evt_driver_device_add};
}

WDFDRIVER
usersim_get_driver_from_module(HMODULE module)
{
    usersim_dll_get_driver_from_module_t usersim_dll_get_driver_from_module =
        (usersim_dll_get_driver_from_module_t)GetProcAddress(module, "usersim_dll_get_driver_from_module");
    return usersim_dll_get_driver_from_module();
}

WDFDEVICE
usersim_get_device_by_name(WDFDRIVER driver, _In_opt_ PCWSTR device_name)
{
    PDRIVER_OBJECT driver_object = (PDRIVER_OBJECT)driver;
    if (!device_name) {
        return (!driver_object->devices.empty()) ? driver_object->devices.front() : nullptr;
    }
    UNICODE_STRING unicode_device_name;
    RtlInitUnicodeString(&unicode_device_name, device_name);
    for (PDEVICE_OBJECT device : driver_object->devices) {
        if (device->init.device_name.Length != unicode_device_name.Length) {
            continue;
        }
        if (memcmp(device->init.device_name.Buffer, unicode_device_name.Buffer, unicode_device_name.Length) == 0) {
            return (WDFDEVICE)device;
        }
    }
    return nullptr;
}

BOOL
usersim_device_io_control(
    HANDLE device_handle,
    DWORD io_control_code,
    _In_reads_opt_(in_buffer_size) void* in_buffer,
    DWORD in_buffer_size,
    _Out_writes_to_opt_(out_buffer_size, *bytes_returned) void* out_buffer,
    DWORD out_buffer_size,
    _Out_opt_ DWORD* bytes_returned,
    _Inout_opt_ OVERLAPPED* overlapped)
{
    UNREFERENCED_PARAMETER(overlapped);

    PWDFDEVICE_INIT device_object = (PWDFDEVICE_INIT)device_handle;
    if (!device_object->io_queue_config.EvtIoDeviceControl) {
        return FALSE;
    }

    size_t buffer_size = max(max(in_buffer_size, out_buffer_size), 1);
    wdfrequest_t* request = (wdfrequest_t*)cxplat_allocate(
        CXPLAT_POOL_FLAG_NON_PAGED, FIELD_OFFSET(wdfrequest_t, buffer[buffer_size]), USERSIM_TAG_WDF_REQUEST);
    if (!request) {
        return FALSE;
    }
    request->buffer_size = in_buffer_size;
    memcpy(&request->buffer, in_buffer, in_buffer_size);

    WDFQUEUE queue = (WDFQUEUE)device_object;
    device_object->io_queue_config.EvtIoDeviceControl(queue, (WDFREQUEST)request, out_buffer_size, in_buffer_size, io_control_code);

    *bytes_returned = (DWORD)request->information;
    memcpy(out_buffer, &request->buffer, *bytes_returned);
    BOOL result = NT_SUCCESS(request->status);
    cxplat_free(request, CXPLAT_POOL_FLAG_NON_PAGED, USERSIM_TAG_WDF_REQUEST);
    return result;
}
