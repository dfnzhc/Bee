cmake_minimum_required(VERSION 3.24)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/CMake")
project(Bee VERSION 0.1.0 LANGUAGES CXX)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

if(MSVC)
    add_compile_options($<$<COMPILE_LANGUAGE:C,CXX>:/utf-8>)
endif()

option(BEE_BUILD_TESTS "Build tests for Bee components" ON)
option(BEE_BUILD_BENCHMARKS "Build benchmarks for Bee components" OFF)

add_subdirectory(Bee)

if(BEE_BUILD_TESTS)
    add_subdirectory(Tests)
endif()

if(BEE_BUILD_BENCHMARKS)
    add_subdirectory(Benchmarks)
endif()
