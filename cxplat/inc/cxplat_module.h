// Copyright (c) Microsoft Corporation
// SPDX-License-Identifier: MIT
#pragma once

#include <stdint.h>

CXPLAT_EXTERN_C_BEGIN

/**
 * @brief Get the path to the module that contains the specified address.
 *
 * @param[in] address The address to look up.
 * @param[out] path The path to the module that contains the address.
 * @retval CXPLAT_STATUS_SUCCESS if the path was successfully retrieved.
 * @retval CXPLAT_STATUS_NO_MEMORY if unable to allocate memory for the module information.
 * @retval CXPLAT_STATUS_NOT_FOUND if the module information could not be found.
 */
cxplat_status_t
cxplat_get_module_path_from_address(_In_ const void* address, _Out_ cxplat_utf8_string_t* path);

CXPLAT_EXTERN_C_END
