cmake_minimum_required(VERSION 4.0)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/CMake)

set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)
set(CMAKE_CXX_STANDARD 23)

set(CMAKE_DISABLE_SOURCE_CHANGES ON)
set(CMAKE_DISABLE_IN_SOURCE_BUILD ON)

project(Bee
        LANGUAGES CXX C
        VERSION 0.0.1)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set_property(GLOBAL PROPERTY PREDEFINED_TARGETS_FOLDER "CMake")

# --------------------------------------------------------------
# Global settings
# --------------------------------------------------------------

set(BEE_BUILD_TESTS ON CACHE BOOL "Build test programs")

# 项目路径
set(BEE_BINARY_DIR "${CMAKE_SOURCE_DIR}/Bin/$<CONFIG>")
set(BEE_DATA_DIR ${CMAKE_SOURCE_DIR}/Data)
set(BEE_EXTERNAL_DIR ${CMAKE_CURRENT_SOURCE_DIR}/External)
set(BEE_TOOLS_DIR ${CMAKE_SOURCE_DIR}/Tools)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${BEE_BINARY_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${BEE_BINARY_DIR}/Lib")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${BEE_BINARY_DIR}/Lib")

include(HelperScripts)
include(CPM)

# --------------------------------------------------------------
# Sources
# --------------------------------------------------------------

# Extern libraries
add_subdirectory(External)

# Project
add_subdirectory(Source)

# Tests
add_subdirectory(Tests)