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

    FreeLibrary(module);
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

    typedef NTSTATUS(FN_WdfDriverCreate)(
        _In_ WDF_DRIVER_GLOBALS* driver_globals,
        _In_ PDRIVER_OBJECT driver_object,
        _In_ PCUNICODE_STRING registry_path,
        _In_opt_ PWDF_OBJECT_ATTRIBUTES driver_attributes,
        _In_ PWDF_DRIVER_CONFIG driver_config,
        _Out_opt_ WDFDRIVER* driver);
    FN_WdfDriverCreate* WdfDriverCreate = (FN_WdfDriverCreate*)UsersimWdfFunctions[WdfDriverCreateTableIndex];
    WDFDRIVER driver;
    NTSTATUS status = WdfDriverCreate(UsersimWdfDriverGlobals, &driver_object, nullptr, nullptr, &config, &driver);
    REQUIRE(status == STATUS_SUCCESS);
    REQUIRE(driver == &driver_object);
    UsersimWdfDriverGlobals->Driver = nullptr;
}
