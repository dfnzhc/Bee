/**
 * @File Random.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2024/6/2
 * @Brief 
 */

#pragma once

#include "./Bit.hpp"
#include "./Common.hpp"
#include "./Hash.hpp"

namespace bee {

#define PCG32_DEFAULT_STATE  0x853c49e6748fea9bULL
#define PCG32_DEFAULT_STREAM 0xda3e39cb94b95bdbULL
#define PCG32_MULT           0x5851f42d4c957f2dULL

class PCG32
{
public:
    // clang-format off
    BEE_FUNC constexpr PCG32() = default;
    BEE_FUNC constexpr PCG32(u64 initState, u64 seed) { setSeed(initState, seed); }
    BEE_FUNC constexpr explicit  PCG32(u64 initState) { setSeed(initState); }

    BEE_FUNC_DECL constexpr void setSeed(u64 initState, u64 seed);
    BEE_FUNC      constexpr void setSeed(u64 initState) { setSeed(initState, MixBits(initState)); }
    // clang-format on

    template<typename T>
    BEE_FUNC_DECL constexpr T gen();

    template<std::integral T>
    BEE_FUNC_DECL constexpr T gen(T b);

    BEE_FUNC_DECL constexpr void advance(i64 dt);
    BEE_FUNC_DECL constexpr i64 distance(const PCG32& other) const;

private:
    u64 _state = PCG32_DEFAULT_STATE, _inc = PCG32_DEFAULT_STREAM;
};

template<std::integral T>
BEE_FUNC constexpr T PCG32::gen(T b)
{
    T threshold = (~b + 1u) % b;
    while (true) {
        T r = gen<T>();
        if (r >= threshold)
            return r % b;
    }
}

template<>
BEE_FUNC constexpr u32 PCG32::gen<u32>()
{
    const auto old = _state;
    _state         = old * PCG32_MULT + _inc;

    return RotRight<u32>(cast_to<u32>(((old >> 18u) ^ old) >> 27u), cast_to<i32>(old >> 59u));
    // const auto res = cast_to<u32>(((old >> 18u) ^ old) >> 27u);
    // const auto rot = cast_to<u32>(old >> 59u);
    // return (res >> rot) | (res << ((~rot + 1u) & 31));
}

template<>
BEE_FUNC constexpr u64 PCG32::gen<u64>()
{
    const auto v0 = cast_to<u64>(gen<u32>());
    const auto v1 = cast_to<u64>(gen<u32>());

    return (v0 << 32) | v1;
}

template<>
BEE_FUNC constexpr i32 PCG32::gen<i32>()
{
    const auto v = gen<u32>();
    if (v <= cast_to<u32>(std::numeric_limits<i32>::max()))
        return cast_to<i32>(v);

    BEE_CHECK_GE(v, (u32)std::numeric_limits<i32>::min());
    return cast_to<i32>(v - (u32)std::numeric_limits<i32>::min()) + std::numeric_limits<i32>::min();
}

template<>
BEE_FUNC constexpr i64 PCG32::gen<i64>()
{
    const auto v = gen<u64>();
    if (v <= cast_to<u64>(std::numeric_limits<i64>::max()))
        return cast_to<i64>(v);

    BEE_CHECK_GE(v, (u64)std::numeric_limits<i64>::min());
    return cast_to<i64>(v - (u64)std::numeric_limits<i64>::min()) + std::numeric_limits<i64>::min();
}

template<>
BEE_FUNC constexpr f32 PCG32::gen<f32>()
{
    union
    {
        f32 f;
        u32 u;
    } x{};

    x.u = ((gen<u32>() >> 9) | 0x3F800000UL);
    return x.f - 1.0f;
}

template<>
BEE_FUNC constexpr f64 PCG32::gen<f64>()
{
    union
    {
        f64 d;
        u64 u;
    } x{};

    x.u = ((gen<u64>() >> 12) | 0x3FF0000000000000UL);
    return x.d - 1.0;
}

BEE_FUNC constexpr void PCG32::setSeed(u64 initState, u64 seed)
{
    _state = 0u;
    _inc   = (initState << 1u) | 1u;
    gen<u32>();
    _state += seed;
    gen<u32>();
}

BEE_FUNC constexpr void PCG32::advance(i64 dt)
{
    u64 curMult = PCG32_MULT, curPlus = _inc, accMult = 1u;

    u64 accPlus = 0u, delta = cast_to<u64>(dt);
    while (delta > 0) {
        if (delta & 1) {
            accMult *= curMult;
            accPlus  = accPlus * curMult + curPlus;
        }
        curPlus  = (curMult + 1) * curPlus;
        curMult *= curMult;
        delta   /= 2;
    }
    _state = accMult * _state + accPlus;
}

BEE_FUNC constexpr i64 PCG32::distance(const PCG32& other) const
{
    BEE_CHECK_EQ(_inc, other._inc);
    u64 curMult = PCG32_MULT, curPlus = _inc, curState = other._state;
    u64 theBit = 1u, distance = 0u;
    while (_state != curState) {
        if ((_state & theBit) != (curState & theBit)) {
            curState  = curState * curMult + curPlus;
            distance |= theBit;
        }
        BEE_CHECK_EQ(_state & theBit, curState & theBit);
        theBit  <<= 1;
        curPlus   = (curMult + 1ULL) * curPlus;
        curMult  *= curMult;
    }

    return cast_to<i64>(distance);
}

struct WyRand
{
    BEE_FUNC static constexpr u64 gen1(u64& seed)
    {
        seed += 0x2d358dccaa6c78a5ull;
        return internal::wyMix(seed, seed ^ 0x8bb84b93962eacc9ull);
    }

    BEE_FUNC static constexpr u64 gen2(u64& seed)
    {
        // clang-format off
        constexpr u64 _wyp[4] = {0x2d358dccaa6c78a5ull, 0x8bb84b93962eacc9ull, 
                                 0x4b33a62ed433d4a3ull, 0x4d5a2da51de1aa47ull};
        // clang-format on

        seed = internal::wyMix(seed ^ _wyp[0], seed ^ _wyp[1]);
        seed = internal::wyMix(seed ^ _wyp[0], _wyp[2]);
        return internal::wyMix(seed, seed ^ _wyp[3]);
    }
};

} // namespace bee
