// Copyright (c) Microsoft Corporation
// SPDX-License-Identifier: MIT

#define CATCH_CONFIG_MAIN
#if !defined(CMAKE_NUGET)
#include <catch2/catch_all.hpp>
#else
#include <catch2/catch.hpp>
#endif
#include "usersim/wdf.h"

TEST_CASE("DriverEntry", "[wdf]")
{
    HMODULE module = LoadLibraryW(L"sample.dll");
    REQUIRE(module != nullptr);

    if (module) {
        FreeLibrary(module);
    }
}

extern "C"
{
    __declspec(dllimport) const WDFFUNC* UsersimWdfFunctions;
    __declspec(dllimport) PWDF_DRIVER_GLOBALS UsersimWdfDriverGlobals;
}

TEST_CASE("WdfDriverCreate", "[wdf]")
{
    DRIVER_OBJECT driver_object = {0};
    WDF_DRIVER_CONFIG config;
    WDF_DRIVER_CONFIG_INIT(&config, nullptr);

    WdfDriverCreate_t* WdfDriverCreate = (WdfDriverCreate_t*)UsersimWdfFunctions[WdfDriverCreateTableIndex];
    WDFDRIVER driver = nullptr;
    DECLARE_CONST_UNICODE_STRING(registry_path, L"");
    NTSTATUS status =
        WdfDriverCreate(UsersimWdfDriverGlobals, &driver_object, &registry_path, nullptr, &config, &driver);
    REQUIRE(status == STATUS_SUCCESS);
    REQUIRE(driver != nullptr);

    // Verify that a duplicate create fails.
    status = WdfDriverCreate(UsersimWdfDriverGlobals, &driver_object, &registry_path, nullptr, &config, &driver);
    REQUIRE(status == STATUS_DRIVER_INTERNAL_ERROR);

    UsersimWdfDriverGlobals->Driver = nullptr;
}

NTSTATUS
driver_query_volume_information(_In_ WDFDEVICE device, _Inout_ IRP* irp)
{
    UNREFERENCED_PARAMETER(device);
    UNREFERENCED_PARAMETER(irp);
    return STATUS_SUCCESS;
}

TEST_CASE("WdfDeviceCreate", "[wdf]")
{
    // Create driver to hold device.
    DRIVER_OBJECT driver_object = {0};
    WDF_DRIVER_CONFIG config;
    WDF_DRIVER_CONFIG_INIT(&config, nullptr);
    WdfDriverCreate_t* WdfDriverCreate = (WdfDriverCreate_t*)UsersimWdfFunctions[WdfDriverCreateTableIndex];
    WDFDRIVER driver = nullptr;
    DECLARE_CONST_UNICODE_STRING(registry_path, L"");
    NTSTATUS status = WdfDriverCreate(UsersimWdfDriverGlobals, &driver_object, &registry_path, nullptr, &config, &driver);
    REQUIRE(status == STATUS_SUCCESS);

    // Allocate control device object.
    WdfControlDeviceInitAllocate_t* WdfControlDeviceInitAllocate =
        (WdfControlDeviceInitAllocate_t*)UsersimWdfFunctions[WdfControlDeviceInitAllocateTableIndex];
    DECLARE_CONST_UNICODE_STRING(security_descriptor, L"");
    PWDFDEVICE_INIT init = WdfControlDeviceInitAllocate(UsersimWdfDriverGlobals, driver, &security_descriptor);
    REQUIRE(init != nullptr);

    // Set device type.
    WdfDeviceInitSetDeviceType_t* WdfDeviceInitSetDeviceType =
        (WdfDeviceInitSetDeviceType_t*)UsersimWdfFunctions[WdfDeviceInitSetDeviceTypeTableIndex];
    WdfDeviceInitSetDeviceType(UsersimWdfDriverGlobals, init, FILE_DEVICE_NULL);

    // Set device characteristics.
    WdfDeviceInitSetCharacteristics_t* WdfDeviceInitSetCharacteristics =
        (WdfDeviceInitSetCharacteristics_t*)UsersimWdfFunctions[WdfDeviceInitSetCharacteristicsTableIndex];
    WdfDeviceInitSetCharacteristics(UsersimWdfDriverGlobals, init, FILE_DEVICE_SECURE_OPEN, FALSE);

    // Set device name.
    UNICODE_STRING device_name;
    RtlInitUnicodeString(&device_name, L"test device name");
    WdfDeviceInitAssignName_t* WdfDeviceInitAssignName =
        (WdfDeviceInitAssignName_t*)UsersimWdfFunctions[WdfDeviceInitAssignNameTableIndex];
    status = WdfDeviceInitAssignName(UsersimWdfDriverGlobals, init, &device_name);
    REQUIRE(status == STATUS_SUCCESS);

    // Set file object config.
    WDF_OBJECT_ATTRIBUTES attributes = {0};
    WDF_FILEOBJECT_CONFIG file_object_config = {0};
    WdfDeviceInitSetFileObjectConfig_t* WdfDeviceInitSetFileObjectConfig =
        (WdfDeviceInitSetFileObjectConfig_t*)UsersimWdfFunctions[WdfDeviceInitSetFileObjectConfigTableIndex];
    WdfDeviceInitSetFileObjectConfig(UsersimWdfDriverGlobals, init, &file_object_config, &attributes);

    WdfDeviceInitAssignWdmIrpPreprocessCallback_t* WdfDeviceInitAssignWdmIrpPreprocessCallback =
        (WdfDeviceInitAssignWdmIrpPreprocessCallback_t*)UsersimWdfFunctions[WdfDeviceInitAssignWdmIrpPreprocessCallbackTableIndex];
    status = WdfDeviceInitAssignWdmIrpPreprocessCallback(
        UsersimWdfDriverGlobals, init, driver_query_volume_information, IRP_MJ_QUERY_VOLUME_INFORMATION, NULL, 0);
    REQUIRE(status == STATUS_SUCCESS);

    // Create device.
    WDFDEVICE device = nullptr;
    WdfDeviceCreate_t* WdfDeviceCreate = (WdfDeviceCreate_t*)UsersimWdfFunctions[WdfDeviceCreateTableIndex];
    status = WdfDeviceCreate(UsersimWdfDriverGlobals, &init, nullptr, &device);
    REQUIRE(status == STATUS_SUCCESS);
    REQUIRE(device != nullptr);

    // Create symbolic link for control object for user mode.
    UNICODE_STRING symbolic_device_name;
    RtlInitUnicodeString(&symbolic_device_name, L"symbolic device name");
    WdfDeviceCreateSymbolicLink_t* WdfDeviceCreateSymbolicLink =
        (WdfDeviceCreateSymbolicLink_t*)UsersimWdfFunctions[WdfDeviceCreateSymbolicLinkTableIndex];
    status = WdfDeviceCreateSymbolicLink(UsersimWdfDriverGlobals, device, &symbolic_device_name);
    REQUIRE(status == STATUS_SUCCESS);

    // Create an I/O queue.
    WDF_IO_QUEUE_CONFIG io_queue_configuration = {0};
    WdfIoQueueCreate_t* WdfIoQueueCreate = (WdfIoQueueCreate_t*)UsersimWdfFunctions[WdfIoQueueCreateTableIndex];
#pragma warning(suppress:28193) // status must be inspected
    status = WdfIoQueueCreate(
        UsersimWdfDriverGlobals, device, &io_queue_configuration, WDF_NO_OBJECT_ATTRIBUTES, WDF_NO_HANDLE);
    REQUIRE(status == STATUS_SUCCESS);

    // Free device, which will free the control device object.
    WdfObjectDelete_t* WdfObjectDelete = (WdfObjectDelete_t*)UsersimWdfFunctions[WdfObjectDeleteTableIndex];
    WdfObjectDelete(UsersimWdfDriverGlobals, device);

    UsersimWdfDriverGlobals->Driver = nullptr;
}