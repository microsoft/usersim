// Copyright (c) Microsoft Corporation
// SPDX-License-Identifier: MIT

#define CATCH_CONFIG_MAIN
#if !defined(CMAKE_NUGET)
#include <catch2/catch_all.hpp>
#else
#include <catch2/catch.hpp>
#endif
#include "usersim/wdf.h"

// The following must be included _after_ wdf.h.
#include "cxplat_passed_test_log.h"
CATCH_REGISTER_LISTENER(cxplat_passed_test_log)

#define IOCTL_KMDF_HELLO_WORLD_CTL_METHOD_BUFFERED 1

TEST_CASE("DriverEntry", "[wdf]")
{
    HMODULE module = LoadLibraryW(L"sample.dll");
    REQUIRE(module != nullptr);

    HANDLE device_handle = usersim_get_device_handle(module, nullptr);
    REQUIRE(device_handle != nullptr);

    // Send an unrecognized ioctl.
    DWORD bytes_returned;
    BOOL ok = usersim_device_io_control(
        device_handle, 0, nullptr, 0, nullptr, 0, &bytes_returned, nullptr);
    REQUIRE(!ok);

    // Send an ioctl with invalid parameters.
    ok = usersim_device_io_control(
        device_handle, IOCTL_KMDF_HELLO_WORLD_CTL_METHOD_BUFFERED, nullptr, 0, nullptr, 0, &bytes_returned, nullptr);
    REQUIRE(!ok);

    // Send an ioctl with valid parameters.
    uint64_t input = 42;
    uint64_t output;
    ok = usersim_device_io_control(
        device_handle,
        IOCTL_KMDF_HELLO_WORLD_CTL_METHOD_BUFFERED,
        &input,
        sizeof(input),
        &output,
        sizeof(output),
        &bytes_returned,
        nullptr);
    REQUIRE(ok);
    REQUIRE(bytes_returned == sizeof(output));
    REQUIRE(output == input);

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

struct _wdf_control_device_init_free_functor
{
    void
    operator()(void* memory)
    {
        WdfDeviceInitFree_t* WdfDeviceInitFree = (WdfDeviceInitFree_t*)UsersimWdfFunctions[WdfDeviceInitFreeTableIndex];
        WdfDeviceInitFree(UsersimWdfDriverGlobals, (PWDFDEVICE_INIT)memory);
    }
};

struct _wdf_control_device_free_functor
{
    void
    operator()(void* memory)
    {
        // Free device, which will free the control device object.
        WdfObjectDelete_t* WdfObjectDelete = (WdfObjectDelete_t*)UsersimWdfFunctions[WdfObjectDeleteTableIndex];
        WdfObjectDelete(UsersimWdfDriverGlobals, (WDFDEVICE)memory);
    }
};

static WDFDEVICE
_test_create_device(WDFDRIVER driver, _In_ PCWSTR device_name, _In_ PCWSTR symbolic_device_name)
{
    // Allocate control device object.  For usage discussion, see
    // https://learn.microsoft.com/en-us/windows-hardware/drivers/wdf/using-control-device-objects#creating-a-control-device-object
    WdfControlDeviceInitAllocate_t* WdfControlDeviceInitAllocate =
        (WdfControlDeviceInitAllocate_t*)UsersimWdfFunctions[WdfControlDeviceInitAllocateTableIndex];
    DECLARE_CONST_UNICODE_STRING(security_descriptor, L"");
    std::unique_ptr<WDFDEVICE_INIT, _wdf_control_device_init_free_functor> init(
        WdfControlDeviceInitAllocate(UsersimWdfDriverGlobals, driver, &security_descriptor));
    REQUIRE(init != nullptr);

    // Set device type.
    WdfDeviceInitSetDeviceType_t* WdfDeviceInitSetDeviceType =
        (WdfDeviceInitSetDeviceType_t*)UsersimWdfFunctions[WdfDeviceInitSetDeviceTypeTableIndex];
    WdfDeviceInitSetDeviceType(UsersimWdfDriverGlobals, init.get(), FILE_DEVICE_NULL);

    // Set device characteristics.
    WdfDeviceInitSetCharacteristics_t* WdfDeviceInitSetCharacteristics =
        (WdfDeviceInitSetCharacteristics_t*)UsersimWdfFunctions[WdfDeviceInitSetCharacteristicsTableIndex];
    WdfDeviceInitSetCharacteristics(UsersimWdfDriverGlobals, init.get(), FILE_DEVICE_SECURE_OPEN, FALSE);

    // Set device name.
    UNICODE_STRING unicode_device_name;
    RtlInitUnicodeString(&unicode_device_name, device_name);
    WdfDeviceInitAssignName_t* WdfDeviceInitAssignName =
        (WdfDeviceInitAssignName_t*)UsersimWdfFunctions[WdfDeviceInitAssignNameTableIndex];
    NTSTATUS status = WdfDeviceInitAssignName(UsersimWdfDriverGlobals, init.get(), &unicode_device_name);
    REQUIRE(status == STATUS_SUCCESS);

    // Set file object config.
    WDF_OBJECT_ATTRIBUTES attributes = {0};
    WDF_FILEOBJECT_CONFIG file_object_config = {0};
    WdfDeviceInitSetFileObjectConfig_t* WdfDeviceInitSetFileObjectConfig =
        (WdfDeviceInitSetFileObjectConfig_t*)UsersimWdfFunctions[WdfDeviceInitSetFileObjectConfigTableIndex];
    WdfDeviceInitSetFileObjectConfig(UsersimWdfDriverGlobals, init.get(), &file_object_config, &attributes);

    WdfDeviceInitAssignWdmIrpPreprocessCallback_t* WdfDeviceInitAssignWdmIrpPreprocessCallback =
        (WdfDeviceInitAssignWdmIrpPreprocessCallback_t*)
            UsersimWdfFunctions[WdfDeviceInitAssignWdmIrpPreprocessCallbackTableIndex];
    status = WdfDeviceInitAssignWdmIrpPreprocessCallback(
        UsersimWdfDriverGlobals, init.get(), driver_query_volume_information, IRP_MJ_QUERY_VOLUME_INFORMATION, NULL, 0);
    REQUIRE(status == STATUS_SUCCESS);

    // Create device.
    WDFDEVICE device = nullptr;
    WdfDeviceCreate_t* WdfDeviceCreate = (WdfDeviceCreate_t*)UsersimWdfFunctions[WdfDeviceCreateTableIndex];
    WDFDEVICE_INIT* temporary_init = init.release();
    status = WdfDeviceCreate(UsersimWdfDriverGlobals, &temporary_init, nullptr, &device);
    init.reset(temporary_init);
    REQUIRE(status == STATUS_SUCCESS);
    REQUIRE(device != nullptr);

    WdfDeviceWdmGetDeviceObject_t* WdfDeviceWdmGetDeviceObject =
        (WdfDeviceWdmGetDeviceObject_t*)UsersimWdfFunctions[WdfDeviceWdmGetDeviceObjectTableIndex];
    PDEVICE_OBJECT device_object = WdfDeviceWdmGetDeviceObject(UsersimWdfDriverGlobals, device);
    REQUIRE(device_object != nullptr);

    // Create symbolic link for control object for user mode.
    UNICODE_STRING unicode_symbolic_device_name;
    RtlInitUnicodeString(&unicode_symbolic_device_name, symbolic_device_name);
    WdfDeviceCreateSymbolicLink_t* WdfDeviceCreateSymbolicLink =
        (WdfDeviceCreateSymbolicLink_t*)UsersimWdfFunctions[WdfDeviceCreateSymbolicLinkTableIndex];
    status = WdfDeviceCreateSymbolicLink(UsersimWdfDriverGlobals, device, &unicode_symbolic_device_name);
    REQUIRE(status == STATUS_SUCCESS);

    // Create an I/O queue.
    WDF_IO_QUEUE_CONFIG io_queue_configuration = {0};
    WdfIoQueueCreate_t* WdfIoQueueCreate = (WdfIoQueueCreate_t*)UsersimWdfFunctions[WdfIoQueueCreateTableIndex];
#pragma warning(suppress : 28193) // status must be inspected
    status = WdfIoQueueCreate(
        UsersimWdfDriverGlobals, device, &io_queue_configuration, WDF_NO_OBJECT_ATTRIBUTES, WDF_NO_HANDLE);
    REQUIRE(status == STATUS_SUCCESS);

    // Finish initializing the control device object.
    WdfControlFinishInitializing_t* WdfControlFinishInitializing =
        (WdfControlFinishInitializing_t*)UsersimWdfFunctions[WdfControlFinishInitializingTableIndex];
    WdfControlFinishInitializing(UsersimWdfDriverGlobals, device);

    return device;
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

    WDFDEVICE device1 = _test_create_device(driver, L"test device name 1", L"symbolic device name 1");
    WDFDEVICE device2 = _test_create_device(driver, L"test device name 2", L"symbolic device name 2");
    WDFDEVICE device3 = _test_create_device(driver, L"test device name 3", L"symbolic device name 3");

    // Free device3, which will free the control device object.
    // This tests a driver explicitly freeing a device.
    WdfObjectDelete_t* WdfObjectDelete = (WdfObjectDelete_t*)UsersimWdfFunctions[WdfObjectDeleteTableIndex];
    WdfObjectDelete(UsersimWdfDriverGlobals, (WDFDEVICE)device3);

    // Free the other devices remaining in the driver as if the driver were now
    // getting cleaned up.
    if (driver_object.device) {
        // Free device, which will free the control device object.
        WdfObjectDelete_t* WdfObjectDelete = (WdfObjectDelete_t*)UsersimWdfFunctions[WdfObjectDeleteTableIndex];
        WdfObjectDelete(UsersimWdfDriverGlobals, driver_object.device);
        driver_object.device = NULL;
    }
    UsersimWdfDriverGlobals->Driver = nullptr;
}
