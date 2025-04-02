// Copyright (c) Microsoft Corporation
// SPDX-License-Identifier: MIT
#pragma once

#include <stdint.h>

CXPLAT_EXTERN_C_BEGIN

/**
 * @brief Get the path to the module that contains the specified address.
 *
 * @param[in] address The address to look up.
 * @param[out] path The buffer to receive the module path.
 * @param[in] path_length The length of the buffer.
 * @param[out] path_length_out The length of the path written to the buffer.
 * @retval CXPLAT_STATUS_SUCCESS if the path was successfully retrieved.
 * @retval CXPLAT_STATUS_NO_MEMORY if unable to allocate memory for the module information.
 * @retval CXPLAT_STATUS_NOT_FOUND if the module information could not be found.
 */
cxplat_status_t
cxplat_get_module_path_from_address(
    _In_ const void* address,
    _Out_writes_z_(path_length) char* path,
    _In_ size_t path_length,
    _Out_opt_ size_t* path_length_out);

CXPLAT_EXTERN_C_END
