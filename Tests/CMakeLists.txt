cmake_minimum_required(VERSION 3.27)

project(BeeTest LANGUAGES CXX)

set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

find_package(GTest CONFIG REQUIRED)
find_package(benchmark CONFIG REQUIRED)

set(TEST_SOURCES
        BitTest.cpp
        CommonTest.cpp
        ConstantTest.cpp
        FloatTest.cpp
)

set(${CMAKE_CURRENT_BINARY_DIR})

foreach (FILE ${TEST_SOURCES})
    get_filename_component(FILE_NAME ${FILE} NAME_WE)
    add_executable(${FILE_NAME} ${FILE})
    set_property(TARGET ${FILE_NAME} PROPERTY FOLDER "Tests")
    set_compiler_flags(${FILE_NAME})

    target_link_libraries(${FILE_NAME}
            PUBLIC GTest::gtest GTest::gtest_main benchmark::benchmark
            PRIVATE bee::bee)

    set_target_properties(${FILE_NAME} PROPERTIES
            CXX_STANDARD 20
    )
    add_test(NAME "${FILE_NAME}Test"
            COMMAND ${FILE_NAME})
endforeach ()
