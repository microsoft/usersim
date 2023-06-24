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
  tests.cpp
)

target_include_directories(usersim_tests PRIVATE
  "${CMAKE_BINARY_DIR}/generated-includes/"
  "${CMAKE_BINARY_DIR}/_deps/catch2-src/src/"
  "${CMAKE_CURRENT_SOURCE_DIR}/../inc/"
  "${WDK_ROOT}/include/${WDK_VERSION}/km"
)

target_link_directories(usersim_tests PRIVATE
  "${CMAKE_BINARY_DIR}/lib/${CONFIG}"
)

target_link_libraries(usersim_tests PRIVATE
  "usersim.lib"
  Catch2::Catch2WithMain
)

add_dependencies(usersim_tests usersim)
