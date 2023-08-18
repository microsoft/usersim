// Copyright (c) Microsoft Corporation
// SPDX-License-Identifier: MIT

// This file contains initialization/cleanup routines for the Windows user-mode cxplat library.
#include "cxplat/fault_injection.h"
#include "leak_detector.h"
#include "symbol_decoder.h"
#include <algorithm>
#include <memory>
#include <string>

extern "C" bool cxplat_fuzzing_enabled = false;

cxplat_leak_detector_ptr _cxplat_leak_detector_ptr;

/**
 * @brief Environment variable to enable fault injection testing.
 * TODO: update names from USERSIM to CXPLAT
 */
#define CXPLAT_FAULT_INJECTION_SIMULATION_ENVIRONMENT_VARIABLE_NAME "USERSIM_FAULT_INJECTION_SIMULATION"
#define CXPLAT_MEMORY_LEAK_DETECTION_ENVIRONMENT_VARIABLE_NAME "USERSIM_MEMORY_LEAK_DETECTION"

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

int
cxplat_initialize()
{
    try {
        auto fault_injection_stack_depth =
            _get_environment_variable_as_size_t(CXPLAT_FAULT_INJECTION_SIMULATION_ENVIRONMENT_VARIABLE_NAME);
        auto leak_detector = _get_environment_variable_as_bool(CXPLAT_MEMORY_LEAK_DETECTION_ENVIRONMENT_VARIABLE_NAME);

               if (fault_injection_stack_depth || leak_detector) {
            _cxplat_symbol_decoder_initialize();
        }
        if (fault_injection_stack_depth && !cxplat_fault_injection_is_enabled()) {
            if (cxplat_fault_injection_initialize(fault_injection_stack_depth) != 0) {
                return STATUS_NO_MEMORY;
            }
            // Set flag to remove some asserts that fire from incorrect client behavior.
            cxplat_fuzzing_enabled = true;
        }

        if (leak_detector) {
            _cxplat_leak_detector_ptr = std::make_unique<cxplat_leak_detector_t>();
        }
    } catch (const std::bad_alloc&) {
        return ENOMEM;
    }
    return 0;
}

void
cxplat_cleanup()
{
    if (_cxplat_leak_detector_ptr) {
        _cxplat_leak_detector_ptr->dump_leaks();
        _cxplat_leak_detector_ptr.reset();
    }
}