cmake_minimum_required(VERSION 3.24)

option(BEE_BUILD_TESTS "Build tests for Bee components" ON)
set(BEE_PUBLIC_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")

include(BeeComponent)

if(BEE_BUILD_TESTS)
    find_package(GTest QUIET)
    if(NOT GTest_FOUND)
        message(STATUS "GTest not found. Disabling BEE_BUILD_TESTS.")
        set(BEE_BUILD_TESTS OFF CACHE BOOL "Build tests for Bee components" FORCE)
    else()
        include(CTest)
        enable_testing()
    endif()
endif()

# Automatically discover all component directories in Bee/* containing CMakeLists.txt.
file(GLOB component_dirs RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}/*")
foreach(component_dir IN LISTS component_dirs)
    set(component_cmake "${CMAKE_CURRENT_SOURCE_DIR}/${component_dir}/CMakeLists.txt")
    if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/${component_dir}" AND EXISTS "${component_cmake}")
        add_subdirectory("${component_dir}")
    endif()
endforeach()
