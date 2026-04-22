/**
 * @File Ops/OpsBridge.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Brief ASCII-only int-returning bridges for element-wise / cast kernels.
 *
 * Consumed by both Api.cpp (MSVC, Result-wrapping) and Ops/*.cu (nvcc).
 * All parameters are raw pointers to device-contiguous buffers; n is the
 * number of elements.
 */

#pragma once

#include <cstddef>

namespace bee::cuda::detail
{

// ScalarType / BinaryOp / UnaryOp values must mirror the enums in Api.hpp.
// They are passed as plain ints to keep this header nvcc-friendly.

int ops_binary(int op, int dt, const void* a, const void* b, void* out, std::size_t n) noexcept;
int ops_unary (int op, int dt, const void* a, void* out, std::size_t n) noexcept;
int ops_cast  (int src_dt, const void* src, int dst_dt, void* dst, std::size_t n) noexcept;

} // namespace bee::cuda::detail
