
enable_testing()

set(INSTALL_GTEST OFF)
set(gtest_force_shared_crt ON)
find_package(GTest CONFIG REQUIRED)

if (NOT GTest_FOUND)
    message(WARNING "GTest not found, failed to build unit tests.")
    return()
endif ()

include(GoogleTest)

AddTestProgram(CoreTest.cpp "Bee::Core;GTest::gtest;GTest::gtest_main" "Core")
AddTestProgram(TupleTest.cpp "Bee::Container;GTest::gtest;GTest::gtest_main" "Container")

AddTestProgram(Math/BitsTest.cpp "Bee::Math;GTest::gtest;GTest::gtest_main" "Bits")
AddTestProgram(Math/CommonTest.cpp "Bee::Math;GTest::gtest;GTest::gtest_main" "Common")
AddTestProgram(Math/VectorTest.cpp "Bee::Math;GTest::gtest;GTest::gtest_main" "VectorMath")
