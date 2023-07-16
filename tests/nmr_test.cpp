// Copyright (c) Microsoft Corporation
// SPDX-License-Identifier: MIT

#if !defined(CMAKE_NUGET)
#include <catch2/catch_all.hpp>
#else
#include <catch2/catch.hpp>
#endif
#include <ntddk.h>
#include <../km/netioddk.h>

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

    return STATUS_SUCCESS;
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

    return STATUS_SUCCESS;
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

TEST_CASE("NmrRegisterClient", "[nmr]")
{
    HANDLE nmr_client_handle;
    
    REQUIRE(NmrRegisterClient(&_test_client_characteristics, nullptr, &nmr_client_handle) == STATUS_SUCCESS);

    // Verify there was no binding callback, since there are no providers.
    REQUIRE(_test_client_binding_context.allocated == false);

    REQUIRE(NmrDeregisterClient(nmr_client_handle) == STATUS_SUCCESS);

    // TODO: test async detach
    //REQUIRE(NmrWaitForClientDeregisterComplete(nmr_client_handle) == STATUS_SUCCESS);
}

TEST_CASE("NmrRegisterProvider", "[nmr]")
{
    HANDLE nmr_provider_handle;

    REQUIRE(NmrRegisterProvider(&_test_provider_characteristics, nullptr, &nmr_provider_handle) == STATUS_SUCCESS);

    // Verify there was no binding callback, since there are no clients.
    REQUIRE(_test_provider_binding_context.allocated == false);

    REQUIRE(NmrDeregisterProvider(nmr_provider_handle) == STATUS_SUCCESS);

    // TODO: test async detach
    //REQUIRE(NmrWaitForProviderDeregisterComplete(nmr_provider_handle) == STATUS_SUCCESS);
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
    REQUIRE(NmrDeregisterProvider(nmr_provider_handle) == STATUS_SUCCESS);

    REQUIRE(_test_client_binding_context.allocated == false);
    REQUIRE(_test_client_binding_context.nmr_binding_handle == nullptr);
    REQUIRE(_test_client_binding_context.provider_binding_context == nullptr);
    REQUIRE(_test_client_binding_context.provider_dispatch == nullptr);

    REQUIRE(_test_provider_binding_context.allocated == false);
    REQUIRE(_test_provider_binding_context.nmr_binding_handle == nullptr);
    REQUIRE(_test_provider_binding_context.client_binding_context == nullptr);
    REQUIRE(_test_provider_binding_context.client_dispatch == nullptr);

    REQUIRE(NmrDeregisterClient(nmr_client_handle) == STATUS_SUCCESS);
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
    REQUIRE(NmrDeregisterClient(nmr_client_handle) == STATUS_SUCCESS);

    REQUIRE(_test_client_binding_context.allocated == false);
    REQUIRE(_test_client_binding_context.nmr_binding_handle == nullptr);
    REQUIRE(_test_client_binding_context.provider_binding_context == nullptr);
    REQUIRE(_test_client_binding_context.provider_dispatch == nullptr);

    REQUIRE(_test_provider_binding_context.allocated == false);
    REQUIRE(_test_provider_binding_context.nmr_binding_handle == nullptr);
    REQUIRE(_test_provider_binding_context.client_binding_context == nullptr);
    REQUIRE(_test_provider_binding_context.client_dispatch == nullptr);

    REQUIRE(NmrDeregisterProvider(nmr_provider_handle) == STATUS_SUCCESS);
}
