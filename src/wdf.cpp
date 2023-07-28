// Copyright (c) Microsoft Corporation
// SPDX-License-Identifier: MIT

#include "framework.h"
#include "wdf.h"

static WDFDRIVER g_CurrentDriver = {};

NTSTATUS
WdfDriverCreate(
    _In_ PDRIVER_OBJECT driver_object,
    _In_ PCUNICODE_STRING registry_path,
    _In_opt_ PWDF_OBJECT_ATTRIBUTES driver_attributes,
    _In_ PWDF_DRIVER_CONFIG driver_config,
    _Out_opt_ WDFDRIVER* driver)
{
    UNREFERENCED_PARAMETER(driver_object);
    UNREFERENCED_PARAMETER(registry_path);
    UNREFERENCED_PARAMETER(driver_attributes);

    g_CurrentDriver.config = *driver_config;
    if (driver != nullptr) {
        *driver = g_CurrentDriver;
    }
    return STATUS_SUCCESS;
}

NTSTATUS
WdfDeviceCreate(
    _Inout_ PWDFDEVICE_INIT* device_init, _In_opt_ PWDF_OBJECT_ATTRIBUTES device_attributes, _Out_ WDFDEVICE* device)
{
    UNREFERENCED_PARAMETER(device_init);
    UNREFERENCED_PARAMETER(device_attributes);

    *device = nullptr;
    return STATUS_SUCCESS;
}

WDF_DRIVER_GLOBALS g_UsersimWdfDriverGlobals = {
    .Driver = g_CurrentDriver,
};

const WDFFUNC g_UsersimWdfFunctions = {};

extern "C"
{
    USERSIM_API
    PWDF_DRIVER_GLOBALS UsersimWdfDriverGlobals = &g_UsersimWdfDriverGlobals;

    USERSIM_API const WDFFUNC* UsersimWdfFunctions = &g_UsersimWdfFunctions;
}

WDFDRIVER
WdfGetDriver() { return g_CurrentDriver; }

void
WDF_DRIVER_CONFIG_INIT(_Out_ PWDF_DRIVER_CONFIG config, _In_opt_ PFN_WDF_DRIVER_DEVICE_ADD evt_driver_device_add)
{
    *config = {.EvtDriverDeviceAdd = evt_driver_device_add};
}
