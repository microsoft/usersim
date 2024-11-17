# Copyright (c) Microsoft Corporation
# SPDX-License-Identifier: MIT

cmake_minimum_required(VERSION 3.16)
project("usersim" LANGUAGES CXX C)
set(CMAKE_CXX_STANDARD 20)

if (IS_DIRECTORY "${PROJECT_SOURCE_DIR}/.git")
  # Install Git pre-commit hook
  file(COPY scripts/git_hooks/pre-commit scripts/git_hooks/commit-msg
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

# # Find and use the WDK for headers
# list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/external/FindWDK/cmake")
# find_package(WDK REQUIRED)

set(NUGET_PACKAGES 
"Microsoft.Windows.SDK.CPP"
"Microsoft.Windows.SDK.CPP.arm64"
"Microsoft.Windows.SDK.CPP.x64"
"Microsoft.Windows.WDK.ARM64"
"Microsoft.Windows.WDK.x64"
)


find_program(NUGET nuget)
if(NOT NUGET)
  message("ERROR: You must first install nuget.exe from https://www.nuget.org/downloads")
else()
  foreach(PACKAGE ${NUGET_PACKAGES})
    execute_process(COMMAND ${NUGET} install ${PACKAGE} -ExcludeVersion -OutputDirectory ${PROJECT_BINARY_DIR}/packages)
  endforeach()
endif()

# Set WDK_ROOT based on the platform (arm64 or x64)
if (TARGET_PLATFORM STREQUAL "arm64")
  set(WDK_ROOT "${PROJECT_BINARY_DIR}/packages/Microsoft.Windows.WDK.ARM64/c")
  set(WDK_VERSION "10.0.26100.0")
else ()
set(WDK_ROOT "${PROJECT_BINARY_DIR}/packages/Microsoft.Windows.WDK.x64/c")
set(WDK_VERSION "10.0.26100.0")
endif ()


# Configure output directories
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${TARGET_PLATFORM}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${CMAKE_ARCHIVE_OUTPUT_DIRECTORY}/Debug)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${CMAKE_ARCHIVE_OUTPUT_DIRECTORY}/Release)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${TARGET_PLATFORM}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/Debug)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/Release)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${TARGET_PLATFORM}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Debug)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Release)

add_subdirectory("cxplat")
add_subdirectory("src")
add_subdirectory("usersim_dll_skeleton")
add_subdirectory("sample")
add_subdirectory("tests")
