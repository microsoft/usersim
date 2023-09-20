// Copyright (c) Microsoft Corporation
// SPDX-License-Identifier: MIT

#include <sal.h>
#include <winsock2.h>
#include <windows.h>
#include "usersim/rtl.h"
#include "usersim/wdf.h"

#define NT_SUCCESS(status) (((NTSTATUS)(status)) >= 0)

WDFDRIVER
WdfGetDriver();

CXPLAT_EXTERN_C
DRIVER_INITIALIZE DriverEntry;

CXPLAT_EXTERN_C
__declspec(dllimport)
PWDF_DRIVER_GLOBALS UsersimWdfDriverGlobals;

CXPLAT_EXTERN_C
__declspec(dllimport) const WDFFUNC* UsersimWdfFunctions;

CXPLAT_EXTERN_C
PWDF_DRIVER_GLOBALS WdfDriverGlobals = NULL;
CXPLAT_EXTERN_C
const WDFFUNC* WdfFunctions_01015 = NULL;
static DRIVER_OBJECT _driver_object = {0};

    NTSTATUS
    UsersimStartDriver()
    {
        WdfDriverGlobals = UsersimWdfDriverGlobals;
        WdfFunctions_01015 = UsersimWdfFunctions;

        UNICODE_STRING registry_path = {0};
        return DriverEntry(&_driver_object, &registry_path);
    }

    CXPLAT_EXTERN_C
    __declspec(dllexport) WDFDRIVER usersim_dll_get_driver_from_module()
    {
        return WdfDriverGlobals->Driver;
    }

    void
    UsersimStopDriver()
    {
        DRIVER_OBJECT* driver = (DRIVER_OBJECT*)WdfDriverGlobals->Driver;
        if (driver->config.EvtDriverUnload != NULL) {
            driver->config.EvtDriverUnload(driver);
        }

        while (!driver->devices.empty()) {
            PDEVICE_OBJECT device_object = driver->devices.back();

            // Free device, which will free the control device object.
            WdfObjectDelete_t* WdfObjectDelete = (WdfObjectDelete_t*)UsersimWdfFunctions[WdfObjectDeleteTableIndex];
            WdfObjectDelete(UsersimWdfDriverGlobals, device_object);
        }

        WdfDriverGlobals->Driver = NULL;
    }

    BOOL APIENTRY
    DllMain(HMODULE hModule, unsigned long ul_reason_for_call, void* lpReserved)
    {
        UNREFERENCED_PARAMETER(hModule);
        UNREFERENCED_PARAMETER(lpReserved);
        switch (ul_reason_for_call) {
        case DLL_PROCESS_ATTACH:
            return NT_SUCCESS(UsersimStartDriver());
        case DLL_THREAD_ATTACH:
            break;
        case DLL_THREAD_DETACH:
            break;
        case DLL_PROCESS_DETACH:
            UsersimStopDriver();
            break;
        }
        return TRUE;
    }