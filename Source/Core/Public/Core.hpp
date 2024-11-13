/**
 * @File Core.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2024/11/13
 * @Brief This file is part of Bee.
 */

#pragma once

// -------------------------
//关于平台、编译器、语言的宏定义

#if defined(_MSC_VER)
#  define BEE_API_EXPORT __declspec(dllexport)
#  define BEE_API_IMPORT __declspec(dllimport)
#elif defined(__clang__) || defined(__GNUC__)
#  define BEE_API_EXPORT __attribute__((visibility("default")))
#  define BEE_API_IMPORT __attribute__((visibility("default")))
#else
#  define BEE_API_EXPORT
#  define BEE_API_IMPORT
#endif

#ifdef BEE_DLL
#  define BEE_API BEE_API_EXPORT
#else  // BEE_DLL
#  define BEE_API BEE_API_IMPORT
#endif // BEE_DLL

namespace bee {

inline int Test(int a, int b)
{
    return a + b;
}

BEE_API int Test2(int a, int b);

} // namespace bee
