/**
 * @File ConnectionTests.cpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2026/4/10
 * @Brief This file is part of Bee.
 */

#include <gtest/gtest.h>

#include <memory>

#include "Event/Connection.hpp"

namespace
{

using bee::Connection;
using bee::ScopedConnection;
using bee::detail::SlotState;

// Helper: create a connected Connection backed by a live SlotState.
auto make_connection() -> std::pair<std::shared_ptr<SlotState>, Connection>
{
    auto state = std::make_shared<SlotState>();
    return {state, Connection(state)};
}

// =============================================================================
// Connection
// =============================================================================

TEST(ConnectionTest, DefaultConstructedNotConnected)
{
    Connection conn;
    EXPECT_FALSE(conn.connected());
}

TEST(ConnectionTest, ConnectedWhenStateAlive)
{
    auto [state, conn] = make_connection();
    EXPECT_TRUE(conn.connected());
}

TEST(ConnectionTest, DisconnectSetsInactive)
{
    auto [state, conn] = make_connection();
    conn.disconnect();
    EXPECT_FALSE(conn.connected());
    EXPECT_FALSE(state->active.load());
}

TEST(ConnectionTest, DisconnectIsIdempotent)
{
    auto [state, conn] = make_connection();
    conn.disconnect();
    conn.disconnect(); // no crash, no UB
    EXPECT_FALSE(conn.connected());
}

TEST(ConnectionTest, ExpiredStateReportsNotConnected)
{
    Connection conn;
    {
        auto state = std::make_shared<SlotState>();
        conn       = Connection(state);
        EXPECT_TRUE(conn.connected());
    } // state destroyed
    EXPECT_FALSE(conn.connected());
}

TEST(ConnectionTest, DisconnectOnExpiredStateIsNoOp)
{
    Connection conn;
    {
        auto state = std::make_shared<SlotState>();
        conn       = Connection(state);
    }
    conn.disconnect(); // no crash
    EXPECT_FALSE(conn.connected());
}

TEST(ConnectionTest, CopySharesState)
{
    auto [state, conn1] = make_connection();
    auto conn2          = conn1;
    EXPECT_TRUE(conn2.connected());
    conn1.disconnect();
    EXPECT_FALSE(conn2.connected()); // same underlying state
}

// =============================================================================
// ScopedConnection
// =============================================================================

TEST(ScopedConnectionTest, AutoDisconnectOnDestruction)
{
    auto [state, conn] = make_connection();
    {
        ScopedConnection scoped(conn);
        EXPECT_TRUE(scoped.connected());
    } // ~ScopedConnection calls disconnect
    EXPECT_FALSE(state->active.load());
}

TEST(ScopedConnectionTest, MoveTransfersOwnership)
{
    auto [state, conn] = make_connection();
    ScopedConnection scoped1(conn);
    ScopedConnection scoped2(std::move(scoped1));
    EXPECT_TRUE(scoped2.connected());
    // scoped1 destructor is safe (moved-from state)
}

TEST(ScopedConnectionTest, MoveAssignDisconnectsOld)
{
    auto [state1, conn1] = make_connection();
    auto [state2, conn2] = make_connection();

    ScopedConnection scoped1(conn1);
    ScopedConnection scoped2(conn2);
    scoped1 = std::move(scoped2);

    EXPECT_FALSE(state1->active.load()); // old connection disconnected
    EXPECT_TRUE(state2->active.load());  // new connection still active
}

TEST(ScopedConnectionTest, ReleaseDetachesConnection)
{
    auto [state, conn] = make_connection();
    ScopedConnection scoped(conn);
    auto             released = scoped.release();
    // scoped destructor does NOT disconnect (released)
    EXPECT_TRUE(released.connected());
    EXPECT_TRUE(state->active.load());
}

TEST(ScopedConnectionTest, DefaultConstructedIsSafe)
{
    ScopedConnection scoped;
    EXPECT_FALSE(scoped.connected());
    // destructor safe on empty connection
}

TEST(ScopedConnectionTest, GetReturnsInnerConnection)
{
    auto [state, conn] = make_connection();
    ScopedConnection scoped(conn);
    EXPECT_TRUE(scoped.get().connected());
}

} // namespace
