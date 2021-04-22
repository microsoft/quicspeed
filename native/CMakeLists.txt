# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

cmake_minimum_required(VERSION 3.16)

# Disable in-source builds to prevent source tree corruption.
if("${CMAKE_CURRENT_SOURCE_DIR}" STREQUAL "${CMAKE_BINARY_DIR}")
  message(FATAL_ERROR "
FATAL: In-source builds are not allowed.
       You should create a separate directory for build files.
")
endif()

message(STATUS "CMAKE Version: ${CMAKE_VERSION}")

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

message(STATUS "Source Dir: ${CMAKE_CURRENT_SOURCE_DIR}")
message(STATUS "Host System name: ${CMAKE_HOST_SYSTEM_NAME}")
if ("${CMAKE_HOST_SYSTEM_NAME}" STREQUAL "Windows")
    set(CMAKE_SYSTEM_VERSION 10.0.18362.0 CACHE STRING INTERNAL FORCE)
    set(CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION 10.0.18362.0 CACHE STRING INTERNAL FORCE)
endif()

if(POLICY CMP0091)
    cmake_policy(SET CMP0091 NEW)
    message(STATUS "Setting policy 0091")
else()
    message(WARNING "CMake version too old to support Policy 0091; CRT static linking won't work")
endif()

if (POLICY CMP0111)
    cmake_policy(SET CMP0111 NEW)
endif()

project(quicspeed)

# Set a default build type if none was specified
set(default_build_type "Release")

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE "${default_build_type}" CACHE
        STRING "Choose the type of build." FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS
                 "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()

set(QUIC_BUILD_DIR ${CMAKE_CURRENT_BINARY_DIR})
set(QUIC_OUTPUT_DIR ${QUIC_BUILD_DIR}/bin/$<IF:$<CONFIG:Debug>,Debug,Release> CACHE STRING "Output directory for build artifacts")

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${QUIC_BUILD_DIR}/obj/$<IF:$<CONFIG:Debug>,Debug,Release>)

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${QUIC_OUTPUT_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${QUIC_OUTPUT_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE ${QUIC_OUTPUT_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${QUIC_OUTPUT_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG ${QUIC_OUTPUT_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${QUIC_OUTPUT_DIR})

add_subdirectory(msquic)

if(WIN32)
    set(SOURCES quicspeed.cpp winuser/dllmain.c winuser/quicspeednative.rc $<TARGET_OBJECTS:MsQuicEtw_Resource>)
else()
    set(SOURCES quicspeed.cpp linux/init.c)
endif()

add_library(quicspeednative SHARED ${SOURCES})

set_property(TARGET quicspeednative PROPERTY FOLDER "libraries")

target_link_libraries(quicspeednative PRIVATE core platform perflib inc warnings perfbin.clog)

if(WIN32)
    if(QUIC_UWP_BUILD)
        target_link_libraries(quicspeednative PUBLIC OneCoreUAP)
    endif()
    SET_TARGET_PROPERTIES(quicspeednative
        PROPERTIES LINK_FLAGS "/DEF:\"${CMAKE_CURRENT_SOURCE_DIR}/winuser/quicspeednative.def\"")
elseif (CX_PLATFORM STREQUAL "linux")
    SET_TARGET_PROPERTIES(quicspeednative
        PROPERTIES LINK_FLAGS "-Wl,--version-script=\"${CMAKE_CURRENT_SOURCE_DIR}/linux/exports.txt\"")
elseif (CX_PLATFORM STREQUAL "darwin")
    SET_TARGET_PROPERTIES(quicspeednative
        PROPERTIES LINK_FLAGS "-exported_symbols_list \"${CMAKE_CURRENT_SOURCE_DIR}/darwin/exports.txt\"")
endif()

target_include_directories(quicspeednative PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../../inc>
    $<INSTALL_INTERFACE:${include_dest}>)
