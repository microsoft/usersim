# Copyright (c) Microsoft Corporation
# SPDX-License-Identifier: MIT

add_library(usersim SHARED
  dllmain.cpp
  etw.cpp
  ex.cpp
  framework.h
  fwp_um.cpp
  fwp_um.h
  io.cpp
  ke.cpp
  kernel_um.cpp
  kernel_um.h
  mm.cpp
  ndis_um.cpp
  ndis.h
  net_platform.h
  nmr_impl.cpp
  nmr_impl.h
  nmr_um.cpp
  ob.cpp
  platform.h
  platform_user.cpp
  ps.cpp
  rtl.cpp
  se.cpp
  tracelog.c
  tracelog.h
  utilities.h
  wdf.cpp
  zw.cpp
  Source.def
)

target_include_directories(usersim PRIVATE
  "."
  "${CMAKE_CURRENT_SOURCE_DIR}/../inc"
  "${CMAKE_CURRENT_SOURCE_DIR}/../cxplat/inc"
  "${CMAKE_CURRENT_SOURCE_DIR}/../cxplat/inc/winuser"
  "${WDK_ROOT}/include/${WDK_VERSION}/km"
  "${WindowsSdkDir}include/10.0.22621.0"
)

target_link_directories(usersim PRIVATE
  "${CMAKE_BINARY_DIR}/lib/${CONFIG}"
)

target_link_libraries(usersim PRIVATE
  "cxplat_winuser.lib"
)

set(defs UNICODE _UNICODE USERSIM_SOURCE)
target_compile_definitions(usersim PRIVATE ${defs})

add_dependencies(usersim cxplat_winuser)
