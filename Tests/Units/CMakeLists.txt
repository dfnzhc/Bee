enable_testing()

set(INSTALL_GTEST OFF)
set(gtest_force_shared_crt ON)
find_package(GTest CONFIG)

if (NOT GTest_FOUND)
    message(WARNING "GTest not found, failed to build unit tests.")
    return()
endif ()

include(GoogleTest)

file(GLOB_RECURSE TEST_FILES ./*.cpp)

add_executable(UnitTests ${TEST_FILES})
gtest_discover_tests(UnitTests)

target_link_libraries(UnitTests
        PRIVATE
        Bee::Bee
        GTest::gtest
        GTest::gmock
        GTest::gtest_main
        GTest::gmock_main
)

