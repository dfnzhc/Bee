/**
 * @File CountMinSketch.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2026/3/23
 * @Brief This file is part of Bee.
 */

#pragma once

#include "Base/Hash.hpp"

#include <cstdint>
#include <stdexcept>
#include <type_traits>
#include <vector>

#include "Base/Numeric.hpp"

namespace bee
{

/**
 * @brief Count-Min Sketch —— 一种亚线性空间的频率估计概率数据结构。
 *
 * 使用 d 个独立的哈希函数，每个哈希函数对应一行宽度为 w 的计数器数组。
 * 插入时对 d 行全部递增，查询时取 d 行计数的最小值，因此只会高估、不会低估。
 *
 * 误差保证：
 *   - 以至少 1 - delta 的概率，估计值与真实值的偏差不超过 epsilon * N，
 *     其中 N 是所有元素的插入总次数。
 *   - 宽度 w = ceil(e / epsilon)，深度 d = ceil(ln(1 / delta))。
 *
 * 改进方向：Count-Mean-Min Sketch
 *
 * @tparam T        元素类型，需要可哈希（std::hash<T> 可用）
 * @tparam CountT   计数器类型，默认 uint64_t
 */
template <typename T, typename CountT = std::uint64_t>
class CountMinSketch
{
    static_assert(std::is_integral_v<CountT> && std::is_unsigned_v<CountT>, "CountT 必须是无符号整数类型");

public:
    using value_type = T;
    using count_type = CountT;

    // =========================================================================
    // 构造
    // =========================================================================

    /**
     * @brief 通过精确的宽度和深度构造。
     * @param width  每行计数器的个数（w）
     * @param depth  哈希函数的个数（d，即行数）
     */
    CountMinSketch(std::size_t width, std::size_t depth) : _width(width), _depth(depth), _table(width * depth, count_type{0}), _totalCount(0)
    {
        if (width == 0 || depth == 0)
            throw std::invalid_argument("CountMinSketch: width 和 depth 必须大于 0");

        // 生成各行的哈希种子，使用简单的线性序列
        _seeds.resize(_depth);
        // 使用不同的质数作为各行种子，使哈希函数近似独立
        for (std::size_t i = 0; i < _depth; ++i) {
            _seeds[i] = 0x9E3779B97F4A7C15ULL * (i + 1);
        }
    }

    /**
     * @brief 通过误差参数 epsilon 和置信参数 delta 构造。
     * @param epsilon 相对误差上界，必须在 (0, 1) 之间
     * @param delta   失败概率上界，必须在 (0, 1) 之间
     */
    static CountMinSketch from_error_params(double epsilon, double delta)
    {
        if (epsilon <= 0.0 || epsilon >= 1.0)
            throw std::invalid_argument("epsilon 必须在 (0, 1) 之间");
        if (delta <= 0.0 || delta >= 1.0)
            throw std::invalid_argument("delta 必须在 (0, 1) 之间");

        auto width = static_cast<std::size_t>(std::ceil(std::exp(1.0) / epsilon));
        auto depth = static_cast<std::size_t>(std::ceil(std::log(1.0 / delta)));
        if (depth < 1)
            depth = 1;
        return CountMinSketch(width, depth);
    }

    // =========================================================================
    // 更新接口
    // =========================================================================

    /**
     * @brief 插入一个元素（计数 +1）。
     * @param item 要插入的元素
     */
    void insert(const T& item)
    {
        update(item, count_type{1});
    }

    /**
     * @brief 以指定的增量更新元素的计数。
     * @param item  元素
     * @param count 增量
     */
    void update(const T& item, count_type count)
    {
        auto h = std::hash<T>{}(item);
        for (std::size_t i = 0; i < _depth; ++i) {
            std::size_t idx = _hash_to_index(h, i);
            auto& cell      = _table[i * _width + idx];
            cell            = SaturatingAdd(cell, count);
        }
        _totalCount = SaturatingAdd(_totalCount, count);
    }

    /**
     * @brief 将另一个相同维度的 CountMinSketch 合并到当前实例中。
     *
     * 合并要求两个 sketch 的宽度、深度和种子完全一致。
     * @param other 要合并的 sketch
     */
    void merge(const CountMinSketch& other)
    {
        if (_width != other._width || _depth != other._depth)
            throw std::invalid_argument("merge: 两个 sketch 的维度必须一致");
        if (_seeds != other._seeds)
            throw std::invalid_argument("merge: 两个 sketch 的哈希种子必须一致");

        for (std::size_t i = 0; i < _table.size(); ++i) {
            _table[i] = SaturatingAdd(_table[i], other._table[i]);
        }
        _totalCount = SaturatingAdd(_totalCount, other._totalCount);
    }

    // =========================================================================
    // 查询接口
    // =========================================================================

    /**
     * @brief 查询元素的估计频率（取各行计数的最小值）。
     * @param item 要查询的元素
     * @return 估计的频率（保证 >= 真实频率）
     */
    [[nodiscard]] count_type estimate(const T& item) const
    {
        auto h            = std::hash<T>{}(item);
        count_type result = std::numeric_limits<count_type>::max();
        for (std::size_t i = 0; i < _depth; ++i) {
            std::size_t idx = _hash_to_index(h, i);
            result          = std::min(result, _table[i * _width + idx]);
        }
        return result;
    }

    /** @brief 返回每行的宽度 */
    [[nodiscard]] std::size_t width() const noexcept
    {
        return _width;
    }

    /** @brief 返回哈希函数个数（深度） */
    [[nodiscard]] std::size_t depth() const noexcept
    {
        return _depth;
    }

    /** @brief 返回所有插入元素的总计数 */
    [[nodiscard]] count_type total_count() const noexcept
    {
        return _totalCount;
    }

    /** @brief 检查是否为空（未插入过任何元素） */
    [[nodiscard]] bool empty() const noexcept
    {
        return _totalCount == 0;
    }

    /** @brief 清空所有计数器 */
    void clear() noexcept
    {
        std::fill(_table.begin(), _table.end(), count_type{0});
        _totalCount = 0;
    }

    /** @brief 获取哈希种子 */
    [[nodiscard]] const std::vector<std::uint64_t>& seeds() const noexcept
    {
        return _seeds;
    }

    /**
     * @brief 使用指定的种子
     * @param seeds 种子数组，长度必须等于 depth
     */
    void set_seeds(const std::vector<std::uint64_t>& seeds)
    {
        if (seeds.size() != _depth)
            throw std::invalid_argument("seeds 长度必须等于 depth");
        _seeds = seeds;
        clear();
    }

private:
    /**
     * @brief 将元素哈希值与行种子混合后映射到 [0, width) 的列索引。
     */
    [[nodiscard]] std::size_t _hash_to_index(std::size_t hash, std::size_t row) const noexcept
    {
        return Splitmix64(hash ^ _seeds[row]) % _width;
    }

private:
    std::size_t _width;
    std::size_t _depth;
    std::vector<count_type> _table;
    std::vector<std::uint64_t> _seeds;
    count_type _totalCount;
};

} // namespace bee
