// Copyright (c) Microsoft Corporation
// SPDX-License-Identifier: MIT

#if !defined(CMAKE_NUGET)
#include <catch2/catch_all.hpp>
#else
#include <catch2/catch.hpp>
#endif
#include "../src/framework.h"

#include <../km/netioddk.h>
#include <atomic>
#include <chrono>
#include <iostream>
#include <thread>

NPIID test_npiid = {0};

#pragma region test_nmr_client

#define TEST_CLIENT_DISPATCH ((const void*)0x1)

NPI_REGISTRATION_INSTANCE _test_client_registration_instance = {
    .Size = sizeof(NPI_REGISTRATION_INSTANCE), .NpiId = &test_npiid};

typedef struct
{
    bool allocated;
    HANDLE nmr_binding_handle;
    void* provider_binding_context;
    const void* provider_dispatch;
} test_client_binding_context_t;

static test_client_binding_context_t _test_client_binding_context = {.allocated = false};
static bool _test_client_async_deregister = false;

static NTSTATUS
_test_client_attach_provider(
    _In_ HANDLE nmr_binding_handle,
    _In_opt_ void* client_context,
    _In_ NPI_REGISTRATION_INSTANCE* provider_registration_instance)
{
    // Verify not already bound.
    REQUIRE(_test_client_binding_context.allocated == false);
    _test_client_binding_context.allocated = true;

    NTSTATUS status = NmrClientAttachProvider(
        nmr_binding_handle,
        &_test_client_binding_context,
        TEST_CLIENT_DISPATCH,
        &_test_client_binding_context.provider_binding_context,
        &_test_client_binding_context.provider_dispatch);

    if (NT_SUCCESS(status)) {
        _test_client_binding_context.nmr_binding_handle = nmr_binding_handle;
    }

    return status;
}

static NTSTATUS
_test_client_detach_provider(_In_ void* client_binding_context)
{
    test_client_binding_context_t* context = (test_client_binding_context_t*)client_binding_context;
    REQUIRE(context->allocated);

    _test_client_binding_context.provider_binding_context = nullptr;
    _test_client_binding_context.provider_dispatch = nullptr;

    return (_test_client_async_deregister) ? STATUS_PENDING : STATUS_SUCCESS;
}

static void
_test_client_cleanup_binding_context(_In_ void* client_binding_context)
{
    test_client_binding_context_t* context = (test_client_binding_context_t*)client_binding_context;
    REQUIRE(context->allocated);
    context->allocated = false;
    context->nmr_binding_handle = nullptr;
}

NPI_CLIENT_CHARACTERISTICS _test_client_characteristics = {
    .Length = sizeof(NPI_CLIENT_CHARACTERISTICS),
    .ClientAttachProvider = (PNPI_CLIENT_ATTACH_PROVIDER_FN)_test_client_attach_provider,
    .ClientDetachProvider = _test_client_detach_provider,
    .ClientCleanupBindingContext = _test_client_cleanup_binding_context,
    .ClientRegistrationInstance = _test_client_registration_instance};

#pragma endregion test_nmr_client
#pragma region test_nmr_provider

#define TEST_PROVIDER_DISPATCH ((const void*)0x2)

NPI_REGISTRATION_INSTANCE _test_provider_registration_instance = {
    .Size = sizeof(NPI_REGISTRATION_INSTANCE), .NpiId = &test_npiid};

typedef struct
{
    bool allocated;
    HANDLE nmr_binding_handle;
    void* client_binding_context;
    const void* client_dispatch;
} test_provider_binding_context_t;

static test_provider_binding_context_t _test_provider_binding_context = {.allocated = false};
static bool _test_provider_async_deregister = false;

static NTSTATUS
_test_provider_attach_client(
    _In_ HANDLE nmr_binding_handle,
    _In_opt_ void* provider_context,
    _In_ NPI_REGISTRATION_INSTANCE* client_registration_instance,
    _In_ void* client_binding_context,
    _In_ const void* client_dispatch,
    _Outptr_ void** provider_binding_context,
    _Outptr_ const void** provider_dispatch)
{
    // Verify not already bound.
    REQUIRE(_test_provider_binding_context.allocated == false);
    _test_provider_binding_context.allocated = true;

    _test_provider_binding_context.nmr_binding_handle = nmr_binding_handle;
    _test_provider_binding_context.client_binding_context = client_binding_context;
    _test_provider_binding_context.client_dispatch = client_dispatch;

    *provider_dispatch = TEST_PROVIDER_DISPATCH;
    *provider_binding_context = &_test_provider_binding_context;

    return STATUS_SUCCESS;
}

static NTSTATUS
_test_provider_detach_client(_In_ void* provider_binding_context)
{
    test_provider_binding_context_t* context = (test_provider_binding_context_t*)provider_binding_context;
    REQUIRE(context->allocated);

    _test_provider_binding_context.client_binding_context = nullptr;
    _test_provider_binding_context.client_dispatch = nullptr;

    return (_test_provider_async_deregister) ? STATUS_PENDING : STATUS_SUCCESS;
}

static void
_test_provider_cleanup_binding_context(_In_ void* provider_binding_context)
{
    test_provider_binding_context_t* context = (test_provider_binding_context_t*)provider_binding_context;
    REQUIRE(context->allocated);
    context->allocated = false;
    context->nmr_binding_handle = nullptr;
}

NPI_PROVIDER_CHARACTERISTICS _test_provider_characteristics = {
    .Length = sizeof(NPI_PROVIDER_CHARACTERISTICS),
    .ProviderAttachClient = (PNPI_PROVIDER_ATTACH_CLIENT_FN)_test_provider_attach_client,
    .ProviderDetachClient = _test_provider_detach_client,
    .ProviderCleanupBindingContext = _test_provider_cleanup_binding_context,
    .ProviderRegistrationInstance = _test_provider_registration_instance};

#pragma endregion test_nmr_provider

#pragma region smoke_nmr_client_provider

// Use a distinct NPI ID so smoke registrations never match the test_npiid registrations above,
// preventing cross-contamination when test_npiid registrations are left over from failed tests.
NPIID smoke_npiid = {1};

NPI_REGISTRATION_INSTANCE _smoke_client_registration_instance = {
    .Size = sizeof(NPI_REGISTRATION_INSTANCE), .NpiId = &smoke_npiid};
NPI_REGISTRATION_INSTANCE _smoke_provider_registration_instance = {
    .Size = sizeof(NPI_REGISTRATION_INSTANCE), .NpiId = &smoke_npiid};

static NTSTATUS
_smoke_client_attach_provider(
    _In_ HANDLE nmr_binding_handle,
    _In_opt_ void* client_context,
    _In_ NPI_REGISTRATION_INSTANCE* provider_registration_instance)
{
    UNREFERENCED_PARAMETER(client_context);
    UNREFERENCED_PARAMETER(provider_registration_instance);

    void* provider_binding_context = nullptr;
    const void* provider_dispatch = nullptr;
    return NmrClientAttachProvider(
        nmr_binding_handle,
        reinterpret_cast<void*>(nmr_binding_handle),
        TEST_CLIENT_DISPATCH,
        &provider_binding_context,
        &provider_dispatch);
}

static NTSTATUS
_smoke_client_detach_provider(_In_ void* client_binding_context)
{
    UNREFERENCED_PARAMETER(client_binding_context);
    return STATUS_SUCCESS;
}

static void
_smoke_client_cleanup_binding_context(_In_ void* client_binding_context)
{
    UNREFERENCED_PARAMETER(client_binding_context);
}

NPI_CLIENT_CHARACTERISTICS _smoke_client_characteristics = {
    .Length = sizeof(NPI_CLIENT_CHARACTERISTICS),
    .ClientAttachProvider = (PNPI_CLIENT_ATTACH_PROVIDER_FN)_smoke_client_attach_provider,
    .ClientDetachProvider = _smoke_client_detach_provider,
    .ClientCleanupBindingContext = _smoke_client_cleanup_binding_context,
    .ClientRegistrationInstance = _smoke_client_registration_instance};

static NTSTATUS
_smoke_provider_attach_client(
    _In_ HANDLE nmr_binding_handle,
    _In_opt_ void* provider_context,
    _In_ NPI_REGISTRATION_INSTANCE* client_registration_instance,
    _In_ void* client_binding_context,
    _In_ const void* client_dispatch,
    _Outptr_ void** provider_binding_context,
    _Outptr_ const void** provider_dispatch)
{
    UNREFERENCED_PARAMETER(nmr_binding_handle);
    UNREFERENCED_PARAMETER(provider_context);
    UNREFERENCED_PARAMETER(client_registration_instance);
    UNREFERENCED_PARAMETER(client_dispatch);

    *provider_binding_context = client_binding_context;
    *provider_dispatch = TEST_PROVIDER_DISPATCH;
    return STATUS_SUCCESS;
}

static NTSTATUS
_smoke_provider_detach_client(_In_ void* provider_binding_context)
{
    UNREFERENCED_PARAMETER(provider_binding_context);
    return STATUS_SUCCESS;
}

static void
_smoke_provider_cleanup_binding_context(_In_ void* provider_binding_context)
{
    UNREFERENCED_PARAMETER(provider_binding_context);
}

NPI_PROVIDER_CHARACTERISTICS _smoke_provider_characteristics = {
    .Length = sizeof(NPI_PROVIDER_CHARACTERISTICS),
    .ProviderAttachClient = (PNPI_PROVIDER_ATTACH_CLIENT_FN)_smoke_provider_attach_client,
    .ProviderDetachClient = _smoke_provider_detach_client,
    .ProviderCleanupBindingContext = _smoke_provider_cleanup_binding_context,
    .ProviderRegistrationInstance = _smoke_provider_registration_instance};

#pragma endregion smoke_nmr_client_provider

TEST_CASE("NmrRegisterClient", "[nmr]")
{
    HANDLE nmr_client_handle;

    REQUIRE(NmrRegisterClient(&_test_client_characteristics, nullptr, &nmr_client_handle) == STATUS_SUCCESS);

    // Verify there was no binding callback, since there are no providers.
    REQUIRE(_test_client_binding_context.allocated == false);

    REQUIRE(NmrDeregisterClient(nmr_client_handle) == STATUS_PENDING);
    REQUIRE(NmrWaitForClientDeregisterComplete(nmr_client_handle) == STATUS_SUCCESS);
}

TEST_CASE("NmrRegisterProvider", "[nmr]")
{
    HANDLE nmr_provider_handle;

    REQUIRE(NmrRegisterProvider(&_test_provider_characteristics, nullptr, &nmr_provider_handle) == STATUS_SUCCESS);

    // Verify there was no binding callback, since there are no clients.
    REQUIRE(_test_provider_binding_context.allocated == false);

    REQUIRE(NmrDeregisterProvider(nmr_provider_handle) == STATUS_PENDING);
    REQUIRE(NmrWaitForProviderDeregisterComplete(nmr_provider_handle) == STATUS_SUCCESS);
}

TEST_CASE("attach during NmrRegisterProvider", "[nmr]")
{
    HANDLE nmr_client_handle;
    REQUIRE(NmrRegisterClient(&_test_client_characteristics, nullptr, &nmr_client_handle) == STATUS_SUCCESS);

    // Verify there was no binding callback, since there are no providers.
    REQUIRE(_test_client_binding_context.allocated == false);

    HANDLE nmr_provider_handle;
    REQUIRE(NmrRegisterProvider(&_test_provider_characteristics, nullptr, &nmr_provider_handle) == STATUS_SUCCESS);

    REQUIRE(_test_client_binding_context.allocated == true);
    REQUIRE(_test_client_binding_context.nmr_binding_handle != nullptr);
    REQUIRE(_test_client_binding_context.provider_binding_context != nullptr);
    REQUIRE(_test_client_binding_context.provider_dispatch == TEST_PROVIDER_DISPATCH);

    REQUIRE(_test_provider_binding_context.allocated == true);
    REQUIRE(_test_provider_binding_context.nmr_binding_handle != nullptr);
    REQUIRE(_test_provider_binding_context.client_binding_context != nullptr);
    REQUIRE(_test_provider_binding_context.client_dispatch == TEST_CLIENT_DISPATCH);

    // Deregister the provider first.
    REQUIRE(NmrDeregisterProvider(nmr_provider_handle) == STATUS_PENDING);
    REQUIRE(NmrWaitForProviderDeregisterComplete(nmr_provider_handle) == STATUS_SUCCESS);

    REQUIRE(_test_client_binding_context.allocated == false);
    REQUIRE(_test_client_binding_context.nmr_binding_handle == nullptr);
    REQUIRE(_test_client_binding_context.provider_binding_context == nullptr);
    REQUIRE(_test_client_binding_context.provider_dispatch == nullptr);

    REQUIRE(_test_provider_binding_context.allocated == false);
    REQUIRE(_test_provider_binding_context.nmr_binding_handle == nullptr);
    REQUIRE(_test_provider_binding_context.client_binding_context == nullptr);
    REQUIRE(_test_provider_binding_context.client_dispatch == nullptr);

    REQUIRE(NmrDeregisterClient(nmr_client_handle) == STATUS_PENDING);
    REQUIRE(NmrWaitForClientDeregisterComplete(nmr_client_handle) == STATUS_SUCCESS);
}

TEST_CASE("attach during NmrRegisterClient", "[nmr]")
{
    HANDLE nmr_provider_handle;
    REQUIRE(NmrRegisterProvider(&_test_provider_characteristics, nullptr, &nmr_provider_handle) == STATUS_SUCCESS);

    // Verify there was no binding callback, since there are no clients.
    REQUIRE(_test_provider_binding_context.allocated == false);

    HANDLE nmr_client_handle;
    REQUIRE(NmrRegisterClient(&_test_client_characteristics, nullptr, &nmr_client_handle) == STATUS_SUCCESS);

    REQUIRE(_test_client_binding_context.allocated == true);
    REQUIRE(_test_client_binding_context.nmr_binding_handle != nullptr);
    REQUIRE(_test_client_binding_context.provider_binding_context != nullptr);
    REQUIRE(_test_client_binding_context.provider_dispatch == TEST_PROVIDER_DISPATCH);

    REQUIRE(_test_provider_binding_context.allocated == true);
    REQUIRE(_test_provider_binding_context.nmr_binding_handle != nullptr);
    REQUIRE(_test_provider_binding_context.client_binding_context != nullptr);
    REQUIRE(_test_provider_binding_context.client_dispatch == TEST_CLIENT_DISPATCH);

    // Deregister the client first.
    REQUIRE(NmrDeregisterClient(nmr_client_handle) == STATUS_PENDING);
    REQUIRE(NmrWaitForClientDeregisterComplete(nmr_client_handle) == STATUS_SUCCESS);

    REQUIRE(_test_client_binding_context.allocated == false);
    REQUIRE(_test_client_binding_context.nmr_binding_handle == nullptr);
    REQUIRE(_test_client_binding_context.provider_binding_context == nullptr);
    REQUIRE(_test_client_binding_context.provider_dispatch == nullptr);

    REQUIRE(_test_provider_binding_context.allocated == false);
    REQUIRE(_test_provider_binding_context.nmr_binding_handle == nullptr);
    REQUIRE(_test_provider_binding_context.client_binding_context == nullptr);
    REQUIRE(_test_provider_binding_context.client_dispatch == nullptr);

    REQUIRE(NmrDeregisterProvider(nmr_provider_handle) == STATUS_PENDING);
    REQUIRE(NmrWaitForProviderDeregisterComplete(nmr_provider_handle) == STATUS_SUCCESS);
}

TEST_CASE("NmrRegisterClient with async deregister", "[nmr]")
{
    HANDLE nmr_client_handle;
    REQUIRE(NmrRegisterClient(&_test_client_characteristics, nullptr, &nmr_client_handle) == STATUS_SUCCESS);
    HANDLE nmr_provider_handle;
    REQUIRE(NmrRegisterProvider(&_test_provider_characteristics, nullptr, &nmr_provider_handle) == STATUS_SUCCESS);

    // Start an asynchronous deregister, as if calls were in progress.
    _test_client_async_deregister = true;
    REQUIRE(NmrDeregisterClient(nmr_client_handle) == STATUS_PENDING);

    // Verify that the binding still exists but no further calls will be initiated.
    REQUIRE(_test_client_binding_context.allocated == true);
    REQUIRE(_test_client_binding_context.nmr_binding_handle != nullptr);
    REQUIRE(_test_client_binding_context.provider_binding_context == nullptr);
    REQUIRE(_test_client_binding_context.provider_dispatch == nullptr);

    REQUIRE(_test_provider_binding_context.allocated == true);
    REQUIRE(_test_provider_binding_context.nmr_binding_handle != nullptr);
    REQUIRE(_test_provider_binding_context.client_binding_context == nullptr);
    REQUIRE(_test_provider_binding_context.client_dispatch == nullptr);

    // Complete the detach.
    NmrClientDetachProviderComplete(_test_client_binding_context.nmr_binding_handle);
    REQUIRE(NmrWaitForClientDeregisterComplete(nmr_client_handle) == STATUS_SUCCESS);
    _test_client_async_deregister = false;

    // The binding should no longer exist.
    REQUIRE(_test_client_binding_context.allocated == false);
    REQUIRE(_test_client_binding_context.nmr_binding_handle == nullptr);
    REQUIRE(_test_provider_binding_context.allocated == false);
    REQUIRE(_test_provider_binding_context.nmr_binding_handle == nullptr);

    REQUIRE(NmrDeregisterProvider(nmr_provider_handle) == STATUS_PENDING);
    REQUIRE(NmrWaitForProviderDeregisterComplete(nmr_provider_handle) == STATUS_SUCCESS);
}

TEST_CASE("NmrRegisterProvider with async deregister", "[nmr]")
{
    HANDLE nmr_client_handle;
    REQUIRE(NmrRegisterClient(&_test_client_characteristics, nullptr, &nmr_client_handle) == STATUS_SUCCESS);
    HANDLE nmr_provider_handle;
    REQUIRE(NmrRegisterProvider(&_test_provider_characteristics, nullptr, &nmr_provider_handle) == STATUS_SUCCESS);

    // Start an asynchronous deregister, as if calls were in progress.
    _test_provider_async_deregister = true;
    REQUIRE(NmrDeregisterProvider(nmr_provider_handle) == STATUS_PENDING);

    // Verify that the binding still exists but no further calls will be initiated.
    REQUIRE(_test_client_binding_context.allocated == true);
    REQUIRE(_test_client_binding_context.nmr_binding_handle != nullptr);
    REQUIRE(_test_client_binding_context.provider_binding_context == nullptr);
    REQUIRE(_test_client_binding_context.provider_dispatch == nullptr);

    REQUIRE(_test_provider_binding_context.allocated == true);
    REQUIRE(_test_provider_binding_context.nmr_binding_handle != nullptr);
    REQUIRE(_test_provider_binding_context.client_binding_context == nullptr);
    REQUIRE(_test_provider_binding_context.client_dispatch == nullptr);

    // Complete the detach.
    NmrProviderDetachClientComplete(_test_provider_binding_context.nmr_binding_handle);
    REQUIRE(NmrWaitForProviderDeregisterComplete(nmr_provider_handle) == STATUS_SUCCESS);
    _test_provider_async_deregister = false;

    // The binding should no longer exist.
    REQUIRE(_test_client_binding_context.allocated == false);
    REQUIRE(_test_client_binding_context.nmr_binding_handle == nullptr);
    REQUIRE(_test_provider_binding_context.allocated == false);
    REQUIRE(_test_provider_binding_context.nmr_binding_handle == nullptr);

    REQUIRE(NmrDeregisterClient(nmr_client_handle) == STATUS_PENDING);
    REQUIRE(NmrWaitForClientDeregisterComplete(nmr_client_handle) == STATUS_SUCCESS);
}

TEST_CASE("concurrent register/deregister smoke", "[nmr][no_fi]")
{
    constexpr auto run_duration = std::chrono::seconds(30);
    const auto test_start = std::chrono::steady_clock::now();
    std::atomic<size_t> ready_threads{0};
    std::atomic<bool> start{false};

    std::thread provider_thread([&ready_threads, &start, run_duration]() {
        ready_threads++;
        while (!start.load(std::memory_order_acquire)) {
            std::this_thread::yield();
        }
        const auto deadline = std::chrono::steady_clock::now() + run_duration;
        while (std::chrono::steady_clock::now() < deadline) {
            HANDLE nmr_provider_handle = nullptr;
            NTSTATUS register_status =
                NmrRegisterProvider(&_smoke_provider_characteristics, nullptr, &nmr_provider_handle);
            if (!NT_SUCCESS(register_status)) {
                continue;
            }

            NTSTATUS deregister_status = NmrDeregisterProvider(nmr_provider_handle);
            if (deregister_status == STATUS_PENDING) {
                (void)NmrWaitForProviderDeregisterComplete(nmr_provider_handle);
            }
        }
    });

    std::thread client_thread([&ready_threads, &start, run_duration]() {
        ready_threads++;
        while (!start.load(std::memory_order_acquire)) {
            std::this_thread::yield();
        }
        const auto deadline = std::chrono::steady_clock::now() + run_duration;
        while (std::chrono::steady_clock::now() < deadline) {
            HANDLE nmr_client_handle = nullptr;
            NTSTATUS register_status = NmrRegisterClient(&_smoke_client_characteristics, nullptr, &nmr_client_handle);
            if (!NT_SUCCESS(register_status)) {
                continue;
            }

            NTSTATUS deregister_status = NmrDeregisterClient(nmr_client_handle);
            if (deregister_status == STATUS_PENDING) {
                (void)NmrWaitForClientDeregisterComplete(nmr_client_handle);
            }
        }
    });

    while (ready_threads.load(std::memory_order_acquire) != 2) {
        std::this_thread::yield();
    }
    start.store(true, std::memory_order_release);

    provider_thread.join();
    client_thread.join();

    const auto elapsed =
        std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - test_start);
    std::cout << "smoke test duration: " << elapsed.count() << " ms" << std::endl;
}