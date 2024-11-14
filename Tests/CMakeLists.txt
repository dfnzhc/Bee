cmake_minimum_required(VERSION 3.27)

project(BeeTest LANGUAGES CXX)

set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

find_package(GTest CONFIG REQUIRED)
#find_package(benchmark CONFIG REQUIRED)

set(TEST_SOURCES
        Core/CoreTest.cpp
        Core/LoggerTest.cpp
)

if (MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CXX_STANDARD} /Zc:preprocessor")
endif()

set(${CMAKE_CURRENT_BINARY_DIR})

foreach (FILE ${TEST_SOURCES})
    get_filename_component(FILE_NAME ${FILE} NAME_WE)
    add_executable(${FILE_NAME} ${FILE})
    set_target_properties(${FILE_NAME} 
            PROPERTIES
            FOLDER "Tests")

    target_link_libraries(${FILE_NAME}
            PUBLIC GTest::gtest GTest::gtest_main #benchmark::benchmark
            PRIVATE Bee::Core)

    add_test(NAME "${FILE_NAME}Test"
            COMMAND ${FILE_NAME}
            WORKING_DIRECTORY ${${PROJECT_NAME_UPPERCASE}_BINARY_DIR})
endforeach ()
