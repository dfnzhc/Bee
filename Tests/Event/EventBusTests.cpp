/**
 * @File EventBusTests.cpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2026/4/10
 * @Brief This file is part of Bee.
 */

#include <gtest/gtest.h>

#include <atomic>
#include <exception>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

#include "Event/EventBus.hpp"
#include "Concurrency/Thread/ThreadPool.hpp"

namespace
{

using bee::Connection;
using bee::EventBus;
using bee::MoveOnlyFunction;
using bee::ScopedConnection;
using bee::ThreadPool;

// Test event types
struct MouseClick
{
    int x = 0;
    int y = 0;
};

struct KeyPress
{
    int keycode = 0;
};

struct Resize
{
    int width  = 0;
    int height = 0;
};

// =============================================================================
// Basic functionality
// =============================================================================

TEST(EventBusTest, SubscribeAndPublish)
{
    EventBus bus;
    MouseClick received{};
    bus.subscribe<MouseClick>([&received](const MouseClick& e) {
        received = e;
    });
    bus.publish(MouseClick{10, 20});
    EXPECT_EQ(received.x, 10);
    EXPECT_EQ(received.y, 20);
}

TEST(EventBusTest, MultipleEventTypesAreIndependent)
{
    EventBus bus;
    int mouse_count = 0;
    int key_count   = 0;

    bus.subscribe<MouseClick>([&mouse_count](const MouseClick&) {
        ++mouse_count;
    });
    bus.subscribe<KeyPress>([&key_count](const KeyPress&) {
        ++key_count;
    });

    bus.publish(MouseClick{});
    bus.publish(MouseClick{});
    bus.publish(KeyPress{});

    EXPECT_EQ(mouse_count, 2);
    EXPECT_EQ(key_count, 1);
}

TEST(EventBusTest, PriorityOrdering)
{
    EventBus bus;
    std::string order;

    bus.subscribe<MouseClick>([&order](const MouseClick&) {
        order += "B";
    }, 10);
    bus.subscribe<MouseClick>([&order](const MouseClick&) {
        order += "A";
    }, 1);
    bus.subscribe<MouseClick>([&order](const MouseClick&) {
        order += "C";
    }, 100);

    bus.publish(MouseClick{});
    EXPECT_EQ(order, "ABC");
}

TEST(EventBusTest, PublishWithNoSubscribersIsNoOp)
{
    EventBus bus;
    bus.publish(MouseClick{42, 99}); // should not crash
}

// =============================================================================
// Clear
// =============================================================================

TEST(EventBusTest, ClearRemovesOnlyOneType)
{
    EventBus bus;
    int mouse_count = 0;
    int key_count   = 0;

    bus.subscribe<MouseClick>([&mouse_count](const MouseClick&) {
        ++mouse_count;
    });
    bus.subscribe<KeyPress>([&key_count](const KeyPress&) {
        ++key_count;
    });

    bus.clear<MouseClick>();
    bus.publish(MouseClick{});
    bus.publish(KeyPress{});

    EXPECT_EQ(mouse_count, 0);
    EXPECT_EQ(key_count, 1);
}

TEST(EventBusTest, ClearAllRemovesEverything)
{
    EventBus bus;
    int mouse_count = 0;
    int key_count   = 0;

    bus.subscribe<MouseClick>([&mouse_count](const MouseClick&) {
        ++mouse_count;
    });
    bus.subscribe<KeyPress>([&key_count](const KeyPress&) {
        ++key_count;
    });

    bus.clear_all();
    bus.publish(MouseClick{});
    bus.publish(KeyPress{});

    EXPECT_EQ(mouse_count, 0);
    EXPECT_EQ(key_count, 0);
}

// =============================================================================
// ScopedConnection
// =============================================================================

TEST(EventBusTest, DisconnectViaConnectionHandle)
{
    EventBus bus;
    int count = 0;
    auto conn = bus.subscribe<MouseClick>([&count](const MouseClick&) {
        ++count;
    });
    bus.publish(MouseClick{});
    EXPECT_EQ(count, 1);
    conn.disconnect();
    bus.publish(MouseClick{});
    EXPECT_EQ(count, 1); // not called after disconnect
}

TEST(EventBusTest, ScopedConnectionAutoDisconnect)
{
    EventBus bus;
    int count = 0;
    {
        ScopedConnection scoped(
                bus.subscribe<MouseClick>([&count](const MouseClick&) {
                    ++count;
                }));
        bus.publish(MouseClick{});
        EXPECT_EQ(count, 1);
    }
    bus.publish(MouseClick{});
    EXPECT_EQ(count, 1); // not called again
}

// =============================================================================
// Error handling
// =============================================================================

TEST(EventBusTest, ErrorHandlerPropagation)
{
    EventBus bus;
    std::exception_ptr captured;
    bus.set_error_handler([&captured](std::exception_ptr ep) {
        captured = ep;
    });

    bus.subscribe<MouseClick>([](const MouseClick&) {
        throw std::runtime_error("bus error");
    });

    bus.publish(MouseClick{});
    ASSERT_TRUE(captured != nullptr);

    try {
        std::rethrow_exception(captured);
    } catch (const std::runtime_error& e) {
        EXPECT_STREQ(e.what(), "bus error");
    }
}

TEST(EventBusTest, ErrorHandlerAppliesToFutureChannels)
{
    EventBus bus;
    std::exception_ptr captured;
    bus.set_error_handler([&captured](std::exception_ptr ep) {
        captured = ep;
    });

    // Subscribe AFTER setting error handler — new channel inherits it
    bus.subscribe<Resize>([](const Resize&) {
        throw std::runtime_error("late channel error");
    });

    bus.publish(Resize{});
    ASSERT_TRUE(captured != nullptr);
}

// =============================================================================
// Thread safety
// =============================================================================

TEST(EventBusTest, ConcurrentSubscribeAndPublish)
{
    EventBus bus;
    std::atomic<int> total{0};

    constexpr int kThreads   = 8;
    constexpr int kPerThread = 100;
    std::atomic<bool> go{false};
    std::vector<std::thread> threads;

    for (int t = 0; t < kThreads; ++t) {
        threads.emplace_back([&, t]() {
            while (!go.load(std::memory_order_acquire)) {
                std::this_thread::yield();
            }
            if (t % 2 == 0) {
                for (int i = 0; i < kPerThread; ++i) {
                    bus.subscribe<MouseClick>(
                            [&total](const MouseClick&) {
                                total.fetch_add(1, std::memory_order_relaxed);
                            });
                }
            } else {
                for (int i = 0; i < kPerThread; ++i) {
                    bus.publish(MouseClick{});
                }
            }
        });
    }

    go.store(true, std::memory_order_release);
    for (auto& t : threads) {
        t.join();
    }

    EXPECT_GE(total.load(), 0); // no crashes
}

// =============================================================================
// Async
// =============================================================================

TEST(EventBusTest, PublishAsyncViaThreadPool)
{
    EventBus bus;
    std::atomic<int> sum{0};

    bus.subscribe<MouseClick>(
            [&sum](const MouseClick& e) {
                sum.fetch_add(e.x, std::memory_order_relaxed);
            });
    bus.subscribe<MouseClick>(
            [&sum](const MouseClick& e) {
                sum.fetch_add(e.x, std::memory_order_relaxed);
            });

    ThreadPool pool(2);
    bus.publish_async(pool, MouseClick{5, 0});
    pool.shutdown();

    EXPECT_EQ(sum.load(), 10);
}

} // namespace
