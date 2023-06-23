# Copyright (c) Microsoft Corporation
# SPDX-License-Identifier: MIT

Include(FetchContent)

FetchContent_Declare(
  Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG        v3.0.1 # or a later release
)

FetchContent_MakeAvailable(Catch2)

add_executable(tests
  tests.cpp
)

target_include_directories(tests PRIVATE
  "${CMAKE_BINARY_DIR}/generated-includes/"
  "${CMAKE_BINARY_DIR}/_deps/catch2-src/src/"
)

target_link_libraries(tests PRIVATE
    Catch2::Catch2WithMain)
