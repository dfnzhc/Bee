#pragma once

#include <cstddef>

namespace bee::tensor::cuda
{

struct ExecContext
{
    void*       stream              = nullptr;
    void*       workspace           = nullptr;
    std::size_t workspace_bytes     = 0;
    bool        synchronize_on_exit = false;
};

[[nodiscard]] auto make_default_exec_context() -> ExecContext;

} // namespace bee::tensor::cuda
