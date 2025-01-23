cmake_minimum_required(VERSION 3.27)

project(BeeTest LANGUAGES CXX)

set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

find_package(GTest CONFIG REQUIRED)
#find_package(benchmark CONFIG REQUIRED)

set(${CMAKE_CURRENT_BINARY_DIR})

AddTestProgram(Core/ObjectTest.cpp "GTest::gtest;GTest::gtest_main;Bee::Bee")
AddTestProgram(Graphics/VulkanTest.cpp "GTest::gtest;GTest::gtest_main;Bee::Bee")
AddTestProgram(Graphics/GraphicsTest.cpp "GTest::gtest;GTest::gtest_main;Bee::Bee")
AddTestProgram(Math/MathTest.cpp "GTest::gtest;GTest::gtest_main;Bee::Bee")
AddTestProgram(Utility/UtilityTest.cpp "GTest::gtest;GTest::gtest_main;Bee::Bee")
AddTestProgram(Utility/IOTest.cpp "GTest::gtest;GTest::gtest_main;Bee::Bee")
AddTestProgram(Utility/GraphTest.cpp "GTest::gtest;GTest::gtest_main;Bee::Bee")

target_include_directories(VulkanTest
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../Source/Bee/Private
)

find_package (Vulkan REQUIRED)
# Launch Test
add_executable(LaunchTest WIN32 Utility/LaunchTest.cpp)
set_target_properties(LaunchTest
        PROPERTIES
        FOLDER "Tests"
        WORKING_DIRECTORY ${${PROJECT_NAME_UPPERCASE}_BINARY_DIR}
)

target_compile_features(LaunchTest PUBLIC cxx_std_23)
target_link_libraries(LaunchTest PRIVATE Bee::Bee Vulkan::Vulkan)