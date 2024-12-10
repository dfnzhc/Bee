cmake_minimum_required(VERSION 3.7...3.27)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/CMake)

set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_DISABLE_SOURCE_CHANGES ON)
set(CMAKE_DISABLE_IN_SOURCE_BUILD ON)

# --------------------------------------------------------------
# Project Configuration
# --------------------------------------------------------------

project(Bee 
        LANGUAGES CXX C)

string(TOLOWER ${PROJECT_NAME} PROJECT_NAME_LOWERCASE)
string(TOUPPER ${PROJECT_NAME} PROJECT_NAME_UPPERCASE)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set_property(GLOBAL PROPERTY PREDEFINED_TARGETS_FOLDER "CMake")

# Platform
if (${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    set(${PROJECT_NAME_UPPERCASE}_PLATFORM "Windows")
    set(${PROJECT_NAME_UPPERCASE}_IN_WINDOWS TRUE)
else ()
    # ...
endif ()

add_compile_options("$<$<C_COMPILER_ID:MSVC>:/utf-8>")
add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")

# --------------------------------------------------------------
# Global settings
# --------------------------------------------------------------

set(${PROJECT_NAME_UPPERCASE}_PRECOMPILED_HEADERS ON CACHE BOOL "Use precompiled header")
set(${PROJECT_NAME_UPPERCASE}_BUILD_TESTS ON CACHE BOOL "Build test programs")

set(${PROJECT_NAME_UPPERCASE}_BINARY_DIR ${CMAKE_SOURCE_DIR}/Bin)

if (CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_CONFIGURATION_TYPES "Debug;Release;MinSizeRel;RelWithDebInfo" CACHE STRING "" FORCE)
else ()
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY HELPSTRING "Choose the type of build")
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug;Release;MinSizeRel;RelWithDebInfo")
endif ()

if (CMAKE_CONFIGURATION_TYPES)
    set(${PROJECT_NAME_UPPERCASE}_OUTPUT_DIR ${${PROJECT_NAME_UPPERCASE}_RUNTIME_OUTPUT_DIR}/$<CONFIG>)
else ()
    set(${PROJECT_NAME_UPPERCASE}_OUTPUT_DIR ${${PROJECT_NAME_UPPERCASE}_RUNTIME_OUTPUT_DIR})
endif ()

if (WIN32)
    string(REPLACE "/O2" "/Od" CMAKE_CXX_FLAGS_RELWITHDEBINFO ${CMAKE_CXX_FLAGS_RELWITHDEBINFO})
    if (MSVC)
        set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
    endif ()

    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${${PROJECT_NAME_UPPERCASE}_BINARY_DIR}/Libs)
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${${PROJECT_NAME_UPPERCASE}_BINARY_DIR}/Libs)
endif ()

if (MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CXX_STANDARD} /Zc:preprocessor")
endif()

include(HelperFunctions)
include(Commands)

# --------------------------------------------------------------
# Sources
# --------------------------------------------------------------

######################################
# Extern libraries
add_subdirectory(External)

######################################
# Project
add_subdirectory(Source)

######################################
# 测试
if (${PROJECT_NAME_UPPERCASE}_BUILD_TESTS)
    enable_testing()
    add_subdirectory(Tests)
endif ()