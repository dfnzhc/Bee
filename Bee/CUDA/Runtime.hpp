/**
 * @File Runtime.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Brief Internal C-style helpers bridging Api.cu (nvcc, C++20) and Api.cpp
 *        (MSVC, C++23). Kept ASCII-only.
 *
 * These are not part of the public Bee::CUDA API. Public surface is Api.hpp.
 */

#pragma once

#include <cstddef>
#include <cstdint>

namespace bee::cuda::detail
{

// Returns cudaError_t cast to int. 0 == cudaSuccess.
int runtime_get_device_count(int* out) noexcept;
int runtime_set_device(int device_index) noexcept;
int runtime_device_synchronize() noexcept;

// Returns static string pointers owned by CUDA runtime.
const char* runtime_error_name(int err) noexcept;
const char* runtime_error_string(int err) noexcept;

// Whether the .cu TU was compiled by nvcc.
bool runtime_compiled_with_nvcc() noexcept;

} // namespace bee::cuda::detail
