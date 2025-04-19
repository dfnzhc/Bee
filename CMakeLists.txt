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
        LANGUAGES CXX C
        VERSION 0.0.1)

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

if (CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_CONFIGURATION_TYPES "Debug;Release;MinSizeRel;RelWithDebInfo" CACHE STRING "" FORCE)
else ()
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY HELPSTRING "Choose the type of build")
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug;Release;MinSizeRel;RelWithDebInfo")
endif ()

set(${PROJECT_NAME_UPPERCASE}_BINARY_DIR ${CMAKE_SOURCE_DIR}/Bin/${CMAKE_BUILD_TYPE})
set(${PROJECT_NAME_UPPERCASE}_DATA_DIR ${CMAKE_SOURCE_DIR}/Data)
set(${PROJECT_NAME_UPPERCASE}_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set(${PROJECT_NAME_UPPERCASE}_VERSION_MINOR ${PROJECT_VERSION_MINOR})
set(${PROJECT_NAME_UPPERCASE}_VERSION_PATCH ${PROJECT_VERSION_PATCH})

configure_file(Config.hpp.in Config.hpp @ONLY)

if (WIN32)
    if (MSVC)
        set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
    endif ()

    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${${PROJECT_NAME_UPPERCASE}_BINARY_DIR}/Libs)
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${${PROJECT_NAME_UPPERCASE}_BINARY_DIR}/Libs)
endif ()

include(HelperFunctions)
include(QtTools)

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
# Tests
if (${PROJECT_NAME_UPPERCASE}_BUILD_TESTS)
    enable_testing()
    add_subdirectory(Tests)
endif ()