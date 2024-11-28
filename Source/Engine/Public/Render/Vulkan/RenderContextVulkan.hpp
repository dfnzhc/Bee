/**
 * @File RenderContextVulkan.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2024/11/25
 * @Brief This file is part of Bee.
 */

#pragma once

#include "Render/RenderContext.hpp"

namespace bee {

class BEE_API RenderContextVulkan final : public RenderContext
{
public:
    ~RenderContextVulkan() override = default;
    
    // clang-format off
    Error initialize() override;
    
    BEE_NODISCARD const DeviceInfo& deviceInfo(u32 devIdx) const override;
    BEE_NODISCARD u32 deviceCount() const override;
    BEE_NODISCARD bool deviceSupportsPresent(u32 devIdx) const override;

    BEE_NODISCARD std::unique_ptr<RenderDriver> createDriver() override;
    // clang-format on

private:
    
};

} // namespace bee