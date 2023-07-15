// Copyright (c) Microsoft Corporation
// SPDX-License-Identifier: MIT

#pragma once

#ifdef USERSIM_SOURCE
#define USERSIM_API __declspec(dllexport)
#else
#define USERSIM_API __declspec(dllimport)
#endif
