// Copyright (c) Microsoft Corporation
// SPDX-License-Identifier: MIT

#pragma once
#if defined(__cplusplus)
extern "C"
{
#endif

    typedef HANDLE WDFDEVICE;
    typedef struct _wdfdriver WDFDRIVER;

    typedef struct _WDF_OBJECT_ATTRIBUTES WDF_OBJECT_ATTRIBUTES, *PWDF_OBJECT_ATTRIBUTES;
    typedef struct _WDFDEVICE_INIT WDFDEVICE_INIT, *PWDFDEVICE_INIT;

    typedef NTSTATUS(EVT_WDF_DRIVER_DEVICE_ADD)(_In_ WDFDRIVER driver, _Inout_ PWDFDEVICE_INIT device_init);
    typedef EVT_WDF_DRIVER_DEVICE_ADD* PFN_WDF_DRIVER_DEVICE_ADD;

    typedef void(EVT_WDF_DRIVER_UNLOAD)(_In_ WDFDRIVER driver);
    typedef EVT_WDF_DRIVER_UNLOAD* PFN_WDF_DRIVER_UNLOAD;

    typedef struct _WDF_DRIVER_CONFIG
    {
        ULONG Size;
        PFN_WDF_DRIVER_DEVICE_ADD EvtDriverDeviceAdd;
        PFN_WDF_DRIVER_UNLOAD EvtDriverUnload;
        ULONG DriverInitFlags;
        ULONG DriverPoolTag;
    } WDF_DRIVER_CONFIG, *PWDF_DRIVER_CONFIG;

    typedef struct _wdfdriver
    {
        WDF_DRIVER_CONFIG config;
    } WDFDRIVER;

#define WDF_NO_OBJECT_ATTRIBUTES 0
#define WDF_NO_HANDLE 0

    NTSTATUS
    WdfDriverCreate(
        _In_ PDRIVER_OBJECT driver_object,
        _In_ PCUNICODE_STRING registry_path,
        _In_opt_ PWDF_OBJECT_ATTRIBUTES driver_attributes,
        _In_ PWDF_DRIVER_CONFIG driver_config,
        _Out_opt_ WDFDRIVER* driver);

    NTSTATUS
    WdfDeviceCreate(
        _Inout_ PWDFDEVICE_INIT* device_init,
        _In_opt_ PWDF_OBJECT_ATTRIBUTES device_attributes,
        _Out_ WDFDEVICE* device);

    WDFDRIVER
    WdfGetDriver();

    void
    WDF_DRIVER_CONFIG_INIT(_Out_ PWDF_DRIVER_CONFIG config, _In_opt_ PFN_WDF_DRIVER_DEVICE_ADD evt_driver_device_add);

#if defined(__cplusplus)
}
#endif