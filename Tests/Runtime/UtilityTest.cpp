/**
 * @File LoggerTest.cpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2024/11/14
 * @Brief This file is part of Bee.
 */


#include <gtest/gtest.h>
#include <Utility/Logger.hpp>
#include <Utility/Enum.hpp>
#include <Utility/Format.hpp>

using namespace bee;

TEST(LoggerTest, BasicOutput)
{
    int a         = 1;
    float b       = 2.f;
    std::string c = "3.0";

    LogInfo("Hello from info");
    LogInfo("Hello from info: {} {} {}", a, b, c);

    LogWarn("Hello from warn");
    LogWarn("Hello from warn: {} {} {}", a, b, c);

    LogError("Hello from error");
    LogError("Hello from error: {} {} {}", a, b, c);

    LogError("Hello from fatal");
    LogFatal("Hello from fatal: {} {} {}", a, b, c);
}

namespace {
enum class Color : uint32_t
{
    Red    = 0x1,
    Green  = 0x2,
    Blue   = 0x4,
    Yellow = 0x8,
    // 组合枚举值
    RedGreen   = Red | Green,
    BlueYellow = Blue | Yellow,
    All        = Red | Green | Blue | Yellow
};
} // namespace 

TEST(EnumTest, EnumToString)
{
    EXPECT_EQ(ToString(Color::Red), magic_enum::enum_name(Color::Red));
    EXPECT_EQ(ToString(Color::Green), magic_enum::enum_name(Color::Green));
    EXPECT_EQ(ToString(Color::Blue), magic_enum::enum_name(Color::Blue));
    EXPECT_EQ(ToString(Color::Yellow), magic_enum::enum_name(Color::Yellow));
    EXPECT_EQ(ToString(Color::RedGreen), magic_enum::enum_name(Color::RedGreen));
    EXPECT_EQ(ToString(Color::BlueYellow), magic_enum::enum_name(Color::BlueYellow));
    EXPECT_EQ(ToString(Color::All), magic_enum::enum_name(Color::All));
}

TEST(EnumTest, EnumInSet)
{
    EXPECT_TRUE(InSet(Color::Red, Color::Red));
    EXPECT_FALSE(InSet(Color::Red, Color::Green));
    EXPECT_FALSE(InSet(Color::Red, Color::Blue));
    EXPECT_FALSE(InSet(Color::Red, Color::Yellow));

    EXPECT_TRUE(InSet(Color::RedGreen, Color::Red));
    EXPECT_TRUE(InSet(Color::RedGreen, Color::Green));
    EXPECT_FALSE(InSet(Color::RedGreen, Color::Blue));
    EXPECT_FALSE(InSet(Color::RedGreen, Color::Yellow));

    EXPECT_FALSE(InSet(Color::BlueYellow, Color::Red));
    EXPECT_FALSE(InSet(Color::BlueYellow, Color::Green));
    EXPECT_TRUE(InSet(Color::BlueYellow, Color::Blue));
    EXPECT_TRUE(InSet(Color::BlueYellow, Color::Yellow));

    EXPECT_TRUE(InSet(Color::All, Color::Red));
    EXPECT_TRUE(InSet(Color::All, Color::Green));
    EXPECT_TRUE(InSet(Color::All, Color::Blue));
    EXPECT_TRUE(InSet(Color::All, Color::Yellow));
}

TEST(FormatTest, FormatFormatDesc)
{
    Format format   = Format::R8;
    FormatDesc desc = GetFormatDesc(format);
    EXPECT_EQ(desc.format, format);
    EXPECT_EQ(desc.bytesPerBlock, 1);
    EXPECT_EQ(desc.channelCount, 1);
    EXPECT_EQ(desc.Type, FormatType::Unorm);
    EXPECT_FALSE(desc.isDepth);
    EXPECT_FALSE(desc.isStencil);
    EXPECT_FALSE(desc.isCompressed);
    EXPECT_EQ(desc.compressionRatio.width, 1);
    EXPECT_EQ(desc.compressionRatio.height, 1);
    EXPECT_EQ(desc.numChannelBits[0], 8);
    EXPECT_EQ(desc.numChannelBits[1], 0);
    EXPECT_EQ(desc.numChannelBits[2], 0);
    EXPECT_EQ(desc.numChannelBits[3], 0);
}

TEST(FormatTest, FormatBytesPerBlock)
{
    EXPECT_EQ(GetFormatBytesPerBlock(Format::R8), 1);
    EXPECT_EQ(GetFormatBytesPerBlock(Format::RG8), 2);
    EXPECT_EQ(GetFormatBytesPerBlock(Format::RGBA8), 4);
    EXPECT_EQ(GetFormatBytesPerBlock(Format::BC1), 8);
}

TEST(FormatTest, FormatChannelCount)
{
    EXPECT_EQ(GetFormatChannelCount(Format::R8), 1);
    EXPECT_EQ(GetFormatChannelCount(Format::RG8), 2);
    EXPECT_EQ(GetFormatChannelCount(Format::RGBA8), 4);
    EXPECT_EQ(GetFormatChannelCount(Format::BC1), 3);
}

TEST(FormatTest, FormatFormatType)
{
    EXPECT_EQ(GetFormatType(Format::R8), FormatType::Unorm);
    EXPECT_EQ(GetFormatType(Format::R8I), FormatType::Int);
    EXPECT_EQ(GetFormatType(Format::R8U), FormatType::Uint);
    EXPECT_EQ(GetFormatType(Format::R8S), FormatType::Snorm);
}

TEST(FormatTest, IdentifiesDepthFormats)
{
    EXPECT_TRUE(IsDepthFormat(Format::D16));
    EXPECT_TRUE(IsDepthFormat(Format::D24));
    EXPECT_TRUE(IsDepthFormat(Format::D32));
    EXPECT_FALSE(IsDepthFormat(Format::R8));
}

TEST(FormatTest, IdentifiesStencilFormats)
{
    EXPECT_TRUE(IsStencilFormat(Format::D24S8));
    EXPECT_FALSE(IsStencilFormat(Format::R8));
}

TEST(FormatTest, IdentifiesDepthStencilFormats)
{
    EXPECT_TRUE(IsDepthStencilFormat(Format::D24S8));
    EXPECT_FALSE(IsDepthStencilFormat(Format::R8));
}

TEST(FormatTest, IdentifiesCompressedFormats)
{
    EXPECT_TRUE(IsCompressedFormat(Format::BC1));
    EXPECT_FALSE(IsCompressedFormat(Format::R8));
}

TEST(FormatTest, FormatPixelsPerBlock)
{
    EXPECT_EQ(GetFormatPixelsPerBlock(Format::R8), 1);
    EXPECT_EQ(GetFormatPixelsPerBlock(Format::BC1), 16);
}

TEST(FormatTest, FormatWidthCompressionRatio)
{
    EXPECT_EQ(GetFormatWidthCompressionRatio(Format::R8), 1);
    EXPECT_EQ(GetFormatWidthCompressionRatio(Format::BC1), 4);
}

TEST(FormatTest, FormatHeightCompressionRatio)
{
    EXPECT_EQ(GetFormatHeightCompressionRatio(Format::R8), 1);
    EXPECT_EQ(GetFormatHeightCompressionRatio(Format::BC1), 4);
}

TEST(FormatTest, IdentifiesIntegerFormats)
{
    EXPECT_TRUE(IsIntegerFormat(Format::R8I));
    EXPECT_TRUE(IsIntegerFormat(Format::R8U));
}

TEST(FormatTest, FormatNumChannelBits)
{
    EXPECT_EQ(GetNumChannelBits(Format::R8, 0), 8);
    EXPECT_EQ(GetNumChannelBits(Format::RG8, 1), 8);
    EXPECT_EQ(GetNumChannelBits(Format::RGBA8, 3), 8);
}

TEST(FormatTest, FormatChannelMask)
{
    BEE_USE_MAGIC_ENUM_BIT_OPERATOR;
    EXPECT_EQ(GetChannelMask(Format::R8), TextureChannelFlags::Red);
    EXPECT_EQ(GetChannelMask(Format::RG8), TextureChannelFlags::Red | TextureChannelFlags::Green);
    EXPECT_EQ(GetChannelMask(Format::RGBA8),
              TextureChannelFlags::Red | TextureChannelFlags::Green | TextureChannelFlags::Blue | TextureChannelFlags::Alpha);
}

TEST(FormatTest, FormatRowPitch)
{
    EXPECT_EQ(GetFormatRowPitch(Format::R8, 512), 512);
    EXPECT_EQ(GetFormatRowPitch(Format::BC1, 512), 512 / 4 * 8);
}

TEST(FormatTest, ConvertsDepthToColor)
{
    EXPECT_EQ(DepthToColorFormat(Format::D16), Format::R16);
    EXPECT_EQ(DepthToColorFormat(Format::D32F), Format::R32F);
    EXPECT_EQ(DepthToColorFormat(Format::R8), Format::R8);
}

TEST(FormatTest, IdentifiesAlphaChannel)
{
    EXPECT_TRUE(DoesFormatHaveAlpha(Format::RGBA8));
    EXPECT_FALSE(DoesFormatHaveAlpha(Format::BGRX8));
    EXPECT_FALSE(DoesFormatHaveAlpha(Format::R8));
}