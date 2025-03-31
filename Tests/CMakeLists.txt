cmake_minimum_required(VERSION 3.27)

project(BeeTest LANGUAGES CXX)

set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

find_package(GTest CONFIG REQUIRED)
#find_package(benchmark CONFIG REQUIRED)

set(${CMAKE_CURRENT_BINARY_DIR})

AddTestProgram(Base/ErrorTest.cpp "GTest::gtest;GTest::gtest_main;Bee::Bee")
AddTestProgram(Math/MathTest.cpp "GTest::gtest;GTest::gtest_main;Bee::Bee")
