#include "cuKernels/CUDAKernel.hpp"

namespace bee
{

bool cuda_kernel_compiled_with_nvcc() noexcept
{
#if defined(__CUDACC__)
    return true;
#else
    return false;
#endif
}

} // namespace bee
