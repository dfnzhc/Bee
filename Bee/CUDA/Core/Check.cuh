/**
 * @File Core/Check.cuh
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Brief .cu 翻译单元使用的 CUDA 错误检查宏。
 *
 * CUDA 源文件使用原始 int 错误码，而 host 侧 .cpp 使用 Bee::Result。
 * 这种分层避免 nvcc 翻译单元依赖 <expected>、std::format 等 host 侧诊断
 * 设施，并让错误转换集中在 Api.cpp 中完成。
 *
 * 约定：本文件中的宏只能用于返回 int 或兼容错误码的函数；0 表示成功，
 * 非 0 表示 cudaError_t 的整数编码。
 */

#pragma once

#include <cuda_runtime.h>
#include "Base/Core/Defines.hpp"

// 执行返回 cudaError_t 的表达式；失败时返回对应整数错误码。
// 调用者函数必须返回 int 或兼容的错误码类型。
#define BEE_CUDA_RET_ON_ERR(expr)                 \
    do {                                          \
        const cudaError_t _bee_cu_err = (expr);   \
        if (_bee_cu_err != cudaSuccess) {         \
            (void)cudaGetLastError();             \
            return static_cast<int>(_bee_cu_err); \
        }                                         \
    } while (0)

// 启动 kernel，并在启动失败或 last-error 非成功时返回整数错误码。
// 使用可变参数形式以兼容 kernel 启动语法中的逗号。
#define BEE_CUDA_LAUNCH_RET(...)                                   \
    do {                                                           \
        __VA_ARGS__;                                               \
        const cudaError_t _bee_cu_launch_err = cudaGetLastError(); \
        if (_bee_cu_launch_err != cudaSuccess) {                   \
            return static_cast<int>(_bee_cu_launch_err);           \
        }                                                          \
    } while (0)

// 当前函数的成功返回码；与 cudaSuccess 的整数编码一致。
#define BEE_CUDA_OK 0
