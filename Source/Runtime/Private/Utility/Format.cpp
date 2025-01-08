#include "Utility/Format.hpp"

using namespace bee;

namespace {
const FormatDesc kFormatDesc[] = {
    // clang-format off
    // Format | BytesPerBlock | ChannelCount | Type | {bDepth bStencil bCompressed} | {CompressionRatio.Width, CompressionRatio.Height} | {numChannelBits.x, numChannelBits.y, numChannelBits.z, numChannelBits.w}
    {Format::Unknown,   0,  0, FormatType::Unknown, {false, false, false,}, {1, 1}, {0, 0, 0, 0}},
    {Format::R8,        1,  1, FormatType::Unorm,   {false, false, false,}, {1, 1}, {8, 0, 0, 0}},
    {Format::R8I,       1,  1, FormatType::Int,     {false, false, false,}, {1, 1}, {8, 0, 0, 0}},
    {Format::R8U,       1,  1, FormatType::Uint,    {false, false, false,}, {1, 1}, {8, 0, 0, 0}},
    {Format::R8S,       1,  1, FormatType::Snorm,   {false, false, false,}, {1, 1}, {8, 0, 0, 0}},

    {Format::R16,       2,  1, FormatType::Unorm,   {false, false, false,}, {1, 1}, {16, 0, 0, 0}},
    {Format::R16I,      2,  1, FormatType::Int,     {false, false, false,}, {1, 1}, {16, 0, 0, 0}},
    {Format::R16U,      2,  1, FormatType::Uint,    {false, false, false,}, {1, 1}, {16, 0, 0, 0}},
    {Format::R16F,      2,  1, FormatType::Float,   {false, false, false,}, {1, 1}, {16, 0, 0, 0}},
    {Format::R16S,      2,  1, FormatType::Snorm,   {false, false, false,}, {1, 1}, {16, 0, 0, 0}},

    {Format::R32I,      4,  1, FormatType::Int,     {false, false, false,}, {1, 1}, {32, 0, 0, 0}},
    {Format::R32U,      4,  1, FormatType::Uint,    {false, false, false,}, {1, 1}, {32, 0, 0, 0}},
    {Format::R32F,      4,  1, FormatType::Float,   {false, false, false,}, {1, 1}, {32, 0, 0, 0}},

    {Format::RG8,       2,  2, FormatType::Unorm,   {false, false, false,}, {1, 1}, {8, 8, 0, 0}},
    {Format::RG8I,      2,  2, FormatType::Int,     {false, false, false,}, {1, 1}, {8, 8, 0, 0}},
    {Format::RG8U,      2,  2, FormatType::Uint,    {false, false, false,}, {1, 1}, {8, 8, 0, 0}},
    {Format::RG8S,      2,  2, FormatType::Snorm,   {false, false, false,}, {1, 1}, {8, 8, 0, 0}},

    {Format::RG16,      4,  2, FormatType::Unorm,   {false, false, false,}, {1, 1}, {16, 16, 0, 0}},
    {Format::RG16I,     4,  2, FormatType::Int,     {false, false, false,}, {1, 1}, {16, 16, 0, 0}},
    {Format::RG16U,     4,  2, FormatType::Uint,    {false, false, false,}, {1, 1}, {16, 16, 0, 0}},
    {Format::RG16F,     4,  2, FormatType::Float,   {false, false, false,}, {1, 1}, {16, 16, 0, 0}},
    {Format::RG16S,     4,  2, FormatType::Snorm,   {false, false, false,}, {1, 1}, {16, 16, 0, 0}},

    {Format::RG32I,     8,  2, FormatType::Int,     {false, false, false,}, {1, 1}, {32, 32, 0, 0}},
    {Format::RG32U,     8,  2, FormatType::Uint,    {false, false, false,}, {1, 1}, {32, 32, 0, 0}},
    {Format::RG32F,     8,  2, FormatType::Float,   {false, false, false,}, {1, 1}, {32, 32, 0, 0}},

    {Format::RGB8,      3,  3, FormatType::Unorm,   {false, false, false,}, {1, 1}, {8, 8, 8, 0}},
    {Format::RGB8I,     3,  3, FormatType::Int,     {false, false, false,}, {1, 1}, {8, 8, 8, 0}},
    {Format::RGB8U,     3,  3, FormatType::Uint,    {false, false, false,}, {1, 1}, {8, 8, 8, 0}},
    {Format::RGB8S,     3,  3, FormatType::Snorm,   {false, false, false,}, {1, 1}, {8, 8, 8, 0}},
    {Format::RGB9E5F,   4,  3, FormatType::Float,   {false, false, false,}, {1, 1}, {9, 9, 9, 5}},

    {Format::RGBA8,     4,  4, FormatType::Unorm,   {false, false, false,}, {1, 1}, {8, 8, 8, 8}},
    {Format::RGBA8I,    4,  4, FormatType::Int,     {false, false, false,}, {1, 1}, {8, 8, 8, 8}},
    {Format::RGBA8U,    4,  4, FormatType::Uint,    {false, false, false,}, {1, 1}, {8, 8, 8, 8}},
    {Format::RGBA8S,    4,  4, FormatType::Snorm,   {false, false, false,}, {1, 1}, {8, 8, 8, 8}},
    {Format::BGRA8,     4,  4, FormatType::Unorm,   {false, false, false,}, {1, 1}, {8, 8, 8, 8}},

    {Format::RGBA16,    8,  4, FormatType::Unorm,   {false, false, false,}, {1, 1}, {16, 16, 16, 16}},
    {Format::RGBA16I,   8,  4, FormatType::Int,     {false, false, false,}, {1, 1}, {16, 16, 16, 16}},
    {Format::RGBA16U,   8,  4, FormatType::Uint,    {false, false, false,}, {1, 1}, {16, 16, 16, 16}},
    {Format::RGBA16F,   8,  4, FormatType::Float,   {false, false, false,}, {1, 1}, {16, 16, 16, 16}},
    {Format::RGBA16S,   8,  4, FormatType::Snorm,   {false, false, false,}, {1, 1}, {16, 16, 16, 16}},

    {Format::RGB32I,    12, 3, FormatType::Int,     {false, false, false,}, {1, 1}, {32, 32, 32, 0}},
    {Format::RGB32U,    12, 3, FormatType::Uint,    {false, false, false,}, {1, 1}, {32, 32, 32, 0}},
    {Format::RGB32F,    12, 3, FormatType::Float,   {false, false, false,}, {1, 1}, {32, 32, 32, 0}},

    {Format::RGBA32I,   16, 4, FormatType::Int,     {false, false, false,}, {1, 1}, {32, 32, 32, 32}},
    {Format::RGBA32U,   16, 4, FormatType::Uint,    {false, false, false,}, {1, 1}, {32, 32, 32, 32}},
    {Format::RGBA32F,   16, 4, FormatType::Float,   {false, false, false,}, {1, 1}, {32, 32, 32, 32}},

    {Format::B5G6R5,    2,  3, FormatType::Unorm,   {false, false, false,}, {1, 1}, {5, 6, 5, 0}},
    {Format::R5G6B5,    2,  3, FormatType::Unorm,   {false, false, false,}, {1, 1}, {5, 6, 5, 0}},
    {Format::RGB5A1,    2,  4, FormatType::Unorm,   {false, false, false,}, {1, 1}, {5, 5, 5, 1}},
    {Format::BGR5A1,    2,  4, FormatType::Unorm,   {false, false, false,}, {1, 1}, {5, 5, 5, 1}},
    {Format::RGB10A2,   4,  4, FormatType::Unorm,   {false, false, false,}, {1, 1}, {10, 10, 10, 2}},
    {Format::RGB10A2U,  4,  4, FormatType::Uint,    {false, false, false,}, {1, 1}, {10, 10, 10, 2}},
    {Format::RG11B10F,  4,  3, FormatType::Float,   {false, false, false,}, {1, 1}, {11, 11, 10, 0}},
    {Format::RGBA4,     2,  4, FormatType::Unorm,   {false, false, false,}, {1, 1}, {4, 4, 4, 4}},
    {Format::BGRA4,     2,  4, FormatType::Unorm,   {false, false, false,}, {1, 1}, {4, 4, 4, 4}},
    {Format::BGRX8,     4,  4, FormatType::Unorm,   {false, false, false,}, {1, 1}, {8, 8, 8, 8}},

    {Format::D16,       2,  1, FormatType::Unorm,   {true, false, false,},  {1, 1}, {16, 0, 0, 0}},
    {Format::D16F,      2,  1, FormatType::Float,   {true, false, false,},  {1, 1}, {16, 0, 0, 0}},
    {Format::D24,       3,  1, FormatType::Unorm,   {true, false, false,},  {1, 1}, {24, 0, 0, 0}},
    {Format::D24F,      3,  1, FormatType::Float,   {true, false, false,},  {1, 1}, {24, 0, 0, 0}},
    {Format::D24S8,     4,  2, FormatType::Unorm,   {true, true, false,},   {1, 1}, {24, 8, 0, 0}},
    {Format::D32,       4,  1, FormatType::Unorm,   {true, false, false,},  {1, 1}, {32, 0, 0, 0}},
    {Format::D32F,      4,  1, FormatType::Float,   {true, false, false,},  {1, 1}, {32, 0, 0, 0}},
    {Format::D32FS8U,   4,  1, FormatType::Float,   {true, true, false,},   {1, 1}, {32, 0, 0, 0}},

    {Format::BC1,       8,  3, FormatType::Unorm,   {false, false, true,},  {4, 4}, {64, 0, 0, 0}},
    {Format::BC2,       16, 4, FormatType::Unorm,   {false, false, true,},  {4, 4}, {128, 0, 0, 0}},
    {Format::BC3,       16, 4, FormatType::Unorm,   {false, false, true,},  {4, 4}, {128, 0, 0, 0}},
    {Format::BC4,       8,  1, FormatType::Unorm,   {false, false, true,},  {4, 4}, {64, 0, 0, 0}},
    {Format::BC5,       16, 2, FormatType::Unorm,   {false, false, true,},  {4, 4}, {128, 0, 0, 0}},
    {Format::BC6H,      16, 3, FormatType::Float,   {false, false, true,},  {4, 4}, {128, 0, 0, 0}},
    {Format::BC7,       16, 4, FormatType::Unorm,   {false, false, true,},  {4, 4}, {128, 0, 0, 0}},
    // clang-format on
};

static_assert(std::size(kFormatDesc) == me::enum_count<Format>(), "Format desc table has a wrong size");
} // namespace

namespace bee {
constexpr FormatDesc GetFormatDesc(Format format)
{
    BEE_DEBUG_ASSERT(me::enum_integer(format) < me::enum_count<Format>());

    return kFormatDesc[me::enum_integer(format)];
}

constexpr u32 GetFormatBytesPerBlock(Format format)
{
    BEE_DEBUG_ASSERT(kFormatDesc[me::enum_integer(format)].format == format);

    return kFormatDesc[me::enum_integer(format)].bytesPerBlock;
}

constexpr u32 GetFormatChannelCount(Format format)
{
    BEE_DEBUG_ASSERT(kFormatDesc[me::enum_integer(format)].format == format);
    return kFormatDesc[me::enum_integer(format)].channelCount;
}

constexpr FormatType GetFormatType(Format format)
{
    BEE_DEBUG_ASSERT(kFormatDesc[me::enum_integer(format)].format == format);
    return kFormatDesc[me::enum_integer(format)].Type;
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
    return kFormatDesc[me::enum_integer(format)].compressionRatio.width;
}

constexpr u32 GetFormatHeightCompressionRatio(Format format)
{
    BEE_DEBUG_ASSERT(kFormatDesc[me::enum_integer(format)].format == format);
    return kFormatDesc[me::enum_integer(format)].compressionRatio.height;
}

constexpr bool IsIntegerFormat(Format format)
{
    FormatType type = GetFormatType(format);
    return type == FormatType::Uint || type == FormatType::Int;
}

constexpr u32 GetNumChannelBits(Format format, int channel)
{
    return kFormatDesc[me::enum_integer(format)].numChannelBits[channel];
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
    if (kFormatDesc[me::enum_integer(format)].numChannelBits[0] > 0)
        mask |= TextureChannelFlags::Red;
    if (kFormatDesc[me::enum_integer(format)].numChannelBits[1] > 0)
        mask |= TextureChannelFlags::Green;
    if (kFormatDesc[me::enum_integer(format)].numChannelBits[2] > 0)
        mask |= TextureChannelFlags::Blue;
    if (kFormatDesc[me::enum_integer(format)].numChannelBits[3] > 0)
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
    case Format::D16: return Format::R16;
    case Format::D32F: return Format::R32F;
    default:
        BEE_DEBUG_ASSERT(IsDepthFormat(format) == false);
        return format;
    }
}

constexpr bool DoesFormatHaveAlpha(Format format)
{
    if (GetFormatChannelCount(format) == 4 && !IsCompressedFormat(format)) {
        switch (format) {
        case Format::BGRX8: return false;
        default: return true;
        }
    }

    return false;
}
} // namespace bee