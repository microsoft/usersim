# Copyright (c) Microsoft Corporation
# SPDX-License-Identifier: MIT

Include(FetchContent)

FetchContent_Declare(
  Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG        v3.0.1 # or a later release
)

FetchContent_MakeAvailable(Catch2)

add_executable(cxplat_test
  cxplat_fault_injection_test.cpp
  cxplat_initialization_test.cpp
  cxplat_memory_test.cpp
  cxplat_module_test.cpp
  cxplat_processor_test.cpp
  cxplat_rundown_test.cpp
  cxplat_size_test.cpp
  cxplat_time_test.cpp
  cxplat_workitem_test.cpp
)

target_include_directories(cxplat_test PRIVATE
  "${CMAKE_BINARY_DIR}/generated-includes/"
  "${CMAKE_BINARY_DIR}/_deps/catch2-src/src/"
  "${CMAKE_CURRENT_SOURCE_DIR}/../inc/"
  "${CMAKE_CURRENT_SOURCE_DIR}/../inc/winuser"
)

target_link_directories(cxplat_test PRIVATE
  "${CMAKE_BINARY_DIR}/lib/${CONFIG}"
)

target_link_libraries(cxplat_test PRIVATE
  "cxplat_winuser.lib"
  "ntdll.lib"
  Catch2::Catch2WithMain
)

add_dependencies(cxplat_test cxplat_winuser)
