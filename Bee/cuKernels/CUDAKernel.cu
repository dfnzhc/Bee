#include "cuKernels/CUDAKernel.hpp"

namespace bee
{

int cuda_kernel_compiled_with_nvcc() noexcept
{
    #if defined(__CUDACC__)
    return 1;
    #else
    return 0;
    #endif
}

} // namespace bee
