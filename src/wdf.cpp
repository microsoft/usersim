// Copyright (c) Microsoft Corporation
// SPDX-License-Identifier: MIT

#include "framework.h"
#include "usersim/wdf.h"

static WDFDRIVER g_CurrentDriver = {};

static NTSTATUS
_WdfDriverCreate(
    _In_ WDF_DRIVER_GLOBALS* driver_globals,
    _In_ PDRIVER_OBJECT driver_object,
    _In_ PCUNICODE_STRING registry_path,
    _In_opt_ PWDF_OBJECT_ATTRIBUTES driver_attributes,
    _In_ PWDF_DRIVER_CONFIG driver_config,
    _Out_opt_ WDFDRIVER* driver)
{
    UNREFERENCED_PARAMETER(driver_globals);
    UNREFERENCED_PARAMETER(driver_object);
    UNREFERENCED_PARAMETER(registry_path);
    UNREFERENCED_PARAMETER(driver_attributes);

    g_CurrentDriver.config = *driver_config;
    if (driver != nullptr) {
        *driver = g_CurrentDriver;
    }
    return STATUS_SUCCESS;
}

static NTSTATUS
_WdfDeviceCreate(
    _In_ WDF_DRIVER_GLOBALS* driver_globals,
    _Inout_ PWDFDEVICE_INIT* device_init, _In_opt_ PWDF_OBJECT_ATTRIBUTES device_attributes, _Out_ WDFDEVICE* device)
{
    UNREFERENCED_PARAMETER(driver_globals);
    UNREFERENCED_PARAMETER(device_init);
    UNREFERENCED_PARAMETER(device_attributes);

    *device = nullptr;
    return STATUS_SUCCESS;
}

WDF_DRIVER_GLOBALS g_UsersimWdfDriverGlobals = {
    .Driver = g_CurrentDriver,
};

typedef enum _WDFFUNCENUM
{
    WdfDeviceCreateTableIndex = 75,
    WdfDriverCreateTableIndex = 116,
    WdfFunctionTableNumEntries = 444,
} WDFFUNCENUM;

WDFFUNC g_UsersimWdfFunctions[WdfFunctionTableNumEntries];

void
usersim_initialize_wdf()
{
    g_UsersimWdfFunctions[WdfDeviceCreateTableIndex] = (WDFFUNC)_WdfDeviceCreate;
    g_UsersimWdfFunctions[WdfDriverCreateTableIndex] = (WDFFUNC)_WdfDriverCreate;
}

extern "C"
{
    USERSIM_API
    PWDF_DRIVER_GLOBALS UsersimWdfDriverGlobals = &g_UsersimWdfDriverGlobals;

    USERSIM_API const WDFFUNC* UsersimWdfFunctions = g_UsersimWdfFunctions;
}

WDFDRIVER
WdfGetDriver() { return g_CurrentDriver; }

void
WDF_DRIVER_CONFIG_INIT(_Out_ PWDF_DRIVER_CONFIG config, _In_opt_ PFN_WDF_DRIVER_DEVICE_ADD evt_driver_device_add)
{
    *config = {.EvtDriverDeviceAdd = evt_driver_device_add};
}
