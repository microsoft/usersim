// Copyright (c) Microsoft Corporation
// SPDX-License-Identifier: MIT

// The code in this file is adapted from code in
// https://learn.microsoft.com/en-us/windows-hardware/drivers/gettingstarted/writing-a-very-small-kmdf--driver

#include <ntddk.h>
#include <stdbool.h>
#include <stdint.h>
#include <wdf.h>

DRIVER_INITIALIZE DriverEntry;
EVT_WDF_DRIVER_DEVICE_ADD KmdfHelloWorldEvtDeviceAdd;
EVT_WDF_IO_QUEUE_IO_DEVICE_CONTROL KmdfHelloWorldIoDeviceControl;

NTSTATUS
DriverEntry(_In_ PDRIVER_OBJECT DriverObject, _In_ PUNICODE_STRING RegistryPath)
{
    // NTSTATUS variable to record success or failure
    NTSTATUS status = STATUS_SUCCESS;

    // Allocate the driver configuration object
    WDF_DRIVER_CONFIG config;

    // Print "Hello World" for DriverEntry
    KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "KmdfHelloWorld: DriverEntry\n"));

    // Initialize the driver configuration object to register the
    // entry point for the EvtDeviceAdd callback, KmdfHelloWorldEvtDeviceAdd
    WDF_DRIVER_CONFIG_INIT(&config, KmdfHelloWorldEvtDeviceAdd);

    // Finally, create the driver object
    status = WdfDriverCreate(DriverObject, RegistryPath, WDF_NO_OBJECT_ATTRIBUTES, &config, WDF_NO_HANDLE);
    return status;
}

NTSTATUS
KmdfHelloWorldEvtDeviceAdd(_In_ WDFDRIVER Driver, _Inout_ PWDFDEVICE_INIT DeviceInit)
{
    // We're not using the driver object,
    // so we need to mark it as unreferenced
    UNREFERENCED_PARAMETER(Driver);

    NTSTATUS status;

    // Allocate the device object
    WDFDEVICE hDevice;

    // Print "Hello World"
    KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "KmdfHelloWorld: KmdfHelloWorldEvtDeviceAdd\n"));

    // Create the device object
    status = WdfDeviceCreate(&DeviceInit, WDF_NO_OBJECT_ATTRIBUTES, &hDevice);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    // Create default queue.
    WDF_IO_QUEUE_CONFIG io_queue_configuration;
    WDF_IO_QUEUE_CONFIG_INIT_DEFAULT_QUEUE(&io_queue_configuration, WdfIoQueueDispatchParallel);
    io_queue_configuration.EvtIoDeviceControl = KmdfHelloWorldIoDeviceControl;
    status = WdfIoQueueCreate(hDevice, &io_queue_configuration, WDF_NO_OBJECT_ATTRIBUTES, WDF_NO_HANDLE);
    return status;
}

#define IOCTL_KMDF_HELLO_WORLD_CTL_METHOD_BUFFERED 1

VOID
KmdfHelloWorldIoDeviceControl(
    _In_ WDFQUEUE queue,
    _In_ WDFREQUEST request,
    size_t output_buffer_length,
    size_t input_buffer_length,
    unsigned long io_control_code)
{
    NTSTATUS status = STATUS_SUCCESS;
    WDFDEVICE device;
    void* input_buffer = NULL;
    void* output_buffer = NULL;
    size_t actual_input_length = 0;
    size_t actual_output_length = 0;

    // Set the minimum sizes based on the io_control_code.
    // Here we just set them to constant values.
    size_t minimum_request_size = sizeof(uint64_t);
    size_t minimum_reply_size = sizeof(uint64_t);

    device = WdfIoQueueGetDevice(queue);

    switch (io_control_code) {
    case IOCTL_KMDF_HELLO_WORLD_CTL_METHOD_BUFFERED:
        // Verify that length of the input buffer supplied to the request object
        // is not zero
        if (input_buffer_length != 0) {
            // Retrieve the input buffer associated with the request object
            status = WdfRequestRetrieveInputBuffer(
                request,             // Request object
                input_buffer_length, // Length of input buffer
                &input_buffer,       // Pointer to buffer
                &actual_input_length // Length of buffer
            );

            if (!NT_SUCCESS(status)) {
                goto Done;
            }

            if ((input_buffer == NULL) || (actual_input_length < minimum_request_size) ||
                (output_buffer_length < minimum_reply_size)) {
                status = STATUS_INVALID_PARAMETER;
                goto Done;
            }

            // Be aware: Input and output buffer point to the same memory.
            if (minimum_reply_size > 0) {
                // Retrieve output buffer associated with the request object
                status = WdfRequestRetrieveOutputBuffer(
                    request, output_buffer_length, &output_buffer, &actual_output_length);
                if (!NT_SUCCESS(status)) {
                    goto Done;
                }
                if (output_buffer == NULL) {
                    status = STATUS_INVALID_PARAMETER;
                    goto Done;
                }

                if (actual_output_length < minimum_reply_size) {
                    status = STATUS_BUFFER_TOO_SMALL;
                    goto Done;
                }
            }

            // Do the actual work, using the request, actual_input_length,
            // actual_output_length.
            memcpy(output_buffer, input_buffer, min(actual_input_length, actual_output_length));
            if (actual_output_length > actual_input_length) {
                RtlZeroMemory((char*)output_buffer + actual_input_length, actual_output_length - actual_input_length);
            }
            status = STATUS_SUCCESS;
            goto Done;
        } else {
            status = STATUS_INVALID_PARAMETER;
            goto Done;
        }
        break;
    default:
        status = STATUS_INVALID_PARAMETER;
        break;
    }

Done:
    if (status != STATUS_PENDING) {
        WdfRequestCompleteWithInformation(request, status, output_buffer_length);
    }
}
