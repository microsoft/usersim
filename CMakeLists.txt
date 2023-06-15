# Copyright (c) Microsoft Corporation
# SPDX-License-Identifier: MIT

cmake_minimum_required(VERSION 3.16)
project("usersim" LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)

if (IS_DIRECTORY "${PROJECT_SOURCE_DIR}/.git")
  # Install Git pre-commit hook
  file(COPY scripts/pre-commit scripts/commit-msg
       DESTINATION "${PROJECT_SOURCE_DIR}/.git/hooks")
endif ()

list(APPEND CMAKE_CONFIGURATION_TYPES FuzzerDebug)
list(REMOVE_DUPLICATES CMAKE_CONFIGURATION_TYPES)
set(CMAKE_CONFIGURATION_TYPES "${CMAKE_CONFIGURATION_TYPES}" CACHE STRING
    "Add the configurations that we need"
    FORCE)
set(CMAKE_EXE_LINKER_FLAGS_FUZZERDEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG}")
set(CMAKE_SHARED_LINKER_FLAGS_FUZZERDEBUG "${CMAKE_SHARED_LINKER_FLAGS_DEBUG}")
set(CMAKE_C_FLAGS_FUZZERDEBUG "${CMAKE_C_FLAGS_DEBUG} /fsanitize-coverage=inline-bool-flag /fsanitize-coverage=edge /fsanitize-coverage=trace-cmp /fsanitize-coverage=trace-div /ZH:SHA_256")
set(CMAKE_CXX_FLAGS_FUZZERDEBUG "${CMAKE_CXX_FLAGS_DEBUG} /fsanitize-coverage=inline-bool-flag /fsanitize-coverage=edge /fsanitize-coverage=trace-cmp /fsanitize-coverage=trace-div /ZH:SHA_256")

# Find and use the WDK for headers
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/external/FindWDK/cmake")
find_package(WDK REQUIRED)

add_subdirectory("src")

