# Copyright (c) Microsoft Corporation
# SPDX-License-Identifier: MIT

add_library(usersim_dll_skeleton STATIC
  dll_skeleton.c
)

target_include_directories(usersim_dll_skeleton PRIVATE
  "../inc"
  "${WDK_ROOT}/include/${WDK_VERSION}/km"
  "${WindowsSdkDir}include/10.0.22621.0"
)
