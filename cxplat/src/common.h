// Copyright (c) Microsoft Corporation
// SPDX-License-Identifier: MIT

#pragma once
#include <sal.h>

#ifdef CXPLAT_SOURCE
#define CXPLAT_API __declspec(dllexport)
#else
#define CXPLAT_API __declspec(dllimport)
#endif
