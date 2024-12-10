cmake_minimum_required(VERSION 3.27)

project(BeeTest LANGUAGES CXX)

set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

find_package(GTest CONFIG REQUIRED)
#find_package(benchmark CONFIG REQUIRED)

set(${CMAKE_CURRENT_BINARY_DIR})

AddTestProgram(Runtime/LoggerTest.cpp "GTest::gtest;GTest::gtest_main;Bee::Runtime")
AddTestProgram(Runtime/MathTest.cpp "GTest::gtest;GTest::gtest_main;Bee::Runtime")
AddTestProgram(Runtime/InputsTest.cpp "GTest::gtest;GTest::gtest_main;Bee::Runtime")
AddTestProgram(Runtime/ObjectTest.cpp "GTest::gtest;GTest::gtest_main;Bee::Runtime")
AddTestProgram(Runtime/GraphTest.cpp "GTest::gtest;GTest::gtest_main;Bee::Runtime")

find_package (Vulkan REQUIRED)
# Launch Test
add_executable(EngineLaunchTest WIN32 Engine/LaunchTest.cpp)
set_target_properties(EngineLaunchTest
        PROPERTIES
        FOLDER "Tests"
        WORKING_DIRECTORY ${${PROJECT_NAME_UPPERCASE}_BINARY_DIR}
)

target_compile_features(EngineLaunchTest PUBLIC cxx_std_23)

target_link_libraries(EngineLaunchTest PRIVATE Bee::Engine Vulkan::Vulkan)
