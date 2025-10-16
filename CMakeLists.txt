cmake_minimum_required(VERSION 4.0)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/CMake)

set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)
set(CMAKE_CXX_STANDARD 23)

set(CMAKE_DISABLE_SOURCE_CHANGES ON)
set(CMAKE_DISABLE_IN_SOURCE_BUILD ON)

# --------------------------------------------------------------
# Project Configuration
# --------------------------------------------------------------

project(Bee
        LANGUAGES CXX C
        VERSION 0.0.1)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set_property(GLOBAL PROPERTY PREDEFINED_TARGETS_FOLDER "CMake")

add_compile_options("$<$<C_COMPILER_ID:MSVC>:/utf-8>")
add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")

# --------------------------------------------------------------
# Global settings
# --------------------------------------------------------------

if (CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_CONFIGURATION_TYPES "Debug;Release;MinSizeRel;RelWithDebInfo" CACHE STRING "" FORCE)
else ()
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY HELPSTRING "Choose the type of build")
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug;Release;MinSizeRel;RelWithDebInfo")
endif ()

set(BEE_BUILD_TESTS ON CACHE BOOL "Build test programs")
set(BEE_BUILD_BENCHMARKS ON CACHE BOOL "Build benchmark programs")

set(BEE_BINARY_DIR "${CMAKE_SOURCE_DIR}/Bin/$<CONFIG>")
set(BEE_DATA_DIR ${CMAKE_SOURCE_DIR}/Data)
set(BEE_EXTERNAL_DIR ${CMAKE_CURRENT_SOURCE_DIR}/External)
set(BEE_TOOLS_DIR ${CMAKE_SOURCE_DIR}/Tools)

set(BEE_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set(BEE_VERSION_MINOR ${PROJECT_VERSION_MINOR})
set(BEE_VERSION_PATCH ${PROJECT_VERSION_PATCH})

# CMake directory
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${BEE_BINARY_DIR}/Libs)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${BEE_BINARY_DIR}/Libs)

if (WIN32)
    if (MSVC)
        set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
    endif ()
endif ()

include(CPM)
include(HelperScripts)

# --------------------------------------------------------------
# Sources
# --------------------------------------------------------------

# Extern libraries
add_subdirectory(External)

# Project
add_subdirectory(Source)

# Tests
add_subdirectory(Tests)