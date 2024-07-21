/**
 * @File VectorTransform.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2024/6/2
 * @Brief 
 */

#pragma once

#include "./VectorCommon.hpp"
//#include "../matrix.hpp"

namespace bee {

template<typename T>
BEE_FUNC vec3_t<T> Slerp(const vec3_t<T>& x, const vec3_t<T>& y, const T& a)
{
    // get cosine of angle between vectors (-1 -> 1)
    T CosAlpha = Dot(x, y);
    // get angle (0 -> Pi)
    T Alpha = aCos(CosAlpha);
    // get sine of angle between vectors (0 -> 1)
    T SinAlpha = Sin(Alpha);
    // this breaks down when SinAlpha = 0, i.e. Alpha = 0 or Pi
    T t1 = Sin((One<T>() - a) * Alpha) / SinAlpha;
    T t2 = Sin(a * Alpha) / SinAlpha;

    // interpolate src vectors
    return x * t1 + y * t2;
}

template<typename T>
BEE_FUNC vec2_t<T> Rotate(const vec2_t<T>& v, const T& angle)
{
    vec2_t<T> res;
    const T cos_val = Cos(angle);
    const T sin_val = Sin(angle);

    res.x = v.x * cos_val - v.y * sin_val;
    res.y = v.x * sin_val + v.y * cos_val;
    return res;
}

template<typename T>
BEE_FUNC vec3_t<T> Rotate(const vec3_t<T>& v, const T& angle, const vec3_t<T>& normal)
{
    return mat3x3_t<T>(Rotate(angle, normal)) * v;
}

template<typename T>
BEE_FUNC vec4_t<T> Rotate(const vec4_t<T>& v, const T& angle, const vec3_t<T>& normal)
{
    return Rotate(angle, normal) * v;
}

template<typename T>
BEE_FUNC vec3_t<T> RotateX(const vec3_t<T>& v, const T& angle)
{
    vec3_t<T> res(v);
    const T cos_val = Cos(angle);
    const T sin_val = Sin(angle);

    res.y = v.y * cos_val - v.z * sin_val;
    res.z = v.y * sin_val + v.z * cos_val;
    return res;
}

template<typename T>
BEE_FUNC vec3_t<T> RotateY(const vec3_t<T>& v, const T& angle)
{
    vec3_t<T> res   = v;
    const T cos_val = Cos(angle);
    const T sin_val = Sin(angle);

    res.x = v.x * cos_val + v.z * sin_val;
    res.z = -v.x * sin_val + v.z * cos_val;
    return res;
}

template<typename T>
BEE_FUNC vec3_t<T> RotateZ(const vec3_t<T>& v, const T& angle)
{
    vec3_t<T> res   = v;
    const T cos_val = Cos(angle);
    const T sin_val = Sin(angle);

    res.x = v.x * cos_val - v.y * sin_val;
    res.y = v.x * sin_val + v.y * cos_val;
    return res;
}

template<typename T>
BEE_FUNC vec4_t<T> RotateX(const vec4_t<T>& v, const T& angle)
{
    vec4_t<T> res   = v;
    const T cos_val = Cos(angle);
    const T sin_val = Sin(angle);

    res.y = v.y * cos_val - v.z * sin_val;
    res.z = v.y * sin_val + v.z * cos_val;
    return res;
}

template<typename T>
BEE_FUNC vec4_t<T> RotateY(const vec4_t<T>& v, const T& angle)
{
    vec4_t<T> res   = v;
    const T cos_val = Cos(angle);
    const T sin_val = Sin(angle);

    res.x = v.x * cos_val + v.z * sin_val;
    res.z = -v.x * sin_val + v.z * cos_val;
    return res;
}

template<typename T>
BEE_FUNC vec4_t<T> RotateZ(const vec4_t<T>& v, const T& angle)
{
    vec4_t<T> res   = v;
    const T cos_val = Cos(angle);
    const T sin_val = Sin(angle);

    res.x = v.x * cos_val - v.y * sin_val;
    res.y = v.x * sin_val + v.y * cos_val;
    return res;
}

//template<typename T>
//BEE_FUNC mat4x4_t<T> Orientation(const vec3_t<T>& Normal, const vec3_t<T>& Up)
//{
//    if (all(Equal(Normal, Up, Epsilon<T>())))
//        return mat4x4_t<T>(One<T>());
//
//    vec3_t<T> rot_axis = Cross(Up, Normal);
//    T Angle            = aCos(Dot(Normal, Up));
//
//    return Rotate(Angle, rot_axis);
//}

} // namespace bee