/**
 * @File Cache.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2026/3/26
 * @Brief This file is part of Bee.
 */

#pragma once

#include <cassert>
#include <cstddef>
#include <functional>
#include <list>
#include <optional>
#include <stdexcept>
#include <unordered_map>
#include <utility>

namespace bee
{

// =============================================================================
// LRUCache —— 最近最少使用缓存
// =============================================================================

/**
 * @brief LRU (Least Recently Used) 缓存。
 *
 * 当缓存满时，淘汰最近最少使用的条目。
 * 使用双向链表 + 哈希表实现 O(1) 的 get/put 操作。
 *
 * @tparam Key    键类型
 * @tparam Value  值类型
 * @tparam Hash   哈希函数，默认 std::hash<Key>
 */
template <typename Key, typename Value, typename Hash = std::hash<Key>>
class LRUCache
{
public:
    using key_type    = Key;
    using mapped_type = Value;
    using size_type   = std::size_t;

    /**
     * @brief 构造 LRU 缓存。
     * @param capacity 最大容量，必须大于 0
     */
    explicit LRUCache(size_type capacity)
        : _capacity(capacity)
    {
        if (capacity == 0)
            throw std::invalid_argument("LRUCache: capacity 必须大于 0");
    }

    /**
     * @brief 查询缓存中的值。
     *
     * 如果命中，将该条目移动到最近使用端（链表头部）。
     * @param key 键
     * @return 如果命中返回值的引用，否则返回 std::nullopt
     */
    std::optional<std::reference_wrapper<Value>> get(const Key& key)
    {
        auto it = _map.find(key);
        if (it == _map.end())
            return std::nullopt;

        // 移到链表头部表示最近访问
        _list.splice(_list.begin(), _list, it->second);
        return it->second->second;
    }

    /**
     * @brief 插入或更新缓存条目。
     *
     * 如果键已存在，更新值并将其移到最近使用端。
     * 如果缓存满，淘汰最近最少使用的条目（链表尾部）。
     * @param key   键
     * @param value 值
     */
    void put(const Key& key, const Value& value)
    {
        auto it = _map.find(key);
        if (it != _map.end()) {
            it->second->second = value;
            _list.splice(_list.begin(), _list, it->second);
            return;
        }

        if (_map.size() >= _capacity) {
            // 淘汰最近最少使用的（链表尾部）
            auto& back = _list.back();
            _map.erase(back.first);
            _list.pop_back();
        }

        _list.emplace_front(key, value);
        _map[key] = _list.begin();
    }

    /**
     * @brief 删除指定键的缓存条目。
     * @param key 键
     * @return 是否成功删除
     */
    bool erase(const Key& key)
    {
        auto it = _map.find(key);
        if (it == _map.end())
            return false;
        _list.erase(it->second);
        _map.erase(it);
        return true;
    }

    /**
     * @brief 检查缓存中是否包含指定键。
     * @param key 键
     * @return 是否存在（不改变访问顺序）
     */
    [[nodiscard]] bool contains(const Key& key) const
    {
        return _map.find(key) != _map.end();
    }

    /** @brief 返回当前缓存中的条目数量 */
    [[nodiscard]] size_type size() const noexcept
    {
        return _map.size();
    }

    /** @brief 返回缓存容量 */
    [[nodiscard]] size_type capacity() const noexcept
    {
        return _capacity;
    }

    /** @brief 是否为空 */
    [[nodiscard]] bool empty() const noexcept
    {
        return _map.empty();
    }

    /** @brief 清空缓存 */
    void clear()
    {
        _list.clear();
        _map.clear();
    }

private:
    size_type _capacity;
    std::list<std::pair<Key, Value>> _list; // 前端为最近使用，后端为最久未用
    std::unordered_map<Key, typename std::list<std::pair<Key, Value>>::iterator, Hash> _map;
};

// =============================================================================
// MRUCache —— 最近最多使用缓存
// =============================================================================

/**
 * @brief MRU (Most Recently Used) 缓存。
 *
 * 当缓存满时，淘汰最近使用过的条目。
 * 适用于访问模式中最近使用过的数据反而不太可能再次被访问的场景。
 *
 * @tparam Key    键类型
 * @tparam Value  值类型
 * @tparam Hash   哈希函数
 */
template <typename Key, typename Value, typename Hash = std::hash<Key>>
class MRUCache
{
public:
    using key_type    = Key;
    using mapped_type = Value;
    using size_type   = std::size_t;

    /**
     * @brief 构造 MRU 缓存。
     * @param capacity 最大容量，必须大于 0
     */
    explicit MRUCache(size_type capacity)
        : _capacity(capacity)
    {
        if (capacity == 0)
            throw std::invalid_argument("MRUCache: capacity 必须大于 0");
    }

    /**
     * @brief 查询缓存中的值。命中时将条目移到链表头部（最近使用端）。
     * @param key 键
     * @return 如果命中返回值的引用
     */
    std::optional<std::reference_wrapper<Value>> get(const Key& key)
    {
        auto it = _map.find(key);
        if (it == _map.end())
            return std::nullopt;

        _list.splice(_list.begin(), _list, it->second);
        return it->second->second;
    }

    /**
     * @brief 插入或更新缓存条目。
     *
     * 如果键已存在，更新值并移到最近使用端。
     * 如果缓存满，淘汰最近使用过的条目（链表头部）。
     * @param key   键
     * @param value 值
     */
    void put(const Key& key, const Value& value)
    {
        auto it = _map.find(key);
        if (it != _map.end()) {
            it->second->second = value;
            _list.splice(_list.begin(), _list, it->second);
            return;
        }

        if (_map.size() >= _capacity) {
            // MRU 淘汰最近使用的（链表头部）
            auto& front = _list.front();
            _map.erase(front.first);
            _list.pop_front();
        }

        _list.emplace_front(key, value);
        _map[key] = _list.begin();
    }

    /**
     * @brief 删除指定键的缓存条目。
     * @param key 键
     * @return 是否成功删除
     */
    bool erase(const Key& key)
    {
        auto it = _map.find(key);
        if (it == _map.end())
            return false;
        _list.erase(it->second);
        _map.erase(it);
        return true;
    }

    /** @brief 检查是否包含指定键（不改变访问顺序） */
    [[nodiscard]] bool contains(const Key& key) const
    {
        return _map.find(key) != _map.end();
    }

    [[nodiscard]] size_type size() const noexcept
    {
        return _map.size();
    }

    [[nodiscard]] size_type capacity() const noexcept
    {
        return _capacity;
    }

    [[nodiscard]] bool empty() const noexcept
    {
        return _map.empty();
    }

    void clear()
    {
        _list.clear();
        _map.clear();
    }

private:
    size_type _capacity;
    std::list<std::pair<Key, Value>> _list;
    std::unordered_map<Key, typename std::list<std::pair<Key, Value>>::iterator, Hash> _map;
};

// =============================================================================
// LFUCache —— 最不常使用缓存（淘汰使用频率最低的）
// =============================================================================

/**
 * @brief LFU (Least Frequently Used) 缓存。
 *
 * 当缓存满时，淘汰访问频率最低的条目。
 * 若频率相同，按该频率桶内的 LRU 顺序淘汰最久未使用条目。
 *
 * 实现基于“频率桶 + 键索引”：
 *   - 每个频率维护一个双向链表（桶）
 *   - keyMap 记录键到桶内节点的迭代器
 *   - minFreq 跟踪当前最小频率，便于 O(1) 淘汰
 *
 * @tparam Key    键类型
 * @tparam Value  值类型
 * @tparam Hash   哈希函数
 */
template <typename Key, typename Value, typename Hash = std::hash<Key>>
class LFUCache
{
    struct Entry
    {
        Key key;
        Value value;
        std::size_t freq;
    };

public:
    using key_type    = Key;
    using mapped_type = Value;
    using size_type   = std::size_t;

    /**
     * @brief 构造 LFU 缓存。
     * @param capacity 最大容量，必须大于 0
     */
    explicit LFUCache(size_type capacity)
        : _capacity(capacity), _minFreq(0)
    {
        if (capacity == 0) {
            throw std::invalid_argument("LFUCache: capacity must be greater than 0");
        }
    }

    /**
     * @brief 查询缓存中的值。命中时增加频率。
     * @param key 键
     * @return 如果命中返回值引用，否则返回 std::nullopt
     */
    std::optional<std::reference_wrapper<Value>> get(const Key& key)
    {
        auto it = _keyMap.find(key);
        if (it == _keyMap.end()) {
            return std::nullopt;
        }

        _incrementFreq(it->second);
        return it->second->value;
    }

    /**
     * @brief 插入或更新条目。
     * @param key 键
     * @param value 值
     */
    void put(const Key& key, const Value& value)
    {
        auto it = _keyMap.find(key);
        if (it != _keyMap.end()) {
            it->second->value = value;
            _incrementFreq(it->second);
            return;
        }

        if (_keyMap.size() >= _capacity) {
            _evict();
        }

        _freqBuckets[1].emplace_front(Entry{key, value, 1});
        _keyMap[key] = _freqBuckets[1].begin();
        _minFreq     = 1;
    }

    /**
     * @brief 删除指定键。
     * @param key 键
     * @return 是否成功删除
     */
    bool erase(const Key& key)
    {
        auto it = _keyMap.find(key);
        if (it == _keyMap.end()) {
            return false;
        }

        const auto freq = it->second->freq;
        _freqBuckets[freq].erase(it->second);
        if (_freqBuckets[freq].empty()) {
            _freqBuckets.erase(freq);
            if (_minFreq == freq) {
                _recomputeMinFreq();
            }
        }

        _keyMap.erase(it);
        return true;
    }

    /** @brief 检查是否包含指定键（不改变频率） */
    [[nodiscard]] bool contains(const Key& key) const
    {
        return _keyMap.find(key) != _keyMap.end();
    }

    [[nodiscard]] size_type size() const noexcept
    {
        return _keyMap.size();
    }

    [[nodiscard]] size_type capacity() const noexcept
    {
        return _capacity;
    }

    [[nodiscard]] bool empty() const noexcept
    {
        return _keyMap.empty();
    }

    /** @brief 清空缓存 */
    void clear()
    {
        _keyMap.clear();
        _freqBuckets.clear();
        _minFreq = 0;
    }

private:
    size_type _capacity;
    size_type _minFreq;

    // 频率 -> 同频率条目链表（头部更“新”）
    std::unordered_map<std::size_t, std::list<Entry>> _freqBuckets;
    // 键 -> 条目迭代器
    std::unordered_map<Key, typename std::list<Entry>::iterator, Hash> _keyMap;

    /** @brief 频率提升：从 oldFreq 桶迁移到 oldFreq+1 桶。 */
    void _incrementFreq(typename std::list<Entry>::iterator entryIt)
    {
        const auto oldFreq = entryIt->freq;
        const auto newFreq = oldFreq + 1;

        _freqBuckets[newFreq].splice(_freqBuckets[newFreq].begin(), _freqBuckets[oldFreq], entryIt);
        entryIt->freq = newFreq;

        if (_freqBuckets[oldFreq].empty()) {
            _freqBuckets.erase(oldFreq);
            if (_minFreq == oldFreq) {
                _minFreq = newFreq;
            }
        }
    }

    /** @brief 淘汰最不常使用条目；同频率时淘汰最久未使用条目。 */
    void _evict()
    {
        if (_keyMap.empty()) {
            return;
        }

        auto bucketIt = _freqBuckets.find(_minFreq);
        assert(bucketIt != _freqBuckets.end() && !bucketIt->second.empty());

        auto& bucket = bucketIt->second;
        _keyMap.erase(bucket.back().key);
        bucket.pop_back();

        if (bucket.empty()) {
            _freqBuckets.erase(bucketIt);
            _recomputeMinFreq();
        }
    }

    /** @brief 在删除/淘汰后重新计算最小频率。 */
    void _recomputeMinFreq()
    {
        _minFreq = 0;
        for (const auto& [freq, bucket] : _freqBuckets) {
            if (bucket.empty()) {
                continue;
            }
            if (_minFreq == 0 || freq < _minFreq) {
                _minFreq = freq;
            }
        }
    }
};

// =============================================================================
// MFUCache —— 最常使用缓存（淘汰使用频率最高的）
// =============================================================================

/**
 * @brief MFU (Most Frequently Used) 缓存。
 *
 * 当缓存满时，淘汰使用频率最高的条目。
 * 与 LFU 相反，适用于高频访问的数据不太可能继续被访问的场景。
 *
 * 实现基于频率桶的双向链表结构：
 *   - 每个频率对应一个桶，桶内用链表维护同频率的条目
 *   - 维护当前最大频率的追踪以便快速淘汰
 *
 * @tparam Key    键类型
 * @tparam Value  值类型
 * @tparam Hash   哈希函数
 */
template <typename Key, typename Value, typename Hash = std::hash<Key>>
class MFUCache
{
    struct Entry
    {
        Key key;
        Value value;
        std::size_t freq;
    };

public:
    using key_type    = Key;
    using mapped_type = Value;
    using size_type   = std::size_t;

    /**
     * @brief 构造 MFU 缓存。
     * @param capacity 最大容量，必须大于 0
     */
    explicit MFUCache(size_type capacity)
        : _capacity(capacity), _maxFreq(0)
    {
        if (capacity == 0)
            throw std::invalid_argument("MFUCache: capacity 必须大于 0");
    }

    /**
     * @brief 查询缓存中的值。命中时增加访问频率。
     * @param key 键
     * @return 如果命中返回值的引用
     */
    std::optional<std::reference_wrapper<Value>> get(const Key& key)
    {
        auto it = _keyMap.find(key);
        if (it == _keyMap.end())
            return std::nullopt;

        _incrementFreq(it->second);
        return it->second->value;
    }

    /**
     * @brief 插入或更新缓存条目。
     * @param key   键
     * @param value 值
     */
    void put(const Key& key, const Value& value)
    {
        auto it = _keyMap.find(key);
        if (it != _keyMap.end()) {
            it->second->value = value;
            _incrementFreq(it->second);
            return;
        }

        if (_keyMap.size() >= _capacity) {
            _evict();
        }

        // 新条目的频率为 1
        _freqBuckets[1].emplace_front(Entry{key, value, 1});
        _keyMap[key] = _freqBuckets[1].begin();
        if (_maxFreq == 0)
            _maxFreq = 1;
    }

    /**
     * @brief 删除指定键的缓存条目。
     * @param key 键
     * @return 是否成功
     */
    bool erase(const Key& key)
    {
        auto it = _keyMap.find(key);
        if (it == _keyMap.end())
            return false;

        std::size_t freq = it->second->freq;
        _freqBuckets[freq].erase(it->second);
        if (_freqBuckets[freq].empty()) {
            _freqBuckets.erase(freq);
            if (_maxFreq == freq)
                _recomputeMaxFreq();
        }
        _keyMap.erase(it);
        return true;
    }

    [[nodiscard]] bool contains(const Key& key) const
    {
        return _keyMap.find(key) != _keyMap.end();
    }

    [[nodiscard]] size_type size() const noexcept
    {
        return _keyMap.size();
    }

    [[nodiscard]] size_type capacity() const noexcept
    {
        return _capacity;
    }

    [[nodiscard]] bool empty() const noexcept
    {
        return _keyMap.empty();
    }

    void clear()
    {
        _keyMap.clear();
        _freqBuckets.clear();
        _maxFreq = 0;
    }

private:
    size_type _capacity;
    size_type _maxFreq;

    // 频率 -> 同频率条目链表
    std::unordered_map<std::size_t, std::list<Entry>> _freqBuckets;
    // 键 -> 条目在链表中的迭代器
    std::unordered_map<Key, typename std::list<Entry>::iterator, Hash> _keyMap;

    /**
     * @brief 将条目从当前频率桶移到下一频率桶。
     */
    void _incrementFreq(typename std::list<Entry>::iterator entryIt)
    {
        std::size_t oldFreq = entryIt->freq;
        std::size_t newFreq = oldFreq + 1;

        // 将条目移动到新频率桶
        _freqBuckets[newFreq].splice(_freqBuckets[newFreq].begin(), _freqBuckets[oldFreq], entryIt);
        entryIt->freq = newFreq;

        // 清理空桶
        if (_freqBuckets[oldFreq].empty()) {
            _freqBuckets.erase(oldFreq);
        }

        if (newFreq > _maxFreq) {
            _maxFreq = newFreq;
        }
    }

    /**
     * @brief 淘汰频率最高的条目。同频率时淘汰最近添加到该频率桶的条目。
     */
    void _evict()
    {
        if (_keyMap.empty())
            return;

        auto bucketIt = _freqBuckets.find(_maxFreq);
        assert(bucketIt != _freqBuckets.end() && !bucketIt->second.empty());

        // 淘汰该频率桶中最老的条目（链表尾部）
        auto& bucket = bucketIt->second;
        _keyMap.erase(bucket.back().key);
        bucket.pop_back();

        if (bucket.empty()) {
            _freqBuckets.erase(bucketIt);
            _recomputeMaxFreq();
        }
    }

    /**
     * @brief 重新计算当前最大频率。
     */
    void _recomputeMaxFreq()
    {
        _maxFreq = 0;
        for (auto& [f, bucket] : _freqBuckets) {
            if (!bucket.empty() && f > _maxFreq) {
                _maxFreq = f;
            }
        }
    }
};

// =============================================================================
// ARCache —— 自适应替换缓存 (Adaptive Replacement Cache)
// =============================================================================

/**
 * @brief ARC (Adaptive Replacement Cache) —— 自适应替换缓存。
 *
 * ARC 在 LRU 和 LFU 之间自适应地平衡，动态调整两种策略的空间分配。
 *
 * 核心思想：
 *   维护四个链表（逻辑上）：
 *   - T1: 最近只被访问过一次的条目（对应 LRU 行为）
 *   - T2: 最近被访问过多次的条目（对应 LFU 行为）
 *   - B1: 从 T1 中淘汰的条目的"幽灵"记录（只保存键，不保存值）
 *   - B2: 从 T2 中淘汰的条目的"幽灵"记录
 *
 *   参数 p 表示 T1 的目标大小，通过命中 B1/B2 来动态调整：
 *   - 命中 B1（最近淘汰的单次访问条目）→ 增大 p → 给 T1 更多空间
 *   - 命中 B2（最近淘汰的多次访问条目）→ 减小 p → 给 T2 更多空间
 *
 * 时间复杂度：所有操作 O(1)
 * 空间复杂度：O(2c)，其中 c 为缓存容量（包括幽灵条目）
 *
 * @tparam Key    键类型
 * @tparam Value  值类型
 * @tparam Hash   哈希函数
 */
template <typename Key, typename Value, typename Hash = std::hash<Key>>
class ARCache
{
    // 条目类型标记
    enum class ListType
    {
        T1,
        T2,
        B1,
        B2,
        None
    };

    struct Entry
    {
        Key key;
        Value value;
        ListType type;
    };

    // 幽灵条目不存储值
    struct GhostEntry
    {
        Key key;
        ListType type;
    };

public:
    using key_type    = Key;
    using mapped_type = Value;
    using size_type   = std::size_t;

    /**
     * @brief 构造 ARC 缓存。
     * @param capacity 最大容量（T1 + T2 的总上限），必须大于 0
     */
    explicit ARCache(size_type capacity)
        : _capacity(capacity), _p(0)
    {
        if (capacity == 0)
            throw std::invalid_argument("ARCache: capacity 必须大于 0");
    }

    /**
     * @brief 查询缓存中的值。
     *
     * - 如果在 T1 中命中：移到 T2（从"看过一次"晋升为"看过多次"）
     * - 如果在 T2 中命中：移到 T2 头部
     *
     * @param key 键
     * @return 如果命中返回值的引用
     */
    std::optional<std::reference_wrapper<Value>> get(const Key& key)
    {
        // 检查 T1
        auto it1 = _t1Map.find(key);
        if (it1 != _t1Map.end()) {
            // 命中 T1：移到 T2 头部
            auto& entry = *(it1->second);
            Value val   = std::move(entry.value);
            _t1.erase(it1->second);
            _t1Map.erase(it1);

            _t2.emplace_front(Entry{key, std::move(val), ListType::T2});
            _t2Map[key] = _t2.begin();
            return _t2.begin()->value;
        }

        // 检查 T2
        auto it2 = _t2Map.find(key);
        if (it2 != _t2Map.end()) {
            // 命中 T2：移到 T2 头部
            _t2.splice(_t2.begin(), _t2, it2->second);
            return it2->second->value;
        }

        return std::nullopt;
    }

    /**
     * @brief 插入或更新缓存条目。
     *
     * 根据 ARC 算法的替换规则：
     * - 如果键在 T1/T2 中：更新值
     * - 如果键在 B1 中：说明最近淘汰了一个只访问过一次的条目但现在又需要，增大 p
     * - 如果键在 B2 中：说明最近淘汰了一个多次访问的条目但现在又需要，减小 p
     * - 如果键完全不在任何链表中：作为新条目加入 T1
     *
     * @param key   键
     * @param value 值
     */
    void put(const Key& key, const Value& value)
    {
        // Case 1: 已在 T1 或 T2 中
        auto it1 = _t1Map.find(key);
        if (it1 != _t1Map.end()) {
            it1->second->value = value;
            // 从 T1 移到 T2
            Entry e = std::move(*(it1->second));
            e.type  = ListType::T2;
            _t1.erase(it1->second);
            _t1Map.erase(it1);
            _t2.emplace_front(std::move(e));
            _t2Map[key] = _t2.begin();
            return;
        }

        auto it2 = _t2Map.find(key);
        if (it2 != _t2Map.end()) {
            it2->second->value = value;
            _t2.splice(_t2.begin(), _t2, it2->second);
            return;
        }

        // Case 2: 在 B1 中（幽灵命中）
        auto ib1 = _b1Map.find(key);
        if (ib1 != _b1Map.end()) {
            // 增大 p：给 T1 更多空间
            size_type delta = _b2.size() >= _b1.size() ? 1 : (_b2.size() > 0 ? _b1.size() / _b2.size() : _capacity);
            _p              = std::min(_p + delta, _capacity);

            _replace(false);

            // 从 B1 删除幽灵条目
            _b1.erase(ib1->second);
            _b1Map.erase(ib1);

            // 加入 T2
            _t2.emplace_front(Entry{key, value, ListType::T2});
            _t2Map[key] = _t2.begin();
            return;
        }

        // Case 3: 在 B2 中（幽灵命中）
        auto ib2 = _b2Map.find(key);
        if (ib2 != _b2Map.end()) {
            // 减小 p：给 T2 更多空间
            size_type delta = _b1.size() >= _b2.size() ? 1 : (_b1.size() > 0 ? _b2.size() / _b1.size() : _capacity);
            _p              = (_p > delta) ? _p - delta : 0;

            _replace(true);

            // 从 B2 删除幽灵条目
            _b2.erase(ib2->second);
            _b2Map.erase(ib2);

            // 加入 T2
            _t2.emplace_front(Entry{key, value, ListType::T2});
            _t2Map[key] = _t2.begin();
            return;
        }

        // Case 4: 完全缺失
        size_type l1 = _t1.size() + _b1.size();
        size_type l2 = _t2.size() + _b2.size();

        if (l1 == _capacity) {
            if (_t1.size() < _capacity) {
                // 淘汰 B1 尾部的幽灵条目
                _b1Map.erase(_b1.back().key);
                _b1.pop_back();
                _replace(false);
            } else {
                // T1 满了，直接淘汰 T1 尾部
                _t1Map.erase(_t1.back().key);
                _t1.pop_back();
            }
        } else if (l1 + l2 >= _capacity) {
            if (l1 + l2 >= 2 * _capacity) {
                // 淘汰 B2 尾部的幽灵条目
                if (!_b2.empty()) {
                    _b2Map.erase(_b2.back().key);
                    _b2.pop_back();
                }
            }
            _replace(false);
        }

        // 新条目加入 T1
        _t1.emplace_front(Entry{key, value, ListType::T1});
        _t1Map[key] = _t1.begin();
    }

    /**
     * @brief 删除指定键的缓存条目。
     *
     * 从 T1、T2、B1、B2 中查找并删除。
     * @param key 键
     * @return 是否成功删除
     */
    bool erase(const Key& key)
    {
        auto it1 = _t1Map.find(key);
        if (it1 != _t1Map.end()) {
            _t1.erase(it1->second);
            _t1Map.erase(it1);
            return true;
        }

        auto it2 = _t2Map.find(key);
        if (it2 != _t2Map.end()) {
            _t2.erase(it2->second);
            _t2Map.erase(it2);
            return true;
        }

        auto ib1 = _b1Map.find(key);
        if (ib1 != _b1Map.end()) {
            _b1.erase(ib1->second);
            _b1Map.erase(ib1);
            return true;
        }

        auto ib2 = _b2Map.find(key);
        if (ib2 != _b2Map.end()) {
            _b2.erase(ib2->second);
            _b2Map.erase(ib2);
            return true;
        }

        return false;
    }

    /**
     * @brief 检查缓存中是否包含指定键（仅检查 T1 和 T2，不检查幽灵列表）。
     * @param key 键
     * @return 是否在缓存中
     */
    [[nodiscard]] bool contains(const Key& key) const
    {
        return _t1Map.find(key) != _t1Map.end() || _t2Map.find(key) != _t2Map.end();
    }

    /** @brief 返回缓存中实际条目数（T1 + T2，不含幽灵条目） */
    [[nodiscard]] size_type size() const noexcept
    {
        return _t1.size() + _t2.size();
    }

    /** @brief 返回缓存容量 */
    [[nodiscard]] size_type capacity() const noexcept
    {
        return _capacity;
    }

    /** @brief 是否为空 */
    [[nodiscard]] bool empty() const noexcept
    {
        return _t1.empty() && _t2.empty();
    }

    /** @brief 返回自适应参数 p（T1 的目标大小） */
    [[nodiscard]] size_type targetT1Size() const noexcept
    {
        return _p;
    }

    /** @brief 返回 T1 的当前大小 */
    [[nodiscard]] size_type t1Size() const noexcept
    {
        return _t1.size();
    }

    /** @brief 返回 T2 的当前大小 */
    [[nodiscard]] size_type t2Size() const noexcept
    {
        return _t2.size();
    }

    /** @brief 清空所有数据和幽灵记录 */
    void clear()
    {
        _t1.clear();
        _t2.clear();
        _b1.clear();
        _b2.clear();
        _t1Map.clear();
        _t2Map.clear();
        _b1Map.clear();
        _b2Map.clear();
        _p = 0;
    }

private:
    size_type _capacity;
    size_type _p; // T1 的目标大小，自适应调节

    // T1/T2: 实际缓存条目
    std::list<Entry> _t1;
    std::list<Entry> _t2;

    // B1/B2: 幽灵条目（仅保存键）
    std::list<GhostEntry> _b1;
    std::list<GhostEntry> _b2;

    // 键到迭代器的映射
    std::unordered_map<Key, typename std::list<Entry>::iterator, Hash> _t1Map;
    std::unordered_map<Key, typename std::list<Entry>::iterator, Hash> _t2Map;
    std::unordered_map<Key, typename std::list<GhostEntry>::iterator, Hash> _b1Map;
    std::unordered_map<Key, typename std::list<GhostEntry>::iterator, Hash> _b2Map;

    /**
     * @brief ARC 的核心替换操作。
     *
     * 根据当前 p 值决定从 T1 还是 T2 中淘汰一个条目，
     * 并将被淘汰条目加入对应的幽灵列表（B1 或 B2）。
     *
     * @param favorT2 如果为 true，倾向于从 T1 中淘汰（给 T2 更多空间）
     */
    void _replace(bool favorT2)
    {
        if (_t1.empty() && _t2.empty())
            return;

        // 判断应从 T1 还是 T2 淘汰：
        // - T1 大于目标 p 则从 T1 淘汰
        // - T1 等于目标 p 且 favorT2 为 true 也从 T1 淘汰
        // - 否则从 T2 淘汰
        bool evictFromT1 = false;
        if (!_t1.empty() && (_t1.size() > _p || (favorT2 && _t1.size() == _p))) {
            evictFromT1 = true;
        } else if (_t2.empty()) {
            evictFromT1 = true;
        }

        if (evictFromT1 && !_t1.empty()) {
            // 从 T1 尾部淘汰，加入 B1
            auto& back = _t1.back();
            _b1.emplace_front(GhostEntry{back.key, ListType::B1});
            _b1Map[back.key] = _b1.begin();
            _t1Map.erase(back.key);
            _t1.pop_back();
        } else if (!_t2.empty()) {
            // 从 T2 尾部淘汰，加入 B2
            auto& back = _t2.back();
            _b2.emplace_front(GhostEntry{back.key, ListType::B2});
            _b2Map[back.key] = _b2.begin();
            _t2Map.erase(back.key);
            _t2.pop_back();
        }
    }
};

} // namespace bee
