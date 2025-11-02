project(BeeUnitTest LANGUAGES CXX)

CPMAddPackage(
        NAME gtest
        GITHUB_REPOSITORY google/googletest
        GIT_TAG v1.17.0
        VERSION 1.17.0
        OPTIONS
        "INSTALL_GTEST OFF"
        "gtest_force_shared_crt ON"
)

set(${CMAKE_CURRENT_BINARY_DIR})

AddTestProgram(Core/LoggerTest.cpp "Bee::Core::Logger;GTest::gtest;GTest::gtest_main;GTest::gmock" "Core")
AddTestProgram(Core/ErrorTest.cpp "Bee::Core::Error;GTest::gtest;GTest::gtest_main;GTest::gmock" "Core")

AddTestProgram(Platform/WindowManagerTest.cpp "Bee::Platform::Impl;SDL3;GTest::gtest;GTest::gtest_main;GTest::gmock" "Platform")
AddTestProgram(Platform/InputManagerTest.cpp "Bee::Platform::Impl;GTest::gtest;GTest::gtest_main;GTest::gmock" "Platform")
