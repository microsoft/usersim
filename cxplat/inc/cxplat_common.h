// Copyright (c) Microsoft Corporation
// SPDX-License-Identifier: MIT

#pragma once
#include <assert.h>

#define CXPLAT_RUNTIME_ASSERT(x) assert(x)
#ifdef _DEBUG
#define CXPLAT_DEBUG_ASSERT(x) assert(x)
#else
#define CXPLAT_DEBUG_ASSERT(x) (void)(x)
#endif //!_DEBUG
