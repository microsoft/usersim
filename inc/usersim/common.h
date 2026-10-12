// Copyright (c) Microsoft Corporation
// SPDX-License-Identifier: MIT

#pragma once

#ifdef USERSIM_SOURCE
#define USERSIM_API __declspec(dllexport)
#else
#define USERSIM_API __declspec(dllimport)
#endif

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * @brief Suspend fault injection within usersim.dll.
 *
 * Calls may be nested and must be balanced by calls to usersim_fault_injection_resume().
 */
USERSIM_API void
usersim_fault_injection_suspend();

/**
 * @brief Resume fault injection within usersim.dll after a matching suspension.
 */
USERSIM_API void
usersim_fault_injection_resume();

#ifdef __cplusplus
}
#endif
