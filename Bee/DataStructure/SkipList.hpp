/**
 * @File SkipList.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2026/3/26
 * @Brief This file is part of Bee.
 */

#pragma once

#include <cassert>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <initializer_list>
#include <iterator>
#include <memory>
#include <random>
#include <stdexcept>
#include <utility>
#include <vector>

namespace bee
{

/**
 * @brief SkipList —— 基于跳表的有序关联容器。
 *
 * 跳表是一种概率性数据结构，通过多层链表实现 O(log n) 的期望查找、插入和删除。
 * 与平衡二叉搜索树（如红黑树）相比，跳表实现更简单，且天然支持范围查询。
 *
 * 性能特征：
 *   - 查找：O(log n) 期望时间
 *   - 插入：O(log n) 期望时间
 *   - 删除：O(log n) 期望时间
 *   - 范围扫描：O(log n + k)，k 为范围内元素数量
 *
 * @tparam Key      键类型，需要支持 < 比较
 * @tparam Value    值类型
 * @tparam Compare  比较函数类型，默认 std::less<Key>
 */
template <std::totally_ordered Key, typename Value, typename Compare = std::less<Key>>
class SkipList
{
    static constexpr int    kMaxLevel    = 32;
    static constexpr double kProbability = 0.25; // 每层晋升概率

    // =========================================================================
    // 内部节点定义
    // =========================================================================
    struct Node
    {
        Key                key;
        Value              value;
        int                level;   // 该节点的层数（1-based）
        std::vector<Node*> forward; // forward[i] 指向第 i 层的后继节点

        Node(Key k, Value v, int lvl)
            : key(std::move(k))
            , value(std::move(v))
            , level(lvl)
            , forward(lvl, nullptr)
        {
        }
    };

    // 哨兵头节点，不存储有效数据
    struct HeadNode
    {
        int                level;
        std::vector<Node*> forward;

        explicit HeadNode(int lvl)
            : level(lvl)
            , forward(lvl, nullptr)
        {
        }
    };

public:
    // =========================================================================
    // 类型定义
    // =========================================================================
    using key_type    = Key;
    using mapped_type = Value;
    using value_type  = std::pair<const Key, Value>;
    using size_type   = std::size_t;

    // =========================================================================
    // 前向迭代器
    // =========================================================================

    /**
     * @brief 跳表的前向迭代器，只遍历最底层链表。
     */
    class iterator
    {
    public:
        using iterator_category = std::forward_iterator_tag;
        using value_type        = std::pair<const Key, Value>;
        using difference_type   = std::ptrdiff_t;
        using pointer           = void;
        using reference         = std::pair<const Key&, Value&>;

        iterator() noexcept
            : _node(nullptr)
        {
        }

        explicit iterator(Node* node) noexcept
            : _node(node)
        {
        }

        reference operator*() const
        {
            return {_node->key, _node->value};
        }

        iterator& operator++()
        {
            _node = _node->forward[0];
            return *this;
        }

        iterator operator++(int)
        {
            iterator tmp = *this;
            ++(*this);
            return tmp;
        }

        bool operator==(const iterator& other) const noexcept
        {
            return _node == other._node;
        }

        bool operator!=(const iterator& other) const noexcept
        {
            return _node != other._node;
        }

        Node* node() const noexcept
        {
            return _node;
        }

    private:
        Node* _node;
    };

    class const_iterator
    {
    public:
        using iterator_category = std::forward_iterator_tag;
        using value_type        = std::pair<const Key, Value>;
        using difference_type   = std::ptrdiff_t;
        using pointer           = void;
        using reference         = std::pair<const Key&, const Value&>;

        const_iterator() noexcept
            : _node(nullptr)
        {
        }

        explicit const_iterator(const Node* node) noexcept
            : _node(node)
        {
        }

        const_iterator(const iterator& it) noexcept
            : _node(it.node())
        {
        }

        reference operator*() const
        {
            return {_node->key, _node->value};
        }

        const_iterator& operator++()
        {
            _node = _node->forward[0];
            return *this;
        }

        const_iterator operator++(int)
        {
            const_iterator tmp = *this;
            ++(*this);
            return tmp;
        }

        bool operator==(const const_iterator& other) const noexcept
        {
            return _node == other._node;
        }

        bool operator!=(const const_iterator& other) const noexcept
        {
            return _node != other._node;
        }

    private:
        const Node* _node;
    };

    // =========================================================================
    // 构造与生命周期管理
    // =========================================================================

    /**
     * @brief 默认构造，创建空跳表。
     * @param comp 比较函数对象
     */
    explicit SkipList(const Compare& comp = Compare())
        : _comp(comp)
        , _head(kMaxLevel)
        , _size(0)
        , _level(1)
    {
        // 使用随机设备初始化随机数生成器
        std::random_device rd;
        _rng.seed(rd());
    }

    /**
     * @brief 从初始化列表构造。
     * @param init 初始化列表，每个元素为 {key, value} 对
     * @param comp 比较函数对象
     */
    SkipList(std::initializer_list<std::pair<Key, Value>> init, const Compare& comp = Compare())
        : SkipList(comp)
    {
        for (auto& [k, v] : init) {
            insert(k, v);
        }
    }

    ~SkipList()
    {
        _clear();
    }

    // 禁止拷贝，允许移动
    SkipList(const SkipList&)            = delete;
    SkipList& operator=(const SkipList&) = delete;

    SkipList(SkipList&& other) noexcept
        : _comp(std::move(other._comp))
        , _head(kMaxLevel)
        , _size(other._size)
        , _level(other._level)
        , _rng(std::move(other._rng))
    {
        for (int i = 0; i < kMaxLevel; ++i) {
            _head.forward[i]       = other._head.forward[i];
            other._head.forward[i] = nullptr;
        }
        other._size  = 0;
        other._level = 1;
    }

    SkipList& operator=(SkipList&& other) noexcept
    {
        if (this != &other) {
            _clear();
            _comp  = std::move(other._comp);
            _size  = other._size;
            _level = other._level;
            _rng   = std::move(other._rng);
            for (int i = 0; i < kMaxLevel; ++i) {
                _head.forward[i]       = other._head.forward[i];
                other._head.forward[i] = nullptr;
            }
            other._size  = 0;
            other._level = 1;
        }
        return *this;
    }

    // =========================================================================
    // 插入
    // =========================================================================

    /**
     * @brief 插入键值对。若键已存在则更新值。
     * @param key   键
     * @param value 值
     * @return 指向插入/更新元素的迭代器和是否为新插入的布尔值
     */
    std::pair<iterator, bool> insert(const Key& key, const Value& value)
    {
        return _doInsert(key, value);
    }

    /**
     * @brief 移动语义插入键值对。若键已存在则更新值。
     * @param key   键
     * @param value 值（右值引用）
     * @return 指向插入/更新元素的迭代器和是否为新插入的布尔值
     */
    std::pair<iterator, bool> insert(const Key& key, Value&& value)
    {
        return _doInsert(key, std::move(value));
    }

    /**
     * @brief 在原地构造键值对。
     * @param key  键
     * @param args 用于构造值的参数
     * @return 指向插入/更新元素的迭代器和是否为新插入的布尔值
     */
    template <typename... Args>
    std::pair<iterator, bool> emplace(const Key& key, Args&&... args)
    {
        return _doInsert(key, Value(std::forward<Args>(args)...));
    }

    // =========================================================================
    // 删除
    // =========================================================================

    /**
     * @brief 删除指定键的元素。
     * @param key 要删除的键
     * @return 是否成功删除（键存在则返回 true）
     */
    bool erase(const Key& key)
    {
        // update[i] 记录每层中最后一个键 < key 的节点
        Node* update[kMaxLevel];
        Node* current = nullptr;

        // 从最高层向下搜索
        _findUpdatePath(key, update, current);

        // current 指向可能匹配的节点
        current = (current != nullptr) ? current->forward[0] : _head.forward[0];

        if (current == nullptr || _comp(key, current->key) || _comp(current->key, key)) {
            return false; // 键不存在
        }

        // 从各层中移除该节点
        for (int i = 0; i < _level; ++i) {
            Node* prev = (i == 0 && update[0] == nullptr) ? nullptr : update[i];
            Node* prevForward;
            if (prev != nullptr) {
                prevForward = prev->forward[i];
            } else {
                prevForward = _head.forward[i];
            }

            if (prevForward != current) {
                break;
            }

            if (prev != nullptr) {
                prev->forward[i] = current->forward[i];
            } else {
                _head.forward[i] = current->forward[i];
            }
        }

        delete current;
        --_size;

        // 如果最高层已经没有节点，则降低层数
        while (_level > 1 && _head.forward[_level - 1] == nullptr) {
            --_level;
        }

        return true;
    }

    /**
     * @brief 删除迭代器指向的元素。
     * @param it 指向要删除元素的迭代器
     * @return 指向被删除元素之后元素的迭代器
     */
    iterator erase(iterator it)
    {
        if (it == end())
            return end();
        Node*    target = it.node();
        iterator next(target->forward[0]);
        erase(target->key);
        return next;
    }

    // =========================================================================
    // 查找
    // =========================================================================

    /**
     * @brief 查找指定键的元素。
     * @param key 要查找的键
     * @return 指向找到元素的迭代器，未找到则返回 end()
     */
    [[nodiscard]] iterator find(const Key& key)
    {
        Node* node = _findNode(key);
        return node ? iterator(node) : end();
    }

    /** @brief const 版本的查找 */
    [[nodiscard]] const_iterator find(const Key& key) const
    {
        const Node* node = _findNodeConst(key);
        return node ? const_iterator(node) : cend();
    }

    /**
     * @brief 检查是否包含指定键。
     * @param key 要检查的键
     * @return 是否存在
     */
    [[nodiscard]] bool contains(const Key& key) const
    {
        return _findNodeConst(key) != nullptr;
    }

    /**
     * @brief 访问指定键对应的值。若键不存在则抛出 std::out_of_range。
     * @param key 键
     * @return 对应值的引用
     */
    [[nodiscard]] Value& at(const Key& key)
    {
        Node* node = _findNode(key);
        if (!node)
            throw std::out_of_range("SkipList::at: 键不存在");
        return node->value;
    }

    /** @brief const 版本的 at */
    [[nodiscard]] const Value& at(const Key& key) const
    {
        const Node* node = _findNodeConst(key);
        if (!node)
            throw std::out_of_range("SkipList::at: 键不存在");
        return node->value;
    }

    /**
     * @brief 下标访问。若键不存在则插入默认值。
     * @param key 键
     * @return 对应值的引用
     */
    Value& operator[](const Key& key)
    {
        auto [it, _] = insert(key, Value{});
        return it.node()->value;
    }

    // =========================================================================
    // 范围查询
    // =========================================================================

    /**
     * @brief 返回第一个不小于 key 的元素的迭代器。
     * @param key 下界键
     * @return 迭代器
     */
    [[nodiscard]] iterator lower_bound(const Key& key)
    {
        Node* node = _lowerBound(key);
        return iterator(node);
    }

    /** @brief const 版本 */
    [[nodiscard]] const_iterator lower_bound(const Key& key) const
    {
        const Node* node = _lowerBoundConst(key);
        return const_iterator(node);
    }

    /**
     * @brief 返回第一个大于 key 的元素的迭代器。
     * @param key 上界键
     * @return 迭代器
     */
    [[nodiscard]] iterator upper_bound(const Key& key)
    {
        Node* node = _upperBound(key);
        return iterator(node);
    }

    /** @brief const 版本 */
    [[nodiscard]] const_iterator upper_bound(const Key& key) const
    {
        const Node* node = _upperBoundConst(key);
        return const_iterator(node);
    }

    // =========================================================================
    // 迭代器
    // =========================================================================

    [[nodiscard]] iterator begin() noexcept
    {
        return iterator(_head.forward[0]);
    }

    [[nodiscard]] iterator end() noexcept
    {
        return iterator(nullptr);
    }

    [[nodiscard]] const_iterator begin() const noexcept
    {
        return const_iterator(_head.forward[0]);
    }

    [[nodiscard]] const_iterator end() const noexcept
    {
        return const_iterator(nullptr);
    }

    [[nodiscard]] const_iterator cbegin() const noexcept
    {
        return const_iterator(_head.forward[0]);
    }

    [[nodiscard]] const_iterator cend() const noexcept
    {
        return const_iterator(nullptr);
    }

    // =========================================================================
    // 容量与状态
    // =========================================================================

    /** @brief 返回元素个数 */
    [[nodiscard]] size_type size() const noexcept
    {
        return _size;
    }

    /** @brief 是否为空 */
    [[nodiscard]] bool empty() const noexcept
    {
        return _size == 0;
    }

    /** @brief 返回当前最高层数 */
    [[nodiscard]] int level() const noexcept
    {
        return _level;
    }

    /** @brief 清空所有元素 */
    void clear()
    {
        _clear();
        _level = 1;
    }

private:
    Compare              _comp;
    HeadNode             _head;
    size_type            _size;
    int                  _level; // 当前最高层数
    mutable std::mt19937 _rng;

    /**
     * @brief 随机生成节点层数。
     *
     * 使用几何分布：每次有 kProbability 的概率晋升到上一层。
     * 最大层数不超过 kMaxLevel。
     */
    int _randomLevel()
    {
        // 使用位运算模拟几何分布，比 uniform_real_distribution 快数倍。
        // 每 2 位提供一次伯努利试验（kProbability = 0.25 = 1/4）。
        int           lvl = 1;
        std::uint32_t rn  = _rng();
        while ((rn & 0x3u) == 0 && lvl < kMaxLevel) {
            ++lvl;
            rn >>= 2;
            if (lvl % 16 == 0) // 用完 32 位后重新生成
                rn = _rng();
        }
        return lvl;
    }

    /**
     * @brief 在跳表中搜索 key，填充 update 数组。
     *
     * update[i] 为第 i 层中最后一个键 < key 的节点指针。
     * 如果第 i 层的前驱是头节点，update[i] 设为 nullptr。
     */
    void _findUpdatePath(const Key& key, Node* update[], Node*& lastBeforeTarget) const
    {
        // 从头节点开始搜索
        // 使用 current == nullptr 表示当前位于头节点
        Node* current = nullptr;

        for (int i = _level - 1; i >= 0; --i) {
            Node* forward = (current != nullptr) ? current->forward[i] : _head.forward[i];
            while (forward != nullptr && _comp(forward->key, key)) {
                current = forward;
                forward = current->forward[i];
            }
            update[i] = current; // nullptr 表示此层前驱为头节点
        }
        lastBeforeTarget = current;
    }

    /**
     * @brief 执行插入的核心逻辑。
     */
    template <typename V>
    std::pair<iterator, bool> _doInsert(const Key& key, V&& value)
    {
        Node* update[kMaxLevel];
        Node* current = nullptr;

        _findUpdatePath(key, update, current);

        // 检查是否已有该键
        Node* candidate = (current != nullptr) ? current->forward[0] : _head.forward[0];

        if (candidate != nullptr && !_comp(key, candidate->key) && !_comp(candidate->key, key)) {
            // 键已存在，更新值
            candidate->value = std::forward<V>(value);
            return {iterator(candidate), false};
        }

        // 生成新节点层数
        int newLevel = _randomLevel();

        // 创建新节点（RAII guard 保证异常安全）
        auto  nodeGuard = std::unique_ptr<Node>(new Node(key, std::forward<V>(value), newLevel));
        auto* newNode   = nodeGuard.get();

        if (newLevel > _level) {
            // 新增的高层的 update 全部指向头节点
            for (int i = _level; i < newLevel; ++i) {
                update[i] = nullptr;
            }
            _level = newLevel;
        }

        // 在各层中插入新节点
        for (int i = 0; i < newLevel; ++i) {
            if (update[i] != nullptr) {
                newNode->forward[i]   = update[i]->forward[i];
                update[i]->forward[i] = newNode;
            } else {
                newNode->forward[i] = _head.forward[i];
                _head.forward[i]    = newNode;
            }
        }

        ++_size;
        nodeGuard.release(); // Ownership transferred to the skip list
        return {iterator(newNode), true};
    }

    /**
     * @brief 查找指定键的节点。
     * @return 找到则返回节点指针，否则返回 nullptr
     */
    const Node* _findNodeConst(const Key& key) const
    {
        const Node* current = nullptr;

        for (int i = _level - 1; i >= 0; --i) {
            const Node* forward = (current != nullptr) ? current->forward[i] : _head.forward[i];
            while (forward != nullptr && _comp(forward->key, key)) {
                current = forward;
                forward = current->forward[i];
            }
        }

        const Node* candidate = (current != nullptr) ? current->forward[0] : _head.forward[0];
        if (candidate != nullptr && !_comp(key, candidate->key) && !_comp(candidate->key, key)) {
            return candidate;
        }
        return nullptr;
    }

    Node* _findNode(const Key& key)
    {
        return const_cast<Node*>(_findNodeConst(key));
    }

    /**
     * @brief 查找第一个不小于 key 的节点。
     */
    const Node* _lowerBoundConst(const Key& key) const
    {
        const Node* current = nullptr;

        for (int i = _level - 1; i >= 0; --i) {
            const Node* forward = (current != nullptr) ? current->forward[i] : _head.forward[i];
            while (forward != nullptr && _comp(forward->key, key)) {
                current = forward;
                forward = current->forward[i];
            }
        }

        return (current != nullptr) ? current->forward[0] : _head.forward[0];
    }

    Node* _lowerBound(const Key& key)
    {
        return const_cast<Node*>(_lowerBoundConst(key));
    }

    /**
     * @brief 查找第一个大于 key 的节点。
     */
    const Node* _upperBoundConst(const Key& key) const
    {
        const Node* current = nullptr;

        for (int i = _level - 1; i >= 0; --i) {
            const Node* forward = (current != nullptr) ? current->forward[i] : _head.forward[i];
            while (forward != nullptr && !_comp(key, forward->key)) {
                current = forward;
                forward = current->forward[i];
            }
        }

        return (current != nullptr) ? current->forward[0] : _head.forward[0];
    }

    Node* _upperBound(const Key& key)
    {
        return const_cast<Node*>(_upperBoundConst(key));
    }

    /**
     * @brief 释放所有节点内存。
     */
    void _clear()
    {
        Node* current = _head.forward[0];
        while (current != nullptr) {
            Node* next = current->forward[0];
            delete current;
            current = next;
        }
        for (int i = 0; i < kMaxLevel; ++i) {
            _head.forward[i] = nullptr;
        }
        _size = 0;
    }
};

} // namespace bee
