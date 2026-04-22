#pragma once

#include <cstdint>
#include <string_view>

namespace bee
{

// 计算设备类型
enum class Device : uint8_t
{
    CPU,
    CUDA,
};

// 返回设备名称字符串
[[nodiscard]] constexpr auto device_name(Device d) noexcept -> std::string_view
{
    switch (d) {
    case Device::CPU: return "CPU";
    case Device::CUDA: return "CUDA";
    default: return "Unknown";
    }
}

} // namespace bee
