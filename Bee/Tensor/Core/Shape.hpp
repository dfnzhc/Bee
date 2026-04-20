#pragma once

// MVP 阶段使用 std::vector 实现 Shape/Strides，后续可替换为 SmallVector 优化小 rank 场景

#include <cstdint>
#include <numeric>
#include <vector>

namespace bee
{

// 张量维度列表（元素类型为 int64_t）
using Shape = std::vector<int64_t>;

// 步长列表（元素单位，非字节）
using Strides = std::vector<int64_t>;

// 计算元素总数；空 shape 视作 scalar，返回 1
[[nodiscard]] inline auto numel(const Shape& shape) noexcept -> int64_t
{
    if (shape.empty())
        return 1;
    int64_t n = 1;
    for (const auto dim : shape)
        n *= dim;
    return n;
}

// 计算行优先（C order）步长：最右维步长为 1
[[nodiscard]] inline auto compute_contiguous_strides(const Shape& shape) -> Strides
{
    if (shape.empty())
        return {};

    const auto ndim = static_cast<int>(shape.size());
    Strides strides(ndim);
    strides[ndim - 1] = 1;
    for (int i = ndim - 2; i >= 0; --i)
        strides[i] = strides[i + 1] * shape[i + 1];
    return strides;
}

// 判断是否 C-contiguous；size==1 的维度步长任意视为连续
[[nodiscard]] inline auto is_contiguous(const Shape& shape, const Strides& strides) noexcept -> bool
{
    if (shape.size() != strides.size())
        return false;
    if (shape.empty())
        return true;

    int64_t expected = 1;
    for (int i = static_cast<int>(shape.size()) - 1; i >= 0; --i) {
        if (shape[i] != 1) {
            if (strides[i] != expected)
                return false;
            expected *= shape[i];
        }
        // size==1 的维度步长任意，不更新 expected
    }
    return true;
}

// 判断两个 shape 是否相等
[[nodiscard]] inline auto shapes_equal(const Shape& a, const Shape& b) noexcept -> bool
{
    return a == b;
}

} // namespace bee
