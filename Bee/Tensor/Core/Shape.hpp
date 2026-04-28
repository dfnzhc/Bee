#pragma once

// Shape/Strides 使用 std::vector 存储任意 rank 的维度元数据。所有 stride
// 均以“元素”为单位，而不是字节；字节偏移由 stride * dtype_size(dtype)
// 在访问层统一换算。

#include <cstdint>
#include <numeric>
#include <vector>

namespace bee
{

// 张量维度列表。空 shape 表示标量；非空 shape 中每个维度表示该轴长度。
using Shape = std::vector<int64_t>;

// 步长列表，单位为元素。strides[i] 表示第 i 维坐标加一时 storage 中前进
// 的元素数量。
using Strides = std::vector<int64_t>;

// 计算元素总数；空 shape 视作标量，返回 1。
[[nodiscard]] inline auto numel(const Shape& shape) noexcept -> int64_t
{
    if (shape.empty())
        return 1;
    int64_t n = 1;
    for (const auto dim : shape)
        n *= dim;
    return n;
}

// 计算行优先（C order）连续布局的步长：最右维步长为 1，向左逐维乘以
// 右侧维度长度。
[[nodiscard]] inline auto compute_contiguous_strides(const Shape& shape) -> Strides
{
    if (shape.empty())
        return {};

    const auto ndim = static_cast<int>(shape.size());
    Strides    strides(ndim);
    strides[ndim - 1] = 1;
    for (int i = ndim - 2; i >= 0; --i)
        strides[i] = strides[i + 1] * shape[i + 1];
    return strides;
}

// 判断 shape/strides 是否表示 C-contiguous 布局。
// size==1 的维度不会改变线性地址，因此其 stride 可为任意值并仍视为连续。
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
