# Copyright (c) Microsoft Corporation
# SPDX-License-Identifier: MIT

add_library(usersim_dll_skeleton STATIC
  dll_skeleton.c
)

target_include_directories(usersim_dll_skeleton PRIVATE
  "${CMAKE_CURRENT_SOURCE_DIR}/../inc"
  "${CMAKE_CURRENT_SOURCE_DIR}/../cxplat/inc"
  "${CMAKE_CURRENT_SOURCE_DIR}/../cxplat/inc/winuser"
  "${WDK_ROOT}/include/${WDK_VERSION}/km"
  "${WindowsSdkDir}include/10.0.22621.0"
)
