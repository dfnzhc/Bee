/**
* @File Format.hpp
* @Author dfnzhc (https://github.com/dfnzhc)
* @Date 2025/1/7
* @Brief This file is part of Bee.
*/

#pragma once

#include "Core/Defines.hpp"
#include "Utility/Enum.hpp"
#include "Utility/Error.hpp"

namespace bee {
enum class Format : u32
{
    Unknown,

    R8_Unorm,
    R8_Int,
    R8_Uint,
    R8_Snorm,

    R16_Unorm,
    R16_Int,
    R16_Uint,
    R16_Float,
    R16_Snorm,

    R32_Int,
    R32_Uint,
    R32_Float,

    RG8_Unorm,
    RG8_Int,
    RG8_Uint,
    RG8_Snorm,

    RG16_Unorm,
    RG16_Int,
    RG16_Uint,
    RG16_Float,
    RG16_Snorm,

    RG32_Int,
    RG32_Uint,
    RG32_Float,

    RGB8_Unorm,
    RGB8_Int,
    RGB8_Uint,
    RGB8_Snorm,

    RGBA8_Unorm,
    RGBA8_Int,
    RGBA8_Uint,
    RGBA8_Snorm,
    RGBA8_Srgb,
    BGRA8_Unorm,
    BGRA8_Srgb,

    RGBA16_Unorm,
    RGBA16_Int,
    RGBA16_Uint,
    RGBA16_Float,
    RGBA16_Snorm,

    RGB32_Int,
    RGB32_Uint,
    RGB32_Float,

    RGBA32_Int,
    RGBA32_Uint,
    RGBA32_Float,

    //
    R5G6B5_Unorm,
    RGB5A1_Unorm,
    B5G6R5_Unorm,
    BGR5A1_Unorm,

    RGB10A2_Unorm,
    RGB10A2_Uint,
    RG11B10_Float,
    BGRA4_Unorm,
    RGBA4_Unorm,
    BGRX8_Unorm,
    BGRX8_Srgb,
    RGB9E5_Float,

    // Depth-stencil
    D16_Unorm,
    D24_Unorm_S8_Uint,
    D32_Float,
    D32_Float_S8_Uint,

    // Compressed formats
    BC1_Unorm, //!< DXT1
    BC1_Srgb,
    BC2_Unorm, //!< DXT3
    BC2_Srgb,
    BC3_Unorm, //!< DXT5
    BC3_Srgb,
    BC4_Unorm, //!< RGTC Unsigned Red
    BC4_Snorm,
    BC5_Unorm, //!< RGTC Unsigned RG
    BC5_Snorm,
    BC6H_UF16,
    BC6H_SF16,
    BC7_Unorm,
    BC7_Srgb,
};

struct FormatDesc
{
    Format format;

    u8 redBits;         // Red | Depth
    u8 greenBits;       // Green | Stencil
    u8 blueBits;        // Blue
    u8 alphaBits;       // Alpha

    u32 blockSize   :6; // byte size
    u32 widthRatio  :4; // 1 for normal formats, >1 for compressed
    u32 heightRatio :4; // 1 for normal formats, >1 for compressed
    u32 isNorm      :1; // [0, 1] normalized
    u32 isSigned    :1; // signed
    u32 isInteger   :1; // integer
    u32 isFloat     :1; // floating point
    u32 isSrgb      :1; // sRGB
    u32 isBgr       :1; // reversed channels (RGBA => BGRA)
    u32 isDepth     :1; // has depth component
    u32 isStencil   :1; // has stencil component
    u32 isCompressed:1; // block-compressed format
    u32 unused      :9;
};

enum class TextureChannelFlags : u32
{
    None  = 0x0,
    Red   = 0x1,
    Green = 0x2,
    Blue  = 0x4,
    Alpha = 0x8,
    RGB   = Red | Green | Blue,
    RGBA  = RGB | Alpha,
};

BEE_API constexpr FormatDesc GetFormatDesc(Format format);

BEE_API constexpr u32 GetFormatBytesPerBlock(Format format);
BEE_API constexpr u32 GetFormatChannelCount(Format format);

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

BEE_API constexpr bool IsSrgbFormat(Format format);
BEE_API constexpr Format SrgbToLinearFormat(Format format);
BEE_API constexpr Format LinearToSrgbFormat(Format format);

} // namespace bee