# Copyright (c) Microsoft Corporation
# SPDX-License-Identifier: MIT

add_library(sample SHARED
  driver.c
)

target_include_directories(sample PRIVATE
  "../inc"
  "${WDK_ROOT}/include/${WDK_VERSION}/km"
)

target_compile_definitions(sample PRIVATE
  _WINDLL
)

target_link_libraries(sample PRIVATE
  "usersim.lib"
)

target_link_directories(sample PRIVATE
  "${CMAKE_BINARY_DIR}/lib/${CONFIG}"
)

add_dependencies(sample usersim)
