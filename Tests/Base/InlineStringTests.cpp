/**
 * @File InlineStringTests.cpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2026/6/22
 * @Brief This file is part of Bee.
 */

#include <gtest/gtest.h>

#include "Base/Core/InlineString.hpp"

#include <string>
#include <string_view>

using namespace bee;

// ============================================================================
// Construction
// ============================================================================

TEST(InlineStringTest, DefaultConstructEmpty)
{
    InlineString s;
    EXPECT_TRUE(s.empty());
    EXPECT_EQ(s.size(), 0u);
    EXPECT_EQ(s.view(), "");
}

TEST(InlineStringTest, ConstructFromStringView)
{
    InlineString s(std::string_view("hello"));
    EXPECT_EQ(s.size(), 5u);
    EXPECT_EQ(s.view(), "hello");
}

TEST(InlineStringTest, ConstructFromCString)
{
    InlineString s("world");
    EXPECT_EQ(s.view(), "world");
}

TEST(InlineStringTest, ConstructFromNullCString)
{
    InlineString s(static_cast<const char*>(nullptr));
    EXPECT_TRUE(s.empty());
}

TEST(InlineStringTest, ConstructFromStdString)
{
    std::string  src = "std string content";
    InlineString s(src);
    EXPECT_EQ(s.view(), src);
}

// ============================================================================
// Inline vs Heap
// ============================================================================

TEST(InlineStringTest, ShortStringStaysInline)
{
    std::string  short_str(InlineString::kInlineCapacity, 'x');
    InlineString s(short_str);
    EXPECT_EQ(s.size(), InlineString::kInlineCapacity);
    EXPECT_EQ(s.view(), short_str);
}

TEST(InlineStringTest, LongStringGoesToHeap)
{
    std::string  long_str(InlineString::kInlineCapacity + 1, 'y');
    InlineString s(long_str);
    EXPECT_EQ(s.size(), InlineString::kInlineCapacity + 1);
    EXPECT_EQ(s.view(), long_str);
}

TEST(InlineStringTest, VeryLongString)
{
    std::string  very_long(1024, 'z');
    InlineString s(very_long);
    EXPECT_EQ(s.view(), very_long);
}

// ============================================================================
// Copy and Move
// ============================================================================

TEST(InlineStringTest, CopyConstruct)
{
    InlineString a("original");
    InlineString b(a);
    EXPECT_EQ(a.view(), "original");
    EXPECT_EQ(b.view(), "original");
}

TEST(InlineStringTest, CopyConstructHeap)
{
    std::string  long_str(200, 'a');
    InlineString a(long_str);
    InlineString b(a);
    EXPECT_EQ(a.view(), long_str);
    EXPECT_EQ(b.view(), long_str);
}

TEST(InlineStringTest, MoveConstruct)
{
    InlineString a("moveable");
    InlineString b(std::move(a));
    EXPECT_EQ(b.view(), "moveable");
}

TEST(InlineStringTest, MoveConstructHeap)
{
    std::string  long_str(200, 'b');
    InlineString a(long_str);
    InlineString b(std::move(a));
    EXPECT_EQ(b.view(), long_str);
    EXPECT_TRUE(a.empty());
}

TEST(InlineStringTest, CopyAssign)
{
    InlineString a("first");
    InlineString b("second");
    b = a;
    EXPECT_EQ(b.view(), "first");
}

TEST(InlineStringTest, MoveAssign)
{
    InlineString a("source");
    InlineString b("dest");
    b = std::move(a);
    EXPECT_EQ(b.view(), "source");
}

TEST(InlineStringTest, AssignFromStringView)
{
    InlineString s("old");
    s = std::string_view("new value");
    EXPECT_EQ(s.view(), "new value");
}

// ============================================================================
// Comparison
// ============================================================================

TEST(InlineStringTest, EqualityWithInlineString)
{
    InlineString a("same");
    InlineString b("same");
    InlineString c("diff");
    EXPECT_EQ(a, b);
    EXPECT_NE(a, c);
}

TEST(InlineStringTest, EqualityWithStringView)
{
    InlineString a("test");
    EXPECT_EQ(a, std::string_view("test"));
    EXPECT_NE(a, std::string_view("other"));
}

TEST(InlineStringTest, EqualityWithCString)
{
    InlineString a("cstr");
    EXPECT_EQ(a, "cstr");
    EXPECT_NE(a, "nope");
}

TEST(InlineStringTest, EqualityWithNullCString)
{
    InlineString empty;
    EXPECT_EQ(empty, static_cast<const char*>(nullptr));

    InlineString nonempty("x");
    EXPECT_NE(nonempty, static_cast<const char*>(nullptr));
}

// ============================================================================
// Conversion
// ============================================================================

TEST(InlineStringTest, ImplicitConversionToStringView)
{
    InlineString     s("implicit");
    std::string_view sv = s;
    EXPECT_EQ(sv, "implicit");
}

TEST(InlineStringTest, ExplicitStr)
{
    InlineString s("to_string");
    std::string  str = s.str();
    EXPECT_EQ(str, "to_string");
}

TEST(InlineStringTest, DataReturnsNullTerminated)
{
    InlineString s("null-term");
    EXPECT_EQ(s.data()[s.size()], '\0');
}

TEST(InlineStringTest, DataReturnsNullTerminatedHeap)
{
    std::string  long_str(200, 'n');
    InlineString s(long_str);
    EXPECT_EQ(s.data()[s.size()], '\0');
}
