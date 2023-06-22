// Copyright (c) Microsoft Corporation
// SPDX-License-Identifier: MIT

#include "framework.h"
#include "ntddk.h"
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
    g_CurrentDriver.config = *driver_config;
    *driver = g_CurrentDriver;
    return STATUS_SUCCESS;
}

NTSTATUS
WdfDeviceCreate(
    _Inout_ PWDFDEVICE_INIT* device_init, _In_opt_ PWDF_OBJECT_ATTRIBUTES device_attributes, _Out_ WDFDEVICE* device)
{
    return STATUS_SUCCESS;
}

WDFDRIVER
WdfGetDriver() { return g_CurrentDriver; }

void
WDF_DRIVER_CONFIG_INIT(_Out_ PWDF_DRIVER_CONFIG config, _In_opt_ PFN_WDF_DRIVER_DEVICE_ADD evt_driver_device_add)
{
    *config = {.EvtDriverDeviceAdd = evt_driver_device_add};
}