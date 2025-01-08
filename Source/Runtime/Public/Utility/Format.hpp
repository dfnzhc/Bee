/**
 * @File RHI_Format.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2025/1/7
 * @Brief This file is part of Bee.
 */

#pragma once

#include "Core/Defines.hpp"
#include "Utility/Enum.hpp"
#include "Utility/Error.hpp"

namespace bee {
// Texture format
// Notations:
//       RGBA16S
//       ^   ^ ^
//       1   2 3
// 
// 1: Components
// 2: Number of bits per component
// 3: [ ]: Unorm | [S]: Snorm | [I]: Int | [U]: Uint | [F]: Float
// Ranges: Unorm (0, 1); Snorm(-1, 1)....

enum class Format : u32
{
    Unknown,

    R8,
    R8I,
    R8U,
    R8S,

    R16,
    R16I,
    R16U,
    R16F,
    R16S,

    R32I,
    R32U,
    R32F,

    RG8,
    RG8I,
    RG8U,
    RG8S,

    RG16,
    RG16I,
    RG16U,
    RG16F,
    RG16S,

    RG32I,
    RG32U,
    RG32F,

    RGB8,
    RGB8I,
    RGB8U,
    RGB8S,
    RGB9E5F,

    RGBA8,
    RGBA8I,
    RGBA8U,
    RGBA8S,
    BGRA8,

    RGBA16,
    RGBA16I,
    RGBA16U,
    RGBA16F,
    RGBA16S,

    RGB32I,
    RGB32U,
    RGB32F,

    RGBA32I,
    RGBA32U,
    RGBA32F,

    //
    B5G6R5,
    R5G6B5,
    RGB5A1,
    BGR5A1,
    RGB10A2,
    RGB10A2U,
    RG11B10F,
    BGRA4,
    RGBA4,
    BGRX8,

    // Depth-stencil
    D16,
    D16F,
    D24,
    D24F,
    D24S8,
    D32,
    D32F,
    D32FS8U,

    // Compressed formats
    BC1,  //!< DXT1 R5G6B5A1
    BC2,  //!< DXT3 R5G6B5A4
    BC3,  //!< DXT5 R5G6B5A8
    BC4,  //!< LATC1/ATI1 R8
    BC5,  //!< LATC2/ATI2 RG8
    BC6H, //!< BC6H RGB16F
    BC7,  //!< BC7 RGB 4-7 bits per color channel, 0-8 bits alpha
};

enum class FormatType
{
    Unknown, ///< Unknown format Type
    Float,   ///< Floating-point
    Unorm,   ///< Unsigned normalized
    Snorm,   ///< Signed normalized
    Uint,    ///< Unsigned integer
    Int      ///< Signed integer
};

struct FormatDesc
{
    Format format;
    u32 bytesPerBlock;
    u32 channelCount;
    FormatType Type;

    struct
    {
        bool isDepth;
        bool isStencil;
        bool isCompressed;
    };

    struct
    {
        u32 width;
        u32 height;
    } compressionRatio;

    int numChannelBits[4];
};

enum class TextureChannelFlags : u32
{
    None  = 0x0,
    Red   = 0x1,
    Green = 0x2,
    Blue  = 0x4,
    Alpha = 0x8,
    RGB   = 0x7,
    RGBA  = 0xF,
};

BEE_API constexpr FormatDesc GetFormatDesc(Format format);

BEE_API constexpr u32 GetFormatBytesPerBlock(Format format);
BEE_API constexpr u32 GetFormatChannelCount(Format format);
BEE_API constexpr FormatType GetFormatType(Format format);

BEE_API constexpr bool IsDepthFormat(Format format);
BEE_API constexpr bool IsStencilFormat(Format format);
BEE_API constexpr bool IsDepthStencilFormat(Format format);
BEE_API constexpr bool IsCompressedFormat(Format format);

BEE_API constexpr u32 GetFormatPixelsPerBlock(Format format);
BEE_API constexpr u32 GetFormatWidthCompressionRatio(Format format);
BEE_API constexpr u32 GetFormatHeightCompressionRatio(Format format);

BEE_API constexpr bool IsIntegerFormat(Format format);

BEE_API constexpr u32 GetNumChannelBits(Format format, int channel);
BEE_API constexpr u32 GetNumChannelBits(Format format, TextureChannelFlags mask);
BEE_API constexpr TextureChannelFlags GetChannelMask(Format format);

BEE_API constexpr u32 GetFormatRowPitch(Format format, u32 width);

BEE_API constexpr Format DepthToColorFormat(Format format);

BEE_API constexpr bool DoesFormatHaveAlpha(Format format);
} // namespace bee