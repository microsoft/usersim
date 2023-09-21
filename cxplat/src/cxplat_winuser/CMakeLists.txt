# Copyright (c) Microsoft Corporation
# SPDX-License-Identifier: MIT

add_library(cxplat_winuser STATIC
  ../../inc/cxplat.h
  ../../inc/cxplat_common.h
  ../../inc/cxplat_memory.h
  ../../inc/cxplat_rundown.h
  ../../inc/cxplat_workitem.h
  ../../inc/winuser/cxplat_fault_injection.h
  ../../inc/winuser/cxplat_platform.h
  ../../inc/winuser/cxplat_winuser.h
  cxplat_winuser.cpp
  $<$<CONFIG:Debug>:fault_injection.cpp>
  $<$<CONFIG:Debug>:leak_detector.cpp>
  $<$<CONFIG:Debug>:leak_detector.h>
  ../memory.c
  memory_winuser.cpp
  rundown_winuser.cpp
  size_winuser.c
  workitem_winuser.cpp
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
