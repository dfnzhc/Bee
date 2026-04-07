#pragma once

#include <string_view>

namespace bee
{

std::string_view cuda_kernel_name() noexcept;

std::string_view cuda_toolkit_version() noexcept;

bool cuda_kernel_has_cutlass() noexcept;

int cuda_kernel_compiled_with_nvcc() noexcept;

} // namespace bee
