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

    LogDebug("Hello from info");
    LogDebug("Hello from info: {} {} {}", a, b, c);

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
    Format format   = Format::R8_Unorm;
    FormatDesc desc = GetFormatDesc(format);
    EXPECT_EQ(desc.format, format);
    EXPECT_EQ(desc.blockSize, 1);
    EXPECT_EQ(desc.isNorm, 1);
    EXPECT_FALSE(desc.isDepth);
    EXPECT_FALSE(desc.isStencil);
    EXPECT_FALSE(desc.isCompressed);
    EXPECT_EQ(desc.widthRatio, 1);
    EXPECT_EQ(desc.heightRatio, 1);
    EXPECT_EQ(desc.redBits, 8);
    EXPECT_EQ(desc.greenBits, 0);
    EXPECT_EQ(desc.blueBits, 0);
    EXPECT_EQ(desc.alphaBits, 0);
}

TEST(FormatTest, FormatBytesPerBlock)
{
    EXPECT_EQ(GetFormatBytesPerBlock(Format::R8_Unorm), 1);
    EXPECT_EQ(GetFormatBytesPerBlock(Format::RG8_Unorm), 2);
    EXPECT_EQ(GetFormatBytesPerBlock(Format::RGBA8_Unorm), 4);
    EXPECT_EQ(GetFormatBytesPerBlock(Format::BC1_Unorm), 8);
}

TEST(FormatTest, FormatChannelCount)
{
    EXPECT_EQ(GetFormatChannelCount(Format::R8_Unorm), 1);
    EXPECT_EQ(GetFormatChannelCount(Format::RG8_Unorm), 2);
    EXPECT_EQ(GetFormatChannelCount(Format::RGBA8_Unorm), 4);
    EXPECT_EQ(GetFormatChannelCount(Format::BC1_Unorm), 4);
}

TEST(FormatTest, IdentifiesDepthFormats)
{
    EXPECT_TRUE(IsDepthFormat(Format::D16_Unorm));
    EXPECT_FALSE(IsDepthFormat(Format::R8_Unorm));
}

TEST(FormatTest, IdentifiesStencilFormats)
{
    EXPECT_TRUE(IsStencilFormat(Format::D24_Unorm_S8_Uint));
    EXPECT_FALSE(IsStencilFormat(Format::R8_Unorm));
}

TEST(FormatTest, IdentifiesDepthStencilFormats)
{
    EXPECT_TRUE(IsDepthStencilFormat(Format::D24_Unorm_S8_Uint));
    EXPECT_FALSE(IsDepthStencilFormat(Format::R8_Unorm));
}

TEST(FormatTest, IdentifiesCompressedFormats)
{
    EXPECT_TRUE(IsCompressedFormat(Format::BC1_Unorm));
    EXPECT_FALSE(IsCompressedFormat(Format::R8_Unorm));
}

TEST(FormatTest, FormatPixelsPerBlock)
{
    EXPECT_EQ(GetFormatPixelsPerBlock(Format::R8_Unorm), 1);
    EXPECT_EQ(GetFormatPixelsPerBlock(Format::BC1_Unorm), 16);
}

TEST(FormatTest, FormatWidthCompressionRatio)
{
    EXPECT_EQ(GetFormatWidthCompressionRatio(Format::R8_Unorm), 1);
    EXPECT_EQ(GetFormatWidthCompressionRatio(Format::BC1_Unorm), 4);
}

TEST(FormatTest, FormatHeightCompressionRatio)
{
    EXPECT_EQ(GetFormatHeightCompressionRatio(Format::R8_Unorm), 1);
    EXPECT_EQ(GetFormatHeightCompressionRatio(Format::BC1_Unorm), 4);
}

TEST(FormatTest, IdentifiesIntegerFormats)
{
    EXPECT_TRUE(IsIntegerFormat(Format::R8_Int));
    EXPECT_TRUE(IsIntegerFormat(Format::R8_Uint));
}

TEST(FormatTest, FormatNumChannelBits)
{
    EXPECT_EQ(GetNumChannelBits(Format::R8_Unorm, 0), 8);
    EXPECT_EQ(GetNumChannelBits(Format::RG8_Unorm, 1), 8);
    EXPECT_EQ(GetNumChannelBits(Format::RGBA8_Unorm, 3), 8);
}

TEST(FormatTest, FormatChannelMask)
{
    BEE_USE_MAGIC_ENUM_BIT_OPERATOR;
    EXPECT_EQ(GetChannelMask(Format::R8_Unorm), TextureChannelFlags::Red);
    EXPECT_EQ(GetChannelMask(Format::RG8_Unorm), TextureChannelFlags::Red | TextureChannelFlags::Green);
    EXPECT_EQ(GetChannelMask(Format::RGBA8_Unorm), TextureChannelFlags::Red | TextureChannelFlags::Green | TextureChannelFlags::Blue | TextureChannelFlags::Alpha);
}

TEST(FormatTest, FormatRowPitch)
{
    EXPECT_EQ(GetFormatRowPitch(Format::R8_Unorm, 512), 512);
    EXPECT_EQ(GetFormatRowPitch(Format::BC1_Unorm, 512), 512 / 4 * 8);
}

TEST(FormatTest, ConvertsDepthToColor)
{
    EXPECT_EQ(DepthToColorFormat(Format::D16_Unorm), Format::R16_Unorm);
    EXPECT_EQ(DepthToColorFormat(Format::D32_Float), Format::R32_Float);
    EXPECT_EQ(DepthToColorFormat(Format::R8_Unorm), Format::R8_Unorm);
}

TEST(FormatTest, IdentifiesAlphaChannel)
{
    EXPECT_TRUE(DoesFormatHaveAlpha(Format::RGBA8_Unorm));
    EXPECT_FALSE(DoesFormatHaveAlpha(Format::BGRX8_Unorm));
    EXPECT_FALSE(DoesFormatHaveAlpha(Format::R8_Unorm));
}

TEST(FormatTest, IdentifiesSrgbFormats)
{
    EXPECT_TRUE(IsSrgbFormat(Format::BC1_Srgb));
    EXPECT_TRUE(IsSrgbFormat(Format::BC2_Srgb));
    EXPECT_TRUE(IsSrgbFormat(Format::BC3_Srgb));
    EXPECT_TRUE(IsSrgbFormat(Format::BGRA8_Srgb));
    EXPECT_TRUE(IsSrgbFormat(Format::BGRX8_Srgb));
    EXPECT_TRUE(IsSrgbFormat(Format::RGBA8_Srgb));
    EXPECT_TRUE(IsSrgbFormat(Format::BC7_Srgb));

    EXPECT_FALSE(IsSrgbFormat(Format::R8_Unorm));
    EXPECT_FALSE(IsSrgbFormat(Format::RG8_Unorm));
    EXPECT_FALSE(IsSrgbFormat(Format::RGBA8_Unorm));
    EXPECT_FALSE(IsSrgbFormat(Format::BC1_Unorm));
    EXPECT_FALSE(IsSrgbFormat(Format::BC2_Unorm));
    EXPECT_FALSE(IsSrgbFormat(Format::BC3_Unorm));
    EXPECT_FALSE(IsSrgbFormat(Format::BGRA8_Unorm));
    EXPECT_FALSE(IsSrgbFormat(Format::BGRX8_Unorm));
    EXPECT_FALSE(IsSrgbFormat(Format::BC7_Unorm));
}

TEST(FormatTest, ConvertsSrgbToLinear)
{
    // 测试已知的 sRGB 到线性格式的转换
    EXPECT_EQ(SrgbToLinearFormat(Format::BC1_Srgb), Format::BC1_Unorm);
    EXPECT_EQ(SrgbToLinearFormat(Format::BC2_Srgb), Format::BC2_Unorm);
    EXPECT_EQ(SrgbToLinearFormat(Format::BC3_Srgb), Format::BC3_Unorm);
    EXPECT_EQ(SrgbToLinearFormat(Format::BGRA8_Srgb), Format::BGRA8_Unorm);
    EXPECT_EQ(SrgbToLinearFormat(Format::BGRX8_Srgb), Format::BGRX8_Unorm);
    EXPECT_EQ(SrgbToLinearFormat(Format::RGBA8_Srgb), Format::RGBA8_Unorm);
    EXPECT_EQ(SrgbToLinearFormat(Format::BC7_Srgb), Format::BC7_Unorm);

    EXPECT_EQ(SrgbToLinearFormat(Format::R8_Unorm), Format::R8_Unorm);
    EXPECT_EQ(SrgbToLinearFormat(Format::RG8_Unorm), Format::RG8_Unorm);
    EXPECT_EQ(SrgbToLinearFormat(Format::RGBA8_Unorm), Format::RGBA8_Unorm);
    EXPECT_EQ(SrgbToLinearFormat(Format::BC1_Unorm), Format::BC1_Unorm);
    EXPECT_EQ(SrgbToLinearFormat(Format::BC2_Unorm), Format::BC2_Unorm);
    EXPECT_EQ(SrgbToLinearFormat(Format::BC3_Unorm), Format::BC3_Unorm);
    EXPECT_EQ(SrgbToLinearFormat(Format::BGRA8_Unorm), Format::BGRA8_Unorm);
    EXPECT_EQ(SrgbToLinearFormat(Format::BGRX8_Unorm), Format::BGRX8_Unorm);
    EXPECT_EQ(SrgbToLinearFormat(Format::BC7_Unorm), Format::BC7_Unorm);
}

TEST(FormatTest, ConvertsLinearToSrgb)
{
    EXPECT_EQ(LinearToSrgbFormat(Format::BC1_Unorm), Format::BC1_Srgb);
    EXPECT_EQ(LinearToSrgbFormat(Format::BC2_Unorm), Format::BC2_Srgb);
    EXPECT_EQ(LinearToSrgbFormat(Format::BC3_Unorm), Format::BC3_Srgb);
    EXPECT_EQ(LinearToSrgbFormat(Format::BGRA8_Unorm), Format::BGRA8_Srgb);
    EXPECT_EQ(LinearToSrgbFormat(Format::BGRX8_Unorm), Format::BGRX8_Srgb);
    EXPECT_EQ(LinearToSrgbFormat(Format::RGBA8_Unorm), Format::RGBA8_Srgb);
    EXPECT_EQ(LinearToSrgbFormat(Format::BC7_Unorm), Format::BC7_Srgb);

    EXPECT_EQ(LinearToSrgbFormat(Format::R8_Unorm), Format::R8_Unorm);
    EXPECT_EQ(LinearToSrgbFormat(Format::RG8_Unorm), Format::RG8_Unorm);
    EXPECT_EQ(LinearToSrgbFormat(Format::RGBA8_Unorm), Format::RGBA8_Srgb);
    EXPECT_EQ(LinearToSrgbFormat(Format::BC1_Srgb), Format::BC1_Srgb);
    EXPECT_EQ(LinearToSrgbFormat(Format::BC2_Srgb), Format::BC2_Srgb);
    EXPECT_EQ(LinearToSrgbFormat(Format::BC3_Srgb), Format::BC3_Srgb);
    EXPECT_EQ(LinearToSrgbFormat(Format::BGRA8_Srgb), Format::BGRA8_Srgb);
    EXPECT_EQ(LinearToSrgbFormat(Format::BGRX8_Srgb), Format::BGRX8_Srgb);
    EXPECT_EQ(LinearToSrgbFormat(Format::BC7_Srgb), Format::BC7_Srgb);
}