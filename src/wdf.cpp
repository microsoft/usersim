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
static WdfDeviceWdmGetDeviceObject_t _WdfDeviceWdmGetDeviceObject;
static WdfObjectDelete_t _WdfObjectDelete;

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
    driver_object->device = nullptr;
    if (driver != nullptr) {
        *driver = driver_object;
    }
    return STATUS_SUCCESS;
}

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
    if (cxplat_fault_injection_inject_fault()) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    DRIVER_OBJECT* driver_object = (DRIVER_OBJECT*)driver_globals->Driver;
    driver_object->device = (PDEVICE_OBJECT)*device_init;
    *device = *device_init;
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
} WDFDEVICE_INIT;

static _Must_inspect_result_ _IRQL_requires_max_(PASSIVE_LEVEL) PWDFDEVICE_INIT _WdfControlDeviceInitAllocate(
    _In_ PWDF_DRIVER_GLOBALS driver_globals, _In_ WDFDRIVER driver, _In_ CONST UNICODE_STRING* sddl_string)
{
    UNREFERENCED_PARAMETER(driver_globals);
    UNREFERENCED_PARAMETER(driver);
    UNREFERENCED_PARAMETER(sddl_string);

    PWDFDEVICE_INIT device_init =
        (PWDFDEVICE_INIT)cxplat_allocate(CxPlatNonPagedPoolNx, sizeof(*device_init), USERSIM_TAG_WDF_DEVICE_INIT, true);
    return device_init;
}

static _IRQL_requires_max_(DISPATCH_LEVEL) VOID
    _WdfDeviceInitFree(_In_ PWDF_DRIVER_GLOBALS driver_globals, _In_ PWDFDEVICE_INIT device_init)
{
    UNREFERENCED_PARAMETER(driver_globals);
    cxplat_free(device_init, USERSIM_TAG_WDF_DEVICE_INIT);
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
    UNREFERENCED_PARAMETER(device);
    UNREFERENCED_PARAMETER(config);
    UNREFERENCED_PARAMETER(queue_attributes);

    if (driver_globals != &g_UsersimWdfDriverGlobals) {
        return STATUS_INVALID_PARAMETER;
    }
    if (cxplat_fault_injection_inject_fault()) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    if (queue != nullptr) {
        memset(queue, 0, sizeof(*queue));
    }
    return STATUS_SUCCESS;
}

_IRQL_requires_max_(DISPATCH_LEVEL) VOID
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

_IRQL_requires_max_(DISPATCH_LEVEL) VOID
    _WdfObjectDelete(_In_ PWDF_DRIVER_GLOBALS driver_globals, _In_ WDFOBJECT object)
{
    UNREFERENCED_PARAMETER(driver_globals);

    cxplat_free_any_tag(object);
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
    g_UsersimWdfFunctions[WdfObjectDeleteTableIndex] = (WDFFUNC)_WdfObjectDelete;
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
