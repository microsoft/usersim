# Copyright (c) Microsoft Corporation
# SPDX-License-Identifier: MIT

add_library(cxplat_winuser STATIC
  ../../inc/cxplat.h
  ../../inc/cxplat_common.h
  ../../inc/cxplat_fault_injection.h
  ../../inc/cxplat_memory.h
  ../../inc/winuser/cxplat_platform.h
  cxplat_winuser.cpp
  ../../inc/winuser/cxplat_winuser.h
  fault_injection.cpp
  leak_detector.cpp
  leak_detector.h
  memory.cpp
  symbol_decoder.h
  winuser_internal.h
)

target_include_directories(cxplat_winuser PRIVATE
  "."
  "../../inc"
  "../../inc/winuser"
  "${WindowsSdkDir}include/10.0.22621.0"
)

set(defs UNICODE _UNICODE CXPLAT_SOURCE)
target_compile_definitions(cxplat_winuser PRIVATE ${defs})
