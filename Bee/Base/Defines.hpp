/**
 * @File Defines.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2026/3/22
 * @Brief This file is part of Bee.
 */

#pragma once

#include <new>

#if defined(__has_cpp_attribute)
    #define BEE_HAS_CPP_ATTRIBUTE(x) __has_cpp_attribute(x)
#else
    #define BEE_HAS_CPP_ATTRIBUTE(x) 0
#endif

#if defined(__has_include)
    #define BEE_HAS_INCLUDE(x) __has_include(x)
#else
    #define BEE_HAS_INCLUDE(x) 0
#endif

#if defined(__has_builtin)
    #define BEE_HAS_BUILTIN(x) __has_builtin(x)
#else
    #define BEE_HAS_BUILTIN(x) 0
#endif

#if BEE_HAS_CPP_ATTRIBUTE(no_unique_address) >= 201803L
    #define BEE_NO_UNIQUE_ADDRESS [[no_unique_address]]
#elif BEE_HAS_CPP_ATTRIBUTE(msvc::no_unique_address) >= 201803L
    #define BEE_NO_UNIQUE_ADDRESS [[msvc::no_unique_address]]
#else
    #define BEE_NO_UNIQUE_ADDRESS
#endif

#if defined(__cpp_lib_hardware_interference_size) && !defined(__APPLE__)
    #define BEE_CACHE_LINE_SIZE std::hardware_destructive_interference_size
#else
    #define BEE_CACHE_LINE_SIZE 64
#endif
