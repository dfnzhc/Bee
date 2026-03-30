#include "CUDAKernel/CUDAKernel.hpp"

namespace bee::cuda
{

std::string_view cuda_kernel_name() noexcept
{
    return "CUDAKernel";
}

std::string_view cuda_toolkit_version() noexcept
{
    return BEE_CUDA_VERSION_STRING;
}

bool cuda_kernel_has_cutlass() noexcept
{
    return BEE_HAS_CUTLASS != 0;
}

} // namespace bee::cuda
