/**
 * @File Error.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2025/3/28
 * @Brief This file is part of Bee.
 */
 
#pragma once

namespace bee {


template<typename CallbackT, typename ResultT = int> inline int Guardian(CallbackT callback, ResultT errorResult = 1)
{
    ResultT result = errorResult;
    try {
        result = callback();
    } catch (...) {
        // TODO: handle exceptions.
    }
    return result;
}


} // namespace bee