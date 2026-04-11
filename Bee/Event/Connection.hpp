/**
 * @File Connection.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2026/4/10
 * @Brief This file is part of Bee.
 */

#pragma once

#include <atomic>
#include <memory>
#include <utility>

namespace bee
{

namespace detail
{
    /// 单个 Signal ↔ Connection 连接的共享状态。
    /// Signal 持有 shared_ptr；Connection 持有 weak_ptr。
    struct SlotState
    {
        std::atomic<bool> active{true};
    };

} // namespace detail

// =============================================================================
// Connection — 轻量、可复制的信号槽连接句柄
// =============================================================================

class Connection
{
public:
    Connection() = default;

    explicit Connection(std::weak_ptr<detail::SlotState> state)
        : state_(std::move(state))
    {
    }

    /// 逻辑断开此槽位。幂等、线程安全。
    auto disconnect() -> void
    {
        if (auto s = state_.lock()) {
            s->active.store(false, std::memory_order_release);
        }
    }

    /// 当拥有此连接的 Signal 仍存活且槽位活跃时返回 true。
    [[nodiscard]] auto connected() const -> bool
    {
        if (auto s = state_.lock()) {
            return s->active.load(std::memory_order_acquire);
        }
        return false;
    }

private:
    std::weak_ptr<detail::SlotState> state_;
};

// =============================================================================
// ScopedConnection — 仅移动的 RAII 包装器，析构时自动断开连接
// =============================================================================

class ScopedConnection
{
public:
    ScopedConnection() = default;

    explicit ScopedConnection(Connection conn)
        : conn_(std::move(conn))
    {
    }

    ~ScopedConnection()
    {
        conn_.disconnect();
    }

    ScopedConnection(ScopedConnection&& other) noexcept
        : conn_(std::move(other.conn_))
    {
    }

    auto operator=(ScopedConnection&& other) noexcept -> ScopedConnection&
    {
        if (this != &other) {
            conn_.disconnect();
            conn_ = std::move(other.conn_);
        }
        return *this;
    }

    ScopedConnection(const ScopedConnection&)                    = delete;
    auto operator=(const ScopedConnection&) -> ScopedConnection& = delete;

    /// 分离内部 Connection，由调用者接管所有权。
    /// release() 之后，此 ScopedConnection 的析构函数不再执行断开操作。
    auto release() -> Connection
    {
        auto released = std::move(conn_);
        return released;
    }

    [[nodiscard]] auto get() const -> const Connection&
    {
        return conn_;
    }

    [[nodiscard]] auto connected() const -> bool
    {
        return conn_.connected();
    }

private:
    Connection conn_;
};

} // namespace bee
