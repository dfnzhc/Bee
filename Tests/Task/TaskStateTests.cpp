/**
 * @File TaskStateTests.cpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2026/4/11
 * @Brief This file is part of Bee.
 */

#include <gtest/gtest.h>

#include "Task/Task.hpp"
#include "Base/Reflection/Nameof.hpp"

namespace
{

using bee::TaskState;

TEST(TaskStateTests, AllValuesExist)
{
    EXPECT_EQ(static_cast<int>(TaskState::Pending), 0);
    EXPECT_EQ(static_cast<int>(TaskState::Running), 1);
    EXPECT_EQ(static_cast<int>(TaskState::Completed), 2);
    EXPECT_EQ(static_cast<int>(TaskState::Cancelled), 3);
    EXPECT_EQ(static_cast<int>(TaskState::Failed), 4);
}

TEST(TaskStateTests, EnumToNameReturnsCorrectStrings)
{
    EXPECT_EQ(bee::enum_to_name(TaskState::Pending), "Pending");
    EXPECT_EQ(bee::enum_to_name(TaskState::Running), "Running");
    EXPECT_EQ(bee::enum_to_name(TaskState::Completed), "Completed");
    EXPECT_EQ(bee::enum_to_name(TaskState::Cancelled), "Cancelled");
    EXPECT_EQ(bee::enum_to_name(TaskState::Failed), "Failed");
}

TEST(TaskStateTests, EnumFromNameRoundTrips)
{
    EXPECT_EQ(bee::enum_from_name<TaskState>("Pending"), TaskState::Pending);
    EXPECT_EQ(bee::enum_from_name<TaskState>("Running"), TaskState::Running);
    EXPECT_EQ(bee::enum_from_name<TaskState>("Completed"), TaskState::Completed);
    EXPECT_EQ(bee::enum_from_name<TaskState>("Cancelled"), TaskState::Cancelled);
    EXPECT_EQ(bee::enum_from_name<TaskState>("Failed"), TaskState::Failed);
}

} // namespace
