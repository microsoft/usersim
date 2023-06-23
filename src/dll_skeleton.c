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
UsersimStartDriver()
{
    DRIVER_OBJECT driver_object = {0};
    UNICODE_STRING registry_path = {0};
    return DriverEntry(&driver_object, &registry_path);
}

void
UsersimStopDriver()
{
    WDFDRIVER driver = WdfGetDriver();
    if (driver.config.EvtDriverUnload != NULL) {
        driver.config.EvtDriverUnload(driver);
    }
}
