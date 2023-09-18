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

DRIVER_INITIALIZE DriverEntry;

__declspec(dllimport)
PWDF_DRIVER_GLOBALS UsersimWdfDriverGlobals;

__declspec(dllimport) const WDFFUNC* UsersimWdfFunctions;

PWDF_DRIVER_GLOBALS WdfDriverGlobals = NULL;
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

    __declspec(dllexport) HANDLE usersim_dll_get_device_handle(_In_opt_z_ const WCHAR* device_name)
    {
        // TODO(issue #114): support multiple devices per driver.
        UNREFERENCED_PARAMETER(device_name);
        DRIVER_OBJECT* driver = (DRIVER_OBJECT*)WdfDriverGlobals->Driver;
        return driver->device;
    }

    void
    UsersimStopDriver()
    {
        DRIVER_OBJECT* driver = (DRIVER_OBJECT*)WdfDriverGlobals->Driver;
        if (driver->config.EvtDriverUnload != NULL) {
            driver->config.EvtDriverUnload(driver);
        }

        if (driver->device) {
            // Free device, which will free the control device object.
            WdfObjectDelete_t* WdfObjectDelete = (WdfObjectDelete_t*)UsersimWdfFunctions[WdfObjectDeleteTableIndex];
            WdfObjectDelete(UsersimWdfDriverGlobals, driver->device);
            driver->device = NULL;
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