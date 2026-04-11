/**
 * @File SignalTests.cpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2026/4/10
 * @Brief This file is part of Bee.
 */

#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <exception>
#include <mutex>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

#include "Event/Signal.hpp"
#include "Concurrency/Thread/ThreadPool.hpp"

namespace
{

using bee::Connection;
using bee::MoveOnlyFunction;
using bee::ScopedConnection;
using bee::Signal;
using bee::ThreadPool;

// =============================================================================
// Basic functionality
// =============================================================================

TEST(SignalTest, ConnectAndEmit)
{
    Signal<void(int)> sig;
    int received = 0;
    sig.connect([&received](int v) {
        received = v;
    });
    sig.emit(42);
    EXPECT_EQ(received, 42);
}

TEST(SignalTest, MultipleSlots)
{
    Signal<void()> sig;
    int count = 0;
    sig.connect([&count]() {
        ++count;
    });
    sig.connect([&count]() {
        ++count;
    });
    sig.connect([&count]() {
        ++count;
    });
    sig.emit();
    EXPECT_EQ(count, 3);
}

TEST(SignalTest, EmptySignalEmitIsNoOp)
{
    Signal<void(int)> sig;
    sig.emit(42); // should not crash
}

TEST(SignalTest, SlotCountAccuracy)
{
    Signal<void()> sig;
    EXPECT_EQ(sig.slot_count(), 0u);
    EXPECT_TRUE(sig.empty());

    auto c1 = sig.connect([]() {
    });
    auto c2 = sig.connect([]() {
    });
    EXPECT_EQ(sig.slot_count(), 2u);
    EXPECT_FALSE(sig.empty());

    c1.disconnect();
    EXPECT_EQ(sig.slot_count(), 1u);
}

// =============================================================================
// Priority ordering
// =============================================================================

TEST(SignalTest, PriorityOrdering)
{
    Signal<void()> sig;
    std::string order;

    sig.connect([&order]() {
        order += "B";
    }, 10);
    sig.connect([&order]() {
        order += "A";
    }, 1);
    sig.connect([&order]() {
        order += "C";
    }, 100);

    sig.emit();
    EXPECT_EQ(order, "ABC");
}

TEST(SignalTest, StablePriorityOrdering)
{
    Signal<void()> sig;
    std::string order;

    sig.connect([&order]() {
        order += "1";
    }, 0);
    sig.connect([&order]() {
        order += "2";
    }, 0);
    sig.connect([&order]() {
        order += "3";
    }, 0);

    sig.emit();
    EXPECT_EQ(order, "123");
}

// =============================================================================
// Disconnect
// =============================================================================

TEST(SignalTest, DisconnectPreventsCallback)
{
    Signal<void()> sig;
    int count = 0;
    auto conn = sig.connect([&count]() {
        ++count;
    });
    conn.disconnect();
    sig.emit();
    EXPECT_EQ(count, 0);
}

TEST(SignalTest, DisconnectAll)
{
    Signal<void()> sig;
    int count = 0;
    sig.connect([&count]() {
        ++count;
    });
    sig.connect([&count]() {
        ++count;
    });
    sig.disconnect_all();
    sig.emit();
    EXPECT_EQ(count, 0);
}

// =============================================================================
// ScopedConnection integration
// =============================================================================

TEST(SignalTest, ScopedConnectionAutoDisconnect)
{
    Signal<void()> sig;
    int count = 0;
    {
        ScopedConnection scoped(sig.connect([&count]() {
            ++count;
        }));
        sig.emit();
        EXPECT_EQ(count, 1);
    }
    sig.emit();
    EXPECT_EQ(count, 1); // not called again
}

TEST(SignalTest, ScopedConnectionRelease)
{
    Signal<void()> sig;
    int count = 0;
    Connection released;
    {
        ScopedConnection scoped(sig.connect([&count]() {
            ++count;
        }));
        released = scoped.release();
    } // scoped destroyed but released, so slot still active
    sig.emit();
    EXPECT_EQ(count, 1);
    released.disconnect();
}

TEST(SignalTest, ConnectionReflectsSignalDestruction)
{
    Connection conn;
    {
        Signal<void()> sig;
        conn = sig.connect([]() {
        });
        EXPECT_TRUE(conn.connected());
    } // sig destroyed → disconnect_all called → active set to false
    EXPECT_FALSE(conn.connected());
}

// =============================================================================
// Error handling
// =============================================================================

TEST(SignalTest, ExceptionInSlotDoesNotStopOthers)
{
    Signal<void()> sig;
    int count = 0;

    sig.connect([&count]() {
        ++count;
    }, 1);
    sig.connect([]() {
        throw std::runtime_error("boom");
    }, 2);
    sig.connect([&count]() {
        ++count;
    }, 3);

    sig.emit();
    EXPECT_EQ(count, 2); // both non-throwing slots ran
}

TEST(SignalTest, ErrorHandlerReceivesException)
{
    Signal<void()> sig;
    std::exception_ptr captured;
    sig.set_error_handler([&captured](std::exception_ptr ep) {
        captured = ep;
    });
    sig.connect([]() {
        throw std::runtime_error("test error");
    });

    sig.emit();
    ASSERT_TRUE(captured != nullptr);
    try {
        std::rethrow_exception(captured);
    } catch (const std::runtime_error& e) {
        EXPECT_STREQ(e.what(), "test error");
    }
}

// =============================================================================
// Thread safety
// =============================================================================

TEST(SignalTest, ConcurrentConnectAndEmit)
{
    Signal<void()> sig;
    std::atomic<int> total{0};

    constexpr int kThreads   = 8;
    constexpr int kPerThread = 100;
    std::atomic<bool> go{false};
    std::vector<std::thread> threads;
    std::vector<Connection> connections;
    std::mutex conn_mutex;

    // Half the threads connect, half emit
    for (int t = 0; t < kThreads; ++t) {
        threads.emplace_back([&, t]() {
            while (!go.load(std::memory_order_acquire)) {
                std::this_thread::yield();
            }
            if (t % 2 == 0) {
                for (int i = 0; i < kPerThread; ++i) {
                    auto c = sig.connect([&total]() {
                        total.fetch_add(1, std::memory_order_relaxed);
                    });
                    std::lock_guard lock(conn_mutex);
                    connections.push_back(c);
                }
            } else {
                for (int i = 0; i < kPerThread; ++i) {
                    sig.emit();
                }
            }
        });
    }

    go.store(true, std::memory_order_release);
    for (auto& t : threads) {
        t.join();
    }

    // Verify no crashes; exact count varies due to timing
    EXPECT_GE(total.load(), 0);
}

TEST(SignalTest, ConcurrentDisconnectAndEmit)
{
    Signal<void()> sig;
    std::atomic<int> total{0};
    constexpr int kSlots = 200;

    std::vector<Connection> connections;
    for (int i = 0; i < kSlots; ++i) {
        connections.push_back(
                sig.connect([&total]() {
                    total.fetch_add(1, std::memory_order_relaxed);
                }));
    }

    std::atomic<bool> go{false};
    std::thread emitter([&]() {
        while (!go.load(std::memory_order_acquire)) {
            std::this_thread::yield();
        }
        for (int i = 0; i < 500; ++i) {
            sig.emit();
        }
    });

    std::thread disconnector([&]() {
        while (!go.load(std::memory_order_acquire)) {
            std::this_thread::yield();
        }
        for (auto& c : connections) {
            c.disconnect();
        }
    });

    go.store(true, std::memory_order_release);
    emitter.join();
    disconnector.join();

    // No crashes; total is some value between 0 and kSlots*500
    EXPECT_GE(total.load(), 0);
}

// =============================================================================
// Async emit
// =============================================================================

TEST(SignalTest, EmitAsyncDispatchesToThreadPool)
{
    Signal<void(int)> sig;
    std::atomic<int> sum{0};

    sig.connect([&sum](int v) {
        sum.fetch_add(v, std::memory_order_relaxed);
    });
    sig.connect([&sum](int v) {
        sum.fetch_add(v, std::memory_order_relaxed);
    });

    ThreadPool pool(2);
    sig.emit_async(pool, 10);
    pool.shutdown();

    EXPECT_EQ(sum.load(), 20);
}

// =============================================================================
// Move semantics
// =============================================================================

TEST(SignalTest, MoveConstructor)
{
    Signal<void()> sig1;
    int count = 0;
    sig1.connect([&count]() {
        ++count;
    });

    Signal<void()> sig2(std::move(sig1));
    sig2.emit();
    EXPECT_EQ(count, 1);

    // sig1 is moved-from — emit is safe but no-op
    sig1.emit();
    EXPECT_EQ(count, 1);
}

TEST(SignalTest, MoveAssignment)
{
    Signal<void()> sig1;
    Signal<void()> sig2;
    int count = 0;
    sig2.connect([&count]() { ++count; });

    sig1 = std::move(sig2);
    sig1.emit();
    EXPECT_EQ(count, 1);

    // sig2 is moved-from — emit is safe but no-op
    sig2.emit();
    EXPECT_EQ(count, 1);
}

} // namespace
