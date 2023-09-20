// Copyright (c) Microsoft Corporation
// SPDX-License-Identifier: MIT

// This file contains initialization/cleanup routines for the Windows user-mode cxplat library.
#include "cxplat.h"
#include "cxplat_fault_injection.h"
#include "leak_detector.h"
#include "symbol_decoder.h"

#include <algorithm>
#include <memory>
#include <psapi.h>
#include <string>

#ifndef NDEBUG
extern "C" bool cxplat_fuzzing_enabled = false;

cxplat_leak_detector_ptr _cxplat_leak_detector_ptr;
#endif

/**
 * @brief Environment variable to enable fault injection testing.
 */
#define CXPLAT_FAULT_INJECTION_SIMULATION_ENVIRONMENT_VARIABLE_NAME "CXPLAT_FAULT_INJECTION_SIMULATION"
#define CXPLAT_MEMORY_LEAK_DETECTION_ENVIRONMENT_VARIABLE_NAME "CXPLAT_MEMORY_LEAK_DETECTION"

/**
 * @brief Get an environment variable as a string.
 *
 * @param[in] name Environment variable name.
 * @return String value of environment variable or an empty string if not set.
 */
static std::string
_get_environment_variable_as_string(const std::string& name)
{
    std::string value;
    size_t required_size = 0;
    getenv_s(&required_size, nullptr, 0, name.c_str());
    if (required_size > 0) {
        value.resize(required_size);
        getenv_s(&required_size, &value[0], required_size, name.c_str());
        value.resize(required_size - 1);
    }
    return value;
}

/**
 * @brief Get an environment variable as a boolean.
 *
 * @param[in] name Environment variable name.
 * @retval false Environment variable is set to "false", "0", or if it's not set.
 * @retval true Environment variable is set to any other value.
 */
static bool
_get_environment_variable_as_bool(const std::string& name)
{
    std::string value = _get_environment_variable_as_string(name);
    if (value.empty()) {
        return false;
    }

    // Convert value to lower case.
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) { return (char)std::tolower(c); });
    if (value == "false") {
        return false;
    }
    if (value == "0") {
        return false;
    }
    return true;
}

/**
 * @brief Get an environment variable as a size_t.
 *
 * @param[in] name Environment variable name.
 * @return Value of environment variable or 0 if it's not set or not a valid number.
 */
static size_t
_get_environment_variable_as_size_t(const std::string& name)
{
    std::string value = _get_environment_variable_as_string(name);
    if (value.empty()) {
        return 0;
    }
    try {
        return std::stoull(value);
    } catch (const std::exception&) {
        return 0;
    }
}

static std::mutex cxplat_initialization_mutex;
static ULONG _cxplat_initialization_count = 0;

inline static HMODULE
_cxplat_get_caller_module()
{
    // Capture the caller's module handle and address.
    uintptr_t caller_address;
    unsigned long caller_address_hash;
    HMODULE module_handle;
    if (CaptureStackBackTrace(1, 1, (void**)&caller_address, &caller_address_hash) == 0) {
        return NULL;
    }
    if (!GetModuleHandleEx(
            GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            reinterpret_cast<LPCTSTR>(caller_address),
            &module_handle)) {
        return NULL;
    }
    return module_handle;
}

cxplat_status_t
cxplat_initialize()
{

    std::unique_lock lock(cxplat_initialization_mutex);
    if (_cxplat_initialization_count > 0) {
#ifndef NDEBUG
        if (cxplat_fault_injection_is_enabled()) {
            if (cxplat_fault_injection_add_module(_cxplat_get_caller_module()) != 0) {
                return CXPLAT_STATUS_NO_MEMORY;
            }
        }
#endif

        // Already initialized.
        _cxplat_initialization_count++;
        return CXPLAT_STATUS_SUCCESS;
    }

    try {
#ifndef NDEBUG
        auto fault_injection_stack_depth =
            _get_environment_variable_as_size_t(CXPLAT_FAULT_INJECTION_SIMULATION_ENVIRONMENT_VARIABLE_NAME);
        auto leak_detector = _get_environment_variable_as_bool(CXPLAT_MEMORY_LEAK_DETECTION_ENVIRONMENT_VARIABLE_NAME);

        if (fault_injection_stack_depth || leak_detector) {
            cxplat_status_t status = _cxplat_symbol_decoder_initialize();
            if (!CXPLAT_SUCCEEDED(status)) {
                return status;
            }
        }
        if (fault_injection_stack_depth && !cxplat_fault_injection_is_enabled()) {
            if (cxplat_fault_injection_initialize(fault_injection_stack_depth) != 0) {
                return CXPLAT_STATUS_NO_MEMORY;
            }

            if (cxplat_fault_injection_add_module(_cxplat_get_caller_module()) != 0) {
                return CXPLAT_STATUS_NO_MEMORY;
            }
            // Set flag to remove some asserts that fire from incorrect client behavior.
            cxplat_fuzzing_enabled = true;
        }

        if (leak_detector) {
            _cxplat_leak_detector_ptr = std::make_unique<cxplat_leak_detector_t>();
        }
#endif

        cxplat_status_t status = cxplat_winuser_initialize_thread_pool();
        if (!CXPLAT_SUCCEEDED(status)) {
            return status;
        }
    } catch (const std::bad_alloc&) {
        return CXPLAT_STATUS_NO_MEMORY;
    }
    _cxplat_initialization_count++;
    return CXPLAT_STATUS_SUCCESS;
}

void
cxplat_cleanup()
{
    std::unique_lock lock(cxplat_initialization_mutex);
    CXPLAT_RUNTIME_ASSERT(_cxplat_initialization_count > 0);
    _cxplat_initialization_count--;
    if (_cxplat_initialization_count > 0) {
#ifndef NDEBUG
        cxplat_fault_injection_remove_module(_cxplat_get_caller_module());
#endif
        // Don't clean up until the count hits 0.
        return;
    }

    cxplat_winuser_clean_up_thread_pool();

#ifndef NDEBUG
    if (_cxplat_leak_detector_ptr) {
        _cxplat_leak_detector_ptr->dump_leaks();
        _cxplat_leak_detector_ptr.reset();
    }
    _cxplat_symbol_decoder_deinitialize();
#endif
}
