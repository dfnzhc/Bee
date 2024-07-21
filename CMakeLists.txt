cmake_minimum_required(VERSION 3.27)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/CMake)

set(BEE_VERSION 0.0.1)
project(bee VERSION ${BEE_VERSION} LANGUAGES CXX)
message(STATUS "BEE: Version " ${BEE_VERSION})

set(BEE_IS_MASTER_PROJECT OFF)
if (${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_CURRENT_SOURCE_DIR})
    set(BEE_IS_MASTER_PROJECT ON)
endif ()

option(BEE_BUILD_LIBRARY "Build dynamic/static library" ON)
option(BEE_BUILD_TESTS "Build the test programs" OFF)
option(BEE_BUILD_INSTALL "Generate the install target" ${BEE_IS_MASTER_PROJECT})

include(GNUInstallDirs)

option(BEE_ENABLE_CXX_20 "Enable C++ 20" OFF)

set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (BEE_ENABLE_CXX_20)
    set(CMAKE_CXX_STANDARD 20)
    add_definitions(-DBEE_FORCE_CXX20)
    if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        message(STATUS "BEE: Disable -Wc++98-compat warnings")
        add_compile_options(-Wno-c++98-compat)
        add_compile_options(-Wno-c++98-compat-pedantic)
    endif ()
    if (NOT BEE_QUIET)
        message(STATUS "BEE: Build with C++20 features")
    endif ()

endif ()

if (BEE_ENABLE_LANG_EXTENSIONS)
    set(CMAKE_CXX_EXTENSIONS ON)
    if ((CMAKE_CXX_COMPILER_ID MATCHES "Clang") OR (CMAKE_CXX_COMPILER_ID MATCHES "GNU"))
        add_compile_options(-fms-extensions)
    endif ()
    message(STATUS "BEE: Build with C++ language extensions")
else ()
    set(CMAKE_CXX_EXTENSIONS OFF)
    if (CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
        add_compile_options(/Za)
        if (MSVC15)
            add_compile_options(/permissive-)
        endif ()
    endif ()
endif ()

option(BEE_ENABLE_FAST_MATH "Enable fast math optimizations" OFF)
if (BEE_ENABLE_FAST_MATH)
    if (NOT BEE_QUIET)
        message(STATUS "BEE: Build with fast math optimizations")
    endif ()

    if ((CMAKE_CXX_COMPILER_ID MATCHES "Clang") OR (CMAKE_CXX_COMPILER_ID MATCHES "GNU"))
        add_compile_options(-ffast-math)

    elseif (CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
        add_compile_options(/fp:fast)
    endif ()
else ()
    if (CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
        add_compile_options(/fp:precise)
    endif ()
endif ()

option(BEE_ENABLE_SIMD_SSE2 "Enable SSE2 optimizations" OFF)
option(BEE_ENABLE_SIMD_SSE3 "Enable SSE3 optimizations" OFF)
option(BEE_ENABLE_SIMD_SSSE3 "Enable SSSE3 optimizations" OFF)
option(BEE_ENABLE_SIMD_SSE4_1 "Enable SSE 4.1 optimizations" OFF)
option(BEE_ENABLE_SIMD_SSE4_2 "Enable SSE 4.2 optimizations" OFF)
option(BEE_ENABLE_SIMD_AVX "Enable AVX optimizations" OFF)
option(BEE_ENABLE_SIMD_AVX2 "Enable AVX2 optimizations" OFF)
option(BEE_TEST_ENABLE_SIMD_NEON "Enable ARM NEON optimizations" OFF)
option(BEE_FORCE_PURE "Force 'pure' instructions" OFF)

add_subdirectory(bee)

set(BEE_BUILD_TESTS ON)
if (BEE_BUILD_TESTS)
    include(CompilerFlags)
    enable_testing()
    add_subdirectory(Tests)
endif ()

if (BEE_BUILD_INSTALL)
    include(CPack)

    install(TARGETS bee-header-only bee EXPORT bee)
    install(
            DIRECTORY bee
            DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
            PATTERN "CMakeLists.txt" EXCLUDE
    )
    install(
            EXPORT bee
            NAMESPACE bee::
            DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/bee"
            FILE beeConfig.cmake
    )
    include(CMakePackageConfigHelpers)
    write_basic_package_version_file(
            "${CMAKE_CURRENT_BINARY_DIR}/beeConfigVersion.cmake"
            COMPATIBILITY AnyNewerVersion
    )
    install(
            FILES "${CMAKE_CURRENT_BINARY_DIR}/beeConfigVersion.cmake"
            DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/bee"
    )

    configure_file(
            "${CMAKE_CURRENT_SOURCE_DIR}/cmake/cmake_uninstall.cmake.in"
            "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
            IMMEDIATE @ONLY
    )

    add_custom_target(
            uninstall
            "${CMAKE_COMMAND}" -P
            "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
    )
endif ()
