# Copyright (c) Microsoft Corporation
# SPDX-License-Identifier: MIT

add_library(sample SHARED
  driver.c
)

target_include_directories(sample PRIVATE
  "${WDK_ROOT}/include/${WDK_VERSION}/km"
  "${WDK_ROOT}/include/wdf/kmdf/1.15"
)

target_compile_definitions(sample PRIVATE
  _WINDLL
  _AMD64_
)

target_link_libraries(sample PRIVATE
  "usersim.lib"
  "usersim_dll_skeleton.lib"
)

target_link_directories(sample PRIVATE
  "${CMAKE_BINARY_DIR}/lib/${CONFIG}"
)

add_dependencies(sample usersim usersim_dll_skeleton)
