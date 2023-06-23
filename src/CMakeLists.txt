# Copyright (c) Microsoft Corporation
# SPDX-License-Identifier: MIT

add_library(usersim STATIC
  dll_skeleton.c
  ex.cpp
  fault_injection.cpp
  fault_injection.h
  framework.h
  fwp_um.cpp
  fwp_um.h
  io.cpp
  ke.cpp
  kernel_um.cpp
  kernel_um.h
  leak_detector.cpp
  leak_detector.h
  mm.cpp
  ndis_um.cpp
  net_platform.h
  nmr_impl.cpp
  nmr_impl.h
  nmr_um.cpp
  platform.h
  platform_user.cpp
  ps.cpp
  rtl.cpp
  se.cpp
  symbol_decoder.h
  tracelog.c
  tracelog.h
  utilities.h
  wdf.cpp
)

target_include_directories(usersim PRIVATE
  "../inc"
  "${WDK_ROOT}/include/${WDK_VERSION}/km"
)

set(defs UNICODE _UNICODE)
target_compile_definitions(usersim PRIVATE ${defs})
