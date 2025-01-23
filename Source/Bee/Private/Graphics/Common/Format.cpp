#include "Graphics/Common/Format.hpp"

using namespace bee;

#define BN_1  0
#define BN_2  BN_1, BN_1
#define BN_3  BN_1, BN_2
#define BN_4  BN_2, BN_2
#define BN_5  BN_2, BN_3
#define BN_6  BN_3, BN_3
#define BN_7  BN_3, BN_4
#define BN_8  BN_4, BN_4
#define BN_9  BN_4, BN_5
#define BN_10 BN_5, BN_5

#define isNorm       1
#define isSigned     1
#define isInteger    1
#define isFloat      1
#define isSrgb       1
#define isBgr        1
#define isDepth      1
#define isStencil    1
#define isCompressed 1

// clang-format off
#define BF_Unorm   isNorm,       BN_1,        BN_1,     BN_1 
#define BF_Int       BN_1,   isSigned,   isInteger,     BN_1 
#define BF_Uint      BN_1,       BN_1,   isInteger,     BN_1 
#define BF_Snorm   isNorm,   isSigned,        BN_1,     BN_1 
#define BF_Float     BN_1,   isSigned,        BN_1,  isFloat
// clang-format on

namespace {
const FormatDesc kFormatDesc[] = {
  // clang-format off
    // Format | BytesPerBlock | ChannelCount | Type | {bDepth bStencil bCompressed} | {CompressionRatio.Width, CompressionRatio.Height} | {numChannelBits.x, numChannelBits.y, numChannelBits.z, numChannelBits.w}
    // R bits | G bits | B bits | A bits | block size | width ratio | height ratio |
    // isNorm | isSigned | isInteger | isFloat | isSrgb | isBgr | isDepth | isStencil | isCompressed | unused         : 9;
    {Format::Unknown,   0, 0, 0, 0, 1, 0, 0, BN_10 },
    {Format::R8_Unorm,  8, 0, 0, 0, 1, 1, 1, BF_Unorm, BN_6 },
    {Format::R8_Int,    8, 0, 0, 0, 1, 1, 1, BF_Int,   BN_6 },
    {Format::R8_Uint,   8, 0, 0, 0, 1, 1, 1, BF_Uint,  BN_6 },
    {Format::R8_Snorm,  8, 0, 0, 0, 1, 1, 1, BF_Snorm, BN_6 },

    {Format::R16_Unorm, 16, 0, 0, 0, 2, 1, 1, BF_Unorm, BN_6 },
    {Format::R16_Int,   16, 0, 0, 0, 2, 1, 1, BF_Int,   BN_6 },
    {Format::R16_Uint,  16, 0, 0, 0, 2, 1, 1, BF_Uint,  BN_6 },
    {Format::R16_Float, 16, 0, 0, 0, 2, 1, 1, BF_Float, BN_6 },
    {Format::R16_Snorm, 16, 0, 0, 0, 2, 1, 1, BF_Snorm, BN_6 },

    {Format::R32_Int,   32, 0, 0, 0, 4, 1, 1, BF_Int,   BN_6 },
    {Format::R32_Uint,  32, 0, 0, 0, 4, 1, 1, BF_Uint,  BN_6 },
    {Format::R32_Float, 32, 0, 0, 0, 4, 1, 1, BF_Float, BN_6 },

    {Format::RG8_Unorm, 8, 8, 0, 0, 2, 1, 1,  BF_Unorm, BN_6 },
    {Format::RG8_Int,   8, 8, 0, 0, 2, 1, 1,  BF_Int,   BN_6 },
    {Format::RG8_Uint,  8, 8, 0, 0, 2, 1, 1,  BF_Uint,  BN_6 },
    {Format::RG8_Snorm, 8, 8, 0, 0, 2, 1, 1,  BF_Snorm, BN_6 },

    {Format::RG16_Unorm, 16, 16, 0, 0, 4, 1, 1, BF_Unorm, BN_6 },
    {Format::RG16_Int,   16, 16, 0, 0, 4, 1, 1, BF_Int,   BN_6 },
    {Format::RG16_Uint,  16, 16, 0, 0, 4, 1, 1, BF_Uint,  BN_6 },
    {Format::RG16_Float, 16, 16, 0, 0, 4, 1, 1, BF_Float, BN_6 },
    {Format::RG16_Snorm, 16, 16, 0, 0, 4, 1, 1, BF_Snorm, BN_6 },

    {Format::RG32_Int,   32, 32, 0, 0, 8, 1, 1, BF_Int,   BN_6 },
    {Format::RG32_Uint,  32, 32, 0, 0, 8, 1, 1, BF_Uint,  BN_6 },
    {Format::RG32_Float, 32, 32, 0, 0, 8, 1, 1, BF_Float, BN_6 },

    {Format::RGB8_Unorm, 8, 8, 8, 0, 3, 1, 1, BF_Unorm, BN_6 },
    {Format::RGB8_Int,   8, 8, 8, 0, 3, 1, 1, BF_Int,   BN_6 },
    {Format::RGB8_Uint,  8, 8, 8, 0, 3, 1, 1, BF_Uint,  BN_6 },
    {Format::RGB8_Snorm, 8, 8, 8, 0, 3, 1, 1, BF_Snorm, BN_6 },

    {Format::RGBA8_Unorm, 8, 8, 8, 8, 4, 1, 1, BF_Unorm, BN_6 },
    {Format::RGBA8_Int,   8, 8, 8, 8, 4, 1, 1, BF_Int,   BN_6 },
    {Format::RGBA8_Uint,  8, 8, 8, 8, 4, 1, 1, BF_Uint,  BN_6 },
    {Format::RGBA8_Snorm, 8, 8, 8, 8, 4, 1, 1, BF_Snorm, BN_6 },
    {Format::RGBA8_Srgb,  8, 8, 8, 8, 4, 1, 1, BN_4,     isSrgb, BN_5 },

    {Format::BGRA8_Unorm, 8, 8, 8, 8, 4, 1, 1, BF_Unorm, BN_1,   isBgr, BN_4 },
    {Format::BGRA8_Srgb,  8, 8, 8, 8, 4, 1, 1, BN_4,     isSrgb, isBgr, BN_4 },

    {Format::RGBA16_Unorm, 16, 16, 16, 16, 8, 1, 1,  BF_Unorm, BN_6 },
    {Format::RGBA16_Int,   16, 16, 16, 16, 8, 1, 1,  BF_Int,   BN_6 },
    {Format::RGBA16_Uint,  16, 16, 16, 16, 8, 1, 1,  BF_Uint,  BN_6 },
    {Format::RGBA16_Float, 16, 16, 16, 16, 8, 1, 1,  BF_Float, BN_6 },
    {Format::RGBA16_Snorm, 16, 16, 16, 16, 8, 1, 1,  BF_Snorm, BN_6 },

    {Format::RGB32_Int,   32, 32, 32, 0, 12, 1, 1, BF_Int,   BN_6 },
    {Format::RGB32_Uint,  32, 32, 32, 0, 12, 1, 1, BF_Uint,  BN_6 },
    {Format::RGB32_Float, 32, 32, 32, 0, 12, 1, 1, BF_Float, BN_6 },

    {Format::RGBA32_Int,   32, 32, 32, 32, 16, 1, 1, BF_Int,   BN_6 },
    {Format::RGBA32_Uint,  32, 32, 32, 32, 16, 1, 1, BF_Uint,  BN_6 },
    {Format::RGBA32_Float, 32, 32, 32, 32, 16, 1, 1, BF_Float, BN_6 },

    {Format::R5G6B5_Unorm,  5, 6, 5, 0, 2, 1, 1, BF_Unorm, BN_6 },
    {Format::RGB5A1_Unorm,  5, 5, 5, 1, 2, 1, 1, BF_Unorm, BN_6 },
    {Format::B5G6R5_Unorm,  5, 6, 5, 0, 2, 1, 1, BF_Unorm, BN_1, isBgr, BN_4 },
    {Format::BGR5A1_Unorm,  5, 5, 5, 1, 2, 1, 1, BF_Unorm, BN_1, isBgr, BN_4 },

    {Format::RGB10A2_Unorm, 10, 10, 10, 2, 4, 1, 1, BF_Unorm, BN_6 },
    {Format::RGB10A2_Uint,  10, 10, 10, 2, 4, 1, 1, BF_Uint,  BN_6 },
    {Format::RG11B10_Float, 11, 11, 10, 0, 4, 1, 1, BF_Float, BN_6 },

    {Format::RGBA4_Unorm,   4, 4, 4, 4, 2, 1, 1, BF_Unorm, BN_6 },
    {Format::BGRA4_Unorm,   4, 4, 4, 4, 2, 1, 1, BF_Unorm, BN_1,   isBgr, BN_4 },
    {Format::BGRX8_Unorm,   8, 8, 8, 8, 4, 1, 1, BF_Unorm, BN_1,   isBgr, BN_4 },
    {Format::BGRX8_Srgb,    8, 8, 8, 8, 4, 1, 1, BF_Unorm, isSrgb, isBgr, BN_4 },
    {Format::RGB9E5_Float,  9, 9, 9, 5, 4, 1, 1, BF_Float, BN_6 },

    {Format::D16_Unorm,         16, 0, 0, 0, 2, 1, 1, BF_Unorm, BN_2,                            isDepth,            BN_3 },
    {Format::D24_Unorm_S8_Uint, 24, 8, 0, 0, 4, 1, 1, isNorm,   BN_1,  isInteger,          BN_3, isDepth, isStencil, BN_2 },
    {Format::D32_Float,         32, 0, 0, 0, 4, 1, 1, BF_Float, BN_2,                            isDepth,            BN_3 },
    {Format::D32_Float_S8_Uint, 32, 8, 0, 0, 8, 1, 1, BN_1 , isSigned, isInteger, isFloat, BN_2, isDepth, isStencil, BN_2 },
    
    {Format::BC1_Unorm,  5,  6,  5, 1,  8, 4, 4, BF_Unorm,  BN_4,          isCompressed, BN_1 },
    {Format::BC1_Srgb,   5,  6,  5, 1,  8, 4, 4, BN_4,      isSrgb,  BN_3, isCompressed, BN_1 },
    {Format::BC2_Unorm,  5,  6,  5, 4, 16, 4, 4, BF_Unorm,  BN_4,          isCompressed, BN_1 },
    {Format::BC2_Srgb,   5,  6,  5, 4, 16, 4, 4, BN_4,      isSrgb,  BN_3, isCompressed, BN_1 },
    {Format::BC3_Unorm,  5,  6,  5, 8, 16, 4, 4, BF_Unorm,  BN_4,          isCompressed, BN_1 },
    {Format::BC3_Srgb,   5,  6,  5, 8, 16, 4, 4, BN_4,      isSrgb,  BN_3, isCompressed, BN_1 },
    {Format::BC4_Unorm,  8,  0,  0, 0,  8, 4, 4, BF_Unorm,  BN_4,          isCompressed, BN_1 },
    {Format::BC4_Snorm,  8,  0,  0, 0,  8, 4, 4, BF_Snorm,  BN_4,          isCompressed, BN_1 },
    {Format::BC5_Unorm,  8,  8,  0, 0, 16, 4, 4, BF_Unorm,  BN_4,          isCompressed, BN_1 },
    {Format::BC5_Snorm,  8,  8,  0, 0, 16, 4, 4, BF_Snorm,  BN_4,          isCompressed, BN_1 },
    {Format::BC6H_UF16, 16, 16, 16, 0, 16, 4, 4, BN_3,      isFloat, BN_4, isCompressed, BN_1 },
    {Format::BC6H_SF16, 16, 16, 16, 0, 16, 4, 4, isFloat,   BN_4,          isCompressed, BN_1 },
    {Format::BC7_Unorm,  8,  8,  8, 8, 16, 4, 4, BF_Unorm,  BN_4,          isCompressed, BN_1 },
    {Format::BC7_Srgb,   8,  8,  8, 8, 16, 4, 4, BN_4,      isSrgb,  BN_3, isCompressed, BN_1 },
  // clang-format on
};

static_assert(std::size(kFormatDesc) == me::enum_count<Format>(), "Format desc table has a wrong size");
} // namespace

#undef BN_1
#undef BN_2
#undef BN_3
#undef BN_4
#undef BN_5
#undef BN_6
#undef BN_7
#undef BN_8
#undef BN_9
#undef BN_1

#undef isNorm
#undef isSigned
#undef isInteger
#undef isFloat
#undef isSrgb
#undef isBgr
#undef isDepth
#undef isStencil
#undef isCompressed

#undef BF_Unorm
#undef BF_Int
#undef BF_Uint
#undef BF_Snorm
#undef BF_Float

namespace bee {
constexpr FormatDesc GetFormatDesc(Format format)
{
    BEE_DEBUG_ASSERT(me::enum_integer(format) < me::enum_count<Format>());

    return kFormatDesc[me::enum_integer(format)];
}

constexpr u32 GetFormatBytesPerBlock(Format format)
{
    BEE_DEBUG_ASSERT(kFormatDesc[me::enum_integer(format)].format == format);

    return kFormatDesc[me::enum_integer(format)].blockSize;
}

constexpr u32 GetFormatChannelCount(Format format)
{
    BEE_DEBUG_ASSERT(kFormatDesc[me::enum_integer(format)].format == format);
    const auto& desc = kFormatDesc[me::enum_integer(format)];
    return !!(desc.redBits) + !!(desc.greenBits) + !!(desc.blueBits) + !!(desc.alphaBits);
}

constexpr bool IsDepthFormat(Format format)
{
    BEE_DEBUG_ASSERT(kFormatDesc[me::enum_integer(format)].format == format);
    return kFormatDesc[me::enum_integer(format)].isDepth;
}

constexpr bool IsStencilFormat(Format format)
{
    BEE_DEBUG_ASSERT(kFormatDesc[me::enum_integer(format)].format == format);
    return kFormatDesc[me::enum_integer(format)].isStencil;
}

constexpr bool IsDepthStencilFormat(Format format)
{
    return IsDepthFormat(format) || IsStencilFormat(format);
}

constexpr bool IsCompressedFormat(Format format)
{
    BEE_DEBUG_ASSERT(kFormatDesc[me::enum_integer(format)].format == format);
    return kFormatDesc[me::enum_integer(format)].isCompressed;
}

constexpr u32 GetFormatPixelsPerBlock(Format format)
{
    BEE_DEBUG_ASSERT(kFormatDesc[me::enum_integer(format)].format == format);
    return GetFormatWidthCompressionRatio(format) * GetFormatHeightCompressionRatio(format);
}

constexpr u32 GetFormatWidthCompressionRatio(Format format)
{
    BEE_DEBUG_ASSERT(kFormatDesc[me::enum_integer(format)].format == format);
    return kFormatDesc[me::enum_integer(format)].widthRatio;
}

constexpr u32 GetFormatHeightCompressionRatio(Format format)
{
    BEE_DEBUG_ASSERT(kFormatDesc[me::enum_integer(format)].format == format);
    return kFormatDesc[me::enum_integer(format)].heightRatio;
}

constexpr bool IsIntegerFormat(Format format)
{
    BEE_DEBUG_ASSERT(kFormatDesc[me::enum_integer(format)].format == format);
    return kFormatDesc[me::enum_integer(format)].isInteger;
}

constexpr u32 GetNumChannelBits(Format format, int channel)
{
    BEE_DEBUG_ASSERT(channel < 4);
    BEE_DEBUG_ASSERT(kFormatDesc[me::enum_integer(format)].format == format);
    return (&(kFormatDesc[me::enum_integer(format)].redBits))[channel];
}

constexpr u32 GetNumChannelBits(Format format, TextureChannelFlags mask)
{
    BEE_USE_MAGIC_ENUM_BIT_OPERATOR;

    u32 bits = 0;
    if (InSet(mask, TextureChannelFlags::Red))
        bits += GetNumChannelBits(format, 0);
    if (InSet(mask, TextureChannelFlags::Green))
        bits += GetNumChannelBits(format, 1);
    if (InSet(mask, TextureChannelFlags::Blue))
        bits += GetNumChannelBits(format, 2);
    if (InSet(mask, TextureChannelFlags::Alpha))
        bits += GetNumChannelBits(format, 3);
    return bits;
}

constexpr TextureChannelFlags GetChannelMask(Format format)
{
    BEE_USE_MAGIC_ENUM_BIT_OPERATOR;

    TextureChannelFlags mask = TextureChannelFlags::None;
    if (kFormatDesc[me::enum_integer(format)].redBits > 0)
        mask |= TextureChannelFlags::Red;
    if (kFormatDesc[me::enum_integer(format)].greenBits > 0)
        mask |= TextureChannelFlags::Green;
    if (kFormatDesc[me::enum_integer(format)].blueBits > 0)
        mask |= TextureChannelFlags::Blue;
    if (kFormatDesc[me::enum_integer(format)].alphaBits > 0)
        mask |= TextureChannelFlags::Alpha;
    return mask;
}

constexpr u32 GetFormatRowPitch(Format format, u32 width)
{
    BEE_DEBUG_ASSERT(width % GetFormatWidthCompressionRatio(format) == 0);
    return (width / GetFormatWidthCompressionRatio(format)) * GetFormatBytesPerBlock(format);
}

constexpr Format DepthToColorFormat(Format format)
{
    switch (format) {
    case Format::D16_Unorm : return Format::R16_Unorm;
    case Format::D32_Float : return Format::R32_Float;
    default                : BEE_DEBUG_ASSERT(IsDepthFormat(format) == false); return format;
    }
}

constexpr bool DoesFormatHaveAlpha(Format format)
{
    if (GetFormatChannelCount(format) == 4 && !IsCompressedFormat(format)) {
        switch (format) {
        case Format::BGRX8_Srgb  : [[fallthrough]];
        case Format::BGRX8_Unorm : return false;
        default                  : return true;
        }
    }

    return false;
}

constexpr bool IsSrgbFormat(Format format)
{
    BEE_DEBUG_ASSERT(kFormatDesc[me::enum_integer(format)].format == format);
    return kFormatDesc[me::enum_integer(format)].isSrgb;
}

constexpr Format SrgbToLinearFormat(Format format)
{
    switch (format) {
    case Format::BC1_Srgb   : return Format::BC1_Unorm;
    case Format::BC2_Srgb   : return Format::BC2_Unorm;
    case Format::BC3_Srgb   : return Format::BC3_Unorm;
    case Format::BGRA8_Srgb : return Format::BGRA8_Unorm;
    case Format::BGRX8_Srgb : return Format::BGRX8_Unorm;
    case Format::RGBA8_Srgb : return Format::RGBA8_Unorm;
    case Format::BC7_Srgb   : return Format::BC7_Unorm;
    default                 : BEE_DEBUG_ASSERT(IsSrgbFormat(format) == false); return format;
    }
}

constexpr Format LinearToSrgbFormat(Format format)
{
    switch (format) {
    case Format::BC1_Unorm   : return Format::BC1_Srgb;
    case Format::BC2_Unorm   : return Format::BC2_Srgb;
    case Format::BC3_Unorm   : return Format::BC3_Srgb;
    case Format::BGRA8_Unorm : return Format::BGRA8_Srgb;
    case Format::BGRX8_Unorm : return Format::BGRX8_Srgb;
    case Format::RGBA8_Unorm : return Format::RGBA8_Srgb;
    case Format::BC7_Unorm   : return Format::BC7_Srgb;
    default                  : return format;
    }
}

} // namespace bee