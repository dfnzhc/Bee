/**
 * @File GLM.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2025/10/27
 * @Brief This file is part of Bee.
 */

#pragma once

#include <Core/Base/Defines.hpp>

BEE_PUSH_WARNING
BEE_CLANG_DISABLE_WARNING("-Wdocumentation")
BEE_CLANG_DISABLE_WARNING("-Wunsafe-buffer-usage")
BEE_CLANG_DISABLE_WARNING("-Wfloat-equal")
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/hash.hpp>
BEE_POP_WARNING

namespace Bee
{
    using float2 = glm::vec2;
    using float3 = glm::vec3;
    using float4 = glm::vec4;

    using double2 = glm::dvec2;
    using double3 = glm::dvec3;
    using double4 = glm::dvec4;

    using int2 = glm::ivec2;
    using int3 = glm::ivec3;
    using int4 = glm::ivec4;

    using uint2 = glm::uvec2;
    using uint3 = glm::uvec3;
    using uint4 = glm::uvec4;
} // namespace Bee
