cmake_minimum_required(VERSION 3.27)

project(BeeTest LANGUAGES CXX)

set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

find_package(GTest CONFIG REQUIRED)
#find_package(benchmark CONFIG REQUIRED)

if (MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CXX_STANDARD} /Zc:preprocessor")
endif()

set(${CMAKE_CURRENT_BINARY_DIR})

AddTestProgram(Runtime/LoggerTest.cpp "Bee::Runtime")
AddTestProgram(Runtime/MathTest.cpp "Bee::Runtime")
AddTestProgram(Engine/EngineTest.cpp "Bee::Runtime;Bee::Engine")
