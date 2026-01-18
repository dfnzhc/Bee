/**
 * @File CoordinateSystem.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2026-01-20
 * @Brief This file is part of Bee.
 */

#pragma once

namespace bee
{

enum class ECoordinateSystem
{
    LeftHanded,  ///< 左手坐标系 (DirectX 风格，Z 轴指向屏幕内)
    RightHanded, ///< 右手坐标系 (OpenGL/Vulkan 风格，Z 轴指向屏幕外)
};

enum class ENdcRange
{
    ZeroToOne,  ///< NDC 范围 [0, 1], DirectX/Metal 风格
    NegOneToOne ///< NDC 范围 [-1, 1], OpenGL/Vulkan 风格
};

#ifndef BEE_COORD_SYSTEM
// 默认使用左手系
#define BEE_COORD_SYSTEM_LH 1
#define BEE_COORD_SYSTEM_RH 0
#elif defined(BEE_COORD_SYSTEM_LH) || defined(BEE_COORD_SYSTEM_LeftHanded) || defined(BEE_COORD_SYSTEM_LEFT_HANDED)
#undef BEE_COORD_SYSTEM_LH
#undef BEE_COORD_SYSTEM_RH
#define BEE_COORD_SYSTEM_LH 1
#define BEE_COORD_SYSTEM_RH 0
#elif defined(BEE_COORD_SYSTEM_RH) || defined(BEE_COORD_SYSTEM_RightHanded) || defined(BEE_COORD_SYSTEM_RIGHT_HANDED)
#undef BEE_COORD_SYSTEM_LH
#undef BEE_COORD_SYSTEM_RH
#define BEE_COORD_SYSTEM_LH 0
#define BEE_COORD_SYSTEM_RH 1
#else
#error "BEE_COORD_SYSTEM must be one of: LH, RH, LeftHanded, RightHanded, LEFT_HANDED, RIGHT_HANDED"
#endif

#ifndef BEE_NDC_RANGE
#if BEE_COORD_SYSTEM_LH
// 左手系默认使用 DirectX 风格 [0, 1]
#define BEE_NDC_RANGE_ZO 1
#define BEE_NDC_RANGE_NO 0
#else
// 右手系默认使用 OpenGL 风格 [-1, 1]
#define BEE_NDC_RANGE_ZO 0
#define BEE_NDC_RANGE_NO 1
#endif
#elif defined(BEE_NDC_RANGE_ZO) || defined(BEE_NDC_RANGE_ZERO_TO_ONE) || defined(BEE_NDC_RANGE_ZEROONE)
#undef BEE_NDC_RANGE_ZO
#undef BEE_NDC_RANGE_NO
#define BEE_NDC_RANGE_ZO 1
#define BEE_NDC_RANGE_NO 0
#elif defined(BEE_NDC_RANGE_NO) || defined(BEE_NDC_RANGE_NEG_ONE_TO_ONE) || defined(BEE_NDC_RANGE_NEGONETOONE) || defined(BEE_NDC_RANGE_OGL)
#undef BEE_NDC_RANGE_ZO
#undef BEE_NDC_RANGE_NO
#define BEE_NDC_RANGE_ZO 0
#define BEE_NDC_RANGE_NO 1
#else
#error "BEE_NDC_RANGE must be one of: ZO, NO, ZeroToOne, NegOneToOne, ZeroOne, NegOneToOne, OGL"
#endif

constexpr ECoordinateSystem GetCoordinateSystem() noexcept
{
    #if BEE_COORD_SYSTEM_LH
    return ECoordinateSystem::LeftHanded;
    #else
    return ECoordinateSystem::RightHanded;
    #endif
}

constexpr ENdcRange GetNdcRange() noexcept
{
    #if BEE_NDC_RANGE_ZO
    return ENdcRange::ZeroToOne;
    #else
    return ENdcRange::NegOneToOne;
    #endif
}

} // namespace bee
