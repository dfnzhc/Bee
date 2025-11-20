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

if (NOT gtest_ADDED)
    message(WARNING "Failed to add gtest library, skipped unit tests build.")
    return()
endif ()

enable_testing()
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

