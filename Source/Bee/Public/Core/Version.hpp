/**
 * @File Version.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2024/11/15
 * @Brief This file is part of Bee.
 */

#pragma once

#define BEE_VERSION_MAJOR 0
#define BEE_VERSION_MINOR 0
#define BEE_VERSION_PATCH 1

#define BEE_MAKE_VERSION(variant, major, minor, patch) ((((uint32_t)(variant)) << 29U) | (((uint32_t)(major)) << 22U) | (((uint32_t)(minor)) << 12U) | ((uint32_t)(patch)))

#define BEE_VERSION BEE_MAKE_VERSION(0, BEE_VERSION_MAJOR, BEE_VERSION_MINOR, BEE_VERSION_PATCH)

#define BEE_GET_VERSION_VARIANT(version) ((uint32_t)(version) >> 29U)
#define BEE_GET_VERSION_MAJOR(version)   (((uint32_t)(version) >> 22U) & 0x7FU)
#define BEE_GET_VERSION_MINOR(version)   (((uint32_t)(version) >> 12U) & 0x3FFU)
#define BEE_GET_VERSION_PATCH(version)   ((uint32_t)(version) & 0xFFFU)