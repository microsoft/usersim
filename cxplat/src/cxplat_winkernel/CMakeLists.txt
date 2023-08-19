# Copyright (c) Microsoft Corporation
# SPDX-License-Identifier: MIT

add_library(cxplat_winkernel STATIC
  ../../inc/cxplat.h
  ../../inc/cxplat_common.h
  ../../inc/cxplat_fault_injection.h
  ../../inc/cxplat_memory.h
  ../../inc/winkernel/cxplat_platform.h
  ../../inc/winkernel/cxplat_winkernel.h
  ../memory.c
  memory_winkernel.c
)

target_include_directories(cxplat_winkernel PRIVATE
  "."
  "../../inc"
  "../../inc/winkernel"
  "${WDK_ROOT}/include/${WDK_VERSION}/km"
)

set(defs UNICODE _UNICODE CXPLAT_SOURCE)
target_compile_definitions(cxplat_winkernel PRIVATE ${defs})
