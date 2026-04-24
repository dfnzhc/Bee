#include "Tensor/Cuda/ExecContext.hpp"

namespace bee::tensor::cuda
{

auto make_default_exec_context() -> ExecContext
{
    return ExecContext{};
}

} // namespace bee::tensor::cuda
