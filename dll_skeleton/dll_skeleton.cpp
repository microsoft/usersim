// Copyright (c) Microsoft Corporation
// SPDX-License-Identifier: MIT

#include "..\inc\ntddk.h"
#include "..\inc\wdf.h"

#include <winsock2.h>
#include <Windows.h>

DRIVER_INITIALIZE DriverEntry;

struct _driver_object
{
    int dummy;
};

NTSTATUS
StartDriver()
{
    DRIVER_OBJECT driver_object = {};
    UNICODE_STRING registry_path = {};
    return DriverEntry(&driver_object, &registry_path);
}

void
StopDriver()
{
    WDFDRIVER driver = WdfGetDriver();
    if (driver.config.EvtDriverUnload != nullptr) {
        driver.config.EvtDriverUnload(driver);
    }
}

bool APIENTRY
DllMain(HMODULE hModule, unsigned long ul_reason_for_call, void* lpReserved)
{
    UNREFERENCED_PARAMETER(hModule);
    UNREFERENCED_PARAMETER(lpReserved);
    switch (ul_reason_for_call) {
    case DLL_PROCESS_ATTACH:
        return NT_SUCCESS(StartDriver());
    case DLL_THREAD_ATTACH:
        break;
    case DLL_THREAD_DETACH:
        break;
    case DLL_PROCESS_DETACH:
        StopDriver();
        break;
    }
    return TRUE;
}
