# Copyright (c) Microsoft Corporation
# SPDX-License-Identifier: MIT

Include(FetchContent)

FetchContent_Declare(
  Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG        v3.0.1 # or a later release
)

FetchContent_MakeAvailable(Catch2)

add_executable(usersim_tests
  etw_test.cpp
  ex_test.cpp
  ke_test.cpp
  mm_test.cpp
  nmr_test.cpp
  ob_test.cpp
  ps_test.cpp
  rtl_test.cpp
  se_test.cpp
  wdf_test.cpp
)

target_include_directories(usersim_tests PRIVATE
  "${CMAKE_BINARY_DIR}/generated-includes/"
  "${CMAKE_BINARY_DIR}/_deps/catch2-src/src/"
  "${CMAKE_CURRENT_SOURCE_DIR}/../inc/"
  "${CMAKE_CURRENT_SOURCE_DIR}/../cxplat/inc/"
  "${CMAKE_CURRENT_SOURCE_DIR}/../cxplat/inc/winuser"
  "${WDK_ROOT}/include/${WDK_VERSION}/km"
)

target_link_directories(usersim_tests PRIVATE
  "${CMAKE_BINARY_DIR}/lib/${CONFIG}"
)

target_link_libraries(usersim_tests PRIVATE
  "usersim.lib"
  "ntdll.lib"
  Catch2::Catch2WithMain
)

add_dependencies(usersim_tests usersim)
