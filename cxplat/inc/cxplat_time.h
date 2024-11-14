// Copyright (c) Microsoft Corporation
// SPDX-License-Identifier: MIT
#pragma once

#include <stdint.h>

CXPLAT_EXTERN_C_BEGIN

uint64_t
cxplat_query_time_since_boot_precise(bool include_suspended_time);

uint64_t
cxplat_query_time_since_boot_approximate(bool include_suspended_time);

CXPLAT_EXTERN_C_END
