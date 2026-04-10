/**
 * @File HyperLogLog.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2026/3/23
 * @Brief This file is part of Bee.
 */

#pragma once

#include <bit>
#include <stdexcept>

#include "Base/Numeric/Hash.hpp"
#include "Base/Numeric/Numeric.hpp"

#include <algorithm>
#include <cmath>
#include <vector>

namespace bee
{

/**
 * @brief HyperLogLog —— 概率基数估计算法。
 *
 * 使用固定大小的寄存器数组估计集合中不同元素的个数（基数）。
 * 空间复杂度 O(m)，其中 m = 2^precision；标准误差约为 1.04 / sqrt(m)。
 *
 * 实现要点：
 *   - precision 决定寄存器个数 m = 2^p，取值范围 [4, 18]。
 *   - 每个寄存器存储观察到的最大前导零位数 + 1。
 *   - 估计时使用调和平均数，并对小基数和大基数分别做线性计数和大范围修正。
 *   
 *   https://en.wikipedia.org/wiki/HyperLogLog
 *
 * @tparam T 元素类型，需要可哈希（std::hash<T> 可用）
 */
template <typename T>
class HyperLogLog
{
public:
    using value_type = T;

    // =========================================================================
    // 构造与生命周期管理
    // =========================================================================

    /**
     * @brief 通过精度参数构造。
     * @param precision 精度，取值 [4, 18]，寄存器数量 m = 2^precision。precision 越大精度越高，但内存占用也越大。
     */
    explicit HyperLogLog(std::uint8_t precision = 14)
    {
        if (precision < 4 || precision > 18)
            throw std::invalid_argument("HyperLogLog: precision 必须在 [4, 18] 范围内");

        _precision     = precision;
        _registerCount = std::size_t{1} << _precision;
        _registers.assign(_registerCount, 0);

        // 预计算偏差修正常数 alpha_m
        _alpha = _compute_alpha(_registerCount);
    }

    // =========================================================================
    // 更新
    // =========================================================================

    /**
     * @brief 插入一个元素到集合中。
     *
     * 如果元素之前已经出现过，不会影响最终估计结果（幂等性）。
     * @param item 要插入的元素
     */
    void insert(const T& item)
    {
        std::uint64_t hash = Splitmix64(static_cast<std::uint64_t>(std::hash<T>{}(item)));

        // 高 _precision 位决定寄存器索引
        std::size_t idx = hash >> (64 - _precision);

        // 剩余位中计算前导零的数量 + 1（即 rho 函数）
        std::uint64_t remaining = (hash << _precision) | (std::uint64_t{1} << (_precision - 1));
        std::uint8_t rank       = std::countl_zero(remaining) + 1;

        // 每个寄存器保留观察到的最大 rank
        if (rank > _registers[idx]) {
            _registers[idx] = rank;
        }
    }

    /**
     * @brief 将另一个 HyperLogLog 合并到当前实例。
     *
     * 合并后的结果等价于对两个集合的并集进行估计。
     * 两个实例的精度必须相同。
     * @param other 要合并的 HyperLogLog 实例
     */
    void merge(const HyperLogLog& other)
    {
        if (_precision != other._precision)
            throw std::invalid_argument("merge: 两个 HyperLogLog 的 precision 必须一致");

        for (std::size_t i = 0; i < _registerCount; ++i) {
            _registers[i] = std::max(_registers[i], other._registers[i]);
        }
    }

    // =========================================================================
    // 状态访问与查询
    // =========================================================================

    /**
     * @brief 估计集合的基数（不同元素个数）。
     * @return 估计的基数值
     */
    [[nodiscard]] double estimate() const
    {
        // 计算调和平均数的倒数之和
        double harmonicSum    = 0.0;
        std::size_t zeroCount = 0;

        for (std::size_t i = 0; i < _registerCount; ++i) {
            harmonicSum += 1.0 / static_cast<double>(std::uint64_t{1} << _registers[i]);
            if (_registers[i] == 0) {
                ++zeroCount;
            }
        }

        // 原始 HyperLogLog 估计
        double m      = static_cast<double>(_registerCount);
        double rawEst = _alpha * m * m / harmonicSum;

        // 小范围修正：当估计值较小且存在空寄存器时，使用线性计数
        if (rawEst <= 2.5 * m) {
            if (zeroCount > 0) {
                // 线性计数：E = m * ln(m / V)，V 为空寄存器数量
                rawEst = m * std::log(m / static_cast<double>(zeroCount));
            }
        }

        // 大范围修正：当估计值接近 2^64 时进行校正
        constexpr double pow2_64 = static_cast<double>(std::uint64_t{1} << 63) * 2.0;
        if (rawEst > pow2_64 / 30.0) {
            rawEst = -pow2_64 * std::log(1.0 - rawEst / pow2_64);
        }

        return rawEst;
    }

    /**
     * @brief 估计基数并返回整数值。
     * @return 四舍五入后的基数估计值
     */
    [[nodiscard]] std::size_t cardinality() const
    {
        return static_cast<std::size_t>(estimate() + 0.5);
    }

    /** @brief 返回精度参数 */
    [[nodiscard]] std::uint8_t precision() const noexcept
    {
        return _precision;
    }

    /** @brief 返回寄存器数量 */
    [[nodiscard]] std::size_t register_count() const noexcept
    {
        return _registerCount;
    }

    /** @brief 返回标准误差（约 1.04 / sqrt(m)） */
    [[nodiscard]] double standard_error() const noexcept
    {
        return 1.04 / std::sqrt(static_cast<double>(_registerCount));
    }

    /** @brief 清空所有寄存器 */
    void clear() noexcept
    {
        std::fill(_registers.begin(), _registers.end(), std::uint8_t{0});
    }

    /** @brief 检查是否为空（未插入过任何元素） */
    [[nodiscard]] bool empty() const noexcept
    {
        return std::all_of(_registers.begin(), _registers.end(), [](std::uint8_t v) {
            return v == 0;
        });
    }

private:
    /**
     * @brief 计算偏差修正常数 alpha_m。
     *
     * 对于 m=16,32,64 使用预先计算好的精确值，其他情况使用通用公式。
     */
    static constexpr double _compute_alpha(std::size_t m) noexcept
    {
        // clang-format off
        switch (m) {
        case 16: return 0.673;
        case 32: return 0.697;
        case 64: return 0.709;
        default: return 0.7213 / (1.0 + 1.079 / static_cast<double>(m));
        }
        // clang-format on
    }

private:
    std::uint8_t _precision    = 14;
    std::size_t _registerCount = 0;
    std::vector<std::uint8_t> _registers;
    double _alpha = 0.0;
};

} // namespace bee
