cmake_minimum_required(VERSION 3.28.3)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/CMake)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CUDA_STANDARD 20)
set(CMAKE_CXX_EXTENSIONS ON)

set(CMAKE_DISABLE_SOURCE_CHANGES ON)
set(CMAKE_DISABLE_IN_SOURCE_BUILD ON)

project(Bee
        LANGUAGES CXX C
        VERSION 0.0.1)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set_property(GLOBAL PROPERTY PREDEFINED_TARGETS_FOLDER "CMake")

add_compile_options("$<$<C_COMPILER_ID:MSVC>:/utf-8>")
add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")
add_compile_options("$<$<COMPILE_LANGUAGE:CUDA>:-Xcompiler=/utf-8>")

# --------------------------------------------------------------
# Global settings
# --------------------------------------------------------------

set(BEE_BUILD_TESTS ON CACHE BOOL "Build test programs")
set(BEE_BUILD_CUDA_TESTS ON CACHE BOOL "Build CUDA test program")

set(BEE_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/Bee/Include)
set(BEE_BINARY_DIR "${CMAKE_SOURCE_DIR}/Bin/$<CONFIG>")
set(BEE_DATA_DIR ${CMAKE_SOURCE_DIR}/Data)
set(BEE_EXTERNAL_DIR ${CMAKE_CURRENT_SOURCE_DIR}/External)
set(BEE_TOOLS_DIR ${CMAKE_SOURCE_DIR}/Tools)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${BEE_BINARY_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${BEE_BINARY_DIR}/Lib")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${BEE_BINARY_DIR}/Lib")

include(BuildMessage)
include(HelperScripts)

message(STATUS "===========================================================")
message(STATUS "${C_Bold} 🐝 Bee Engine${C_Reset} v${CMAKE_PROJECT_VERSION}")
message(STATUS "${LOG_TREE_MID}⚙️  Arch: ${CMAKE_SYSTEM_NAME} (${CMAKE_SYSTEM_PROCESSOR}) / ${CMAKE_CXX_COMPILER_ID}")
message(STATUS "${LOG_TREE_MID}🚀 Build: ${C_Yell}${CMAKE_BUILD_TYPE}${C_Reset}")


PrintTreeEnd()

# --------------------------------------------------------------
# Sources
# --------------------------------------------------------------
message(STATUS "")
message(STATUS " ${C_Bold}📦 Modules${C_Reset}")

add_subdirectory(Bee)

PrintTreeEnd()

add_subdirectory(Tests)

message(STATUS "")
message(STATUS " ✨ ${C_Bold}Ready to Buzz.${C_Reset}")
message(STATUS "===========================================================")