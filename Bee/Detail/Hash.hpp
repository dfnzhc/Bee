/**
 * @File hash.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2024/5/28
 * @Brief 
 */

#pragma once

#include "./Common.hpp"

namespace bee {

BEE_INLINE constexpr void HashCombine(size_t& seed, size_t hash)
{
    hash += 0x9e3779b9 + (seed << 6) + (seed >> 2);
    seed ^= hash;
}

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wimplicit-fallthrough"
// https://github.com/explosion/murmurhash/blob/master/murmurhash/MurmurHash2.cpp
BEE_FUNC u64 MurmurHash64A(const unsigned char* key, size_t len, u64 seed)
{
    const u64 m = 0xc6a4a7935bd1e995ull;
    const int r = 47;

    u64 h = seed ^ (len * m);

    const unsigned char* end = key + 8 * (len / 8);

    while (key != end) {
        u64 k;
        Memcpy(&k, key, sizeof(u64));
        key += 8;

        k *= m;
        k ^= k >> r;
        k *= m;

        h ^= k;
        h *= m;
    }

    switch (len & 7) {
    case 7 : h ^= u64(key[6]) << 48;
    case 6 : h ^= u64(key[5]) << 40;
    case 5 : h ^= u64(key[4]) << 32;
    case 4 : h ^= u64(key[3]) << 24;
    case 3 : h ^= u64(key[2]) << 16;
    case 2 : h ^= u64(key[1]) << 8;
    case 1 : h ^= u64(key[0]); h *= m;
    }

    h ^= h >> r;
    h *= m;
    h ^= h >> r;

    return h;
}
#pragma clang diagnostic pop

// http://zimbry.blogspot.ch/2011/09/better-bit-mixing-improving-on.html
BEE_FUNC u64 MixBits(u64 v)
{
    v ^= (v >> 31);
    v *= 0x7fb5d329728ea185;
    v ^= (v >> 27);
    v *= 0x81dadef4bc2dd44d;
    v ^= (v >> 33);
    return v;
}

// -------------------------
// wyhash
// https://github.com/wangyi-fudan/wyhash
// -------------------------
namespace internal {

BEE_FUNC constexpr u64 wyRot(u64 x)
{
    return (x >> 32) | (x << 32);
}

BEE_FUNC constexpr void wyMum(u64* A, u64* B)
{
    u64 ha = *A >> 32, hb = *B >> 32, la = (uint32_t)*A, lb = (uint32_t)*B, hi, lo;
    u64 rh = ha * hb, rm0 = ha * lb, rm1 = hb * la, rl = la * lb, t = rl + (rm0 << 32), c = t < rl;
    lo  = t + (rm1 << 32);
    c  += lo < t;
    hi  = rh + (rm0 >> 32) + (rm1 >> 32) + c;
    *A ^= lo;
    *B ^= hi;
}

BEE_FUNC constexpr u64 wyMix(u64 A, u64 B)
{
    wyMum(&A, &B);
    return A ^ B;
}

BEE_FUNC constexpr u64 wyR8(const u8* p)
{
    u64 v;
    Memcpy(&v, p, 8);
    return (((v >> 56) & 0xff) | ((v >> 40) & 0xff00) | ((v >> 24) & 0xff0000) | ((v >> 8) & 0xff000000) |
            ((v << 8) & 0xff00000000) | ((v << 24) & 0xff0000000000) | ((v << 40) & 0xff000000000000) |
            ((v << 56) & 0xff00000000000000));
}

BEE_FUNC constexpr u64 wyR4(const u8* p)
{
    uint32_t v;
    Memcpy(&v, p, 4);
    return (((v >> 24) & 0xff) | ((v >> 8) & 0xff00) | ((v << 8) & 0xff0000) | ((v << 24) & 0xff000000));
}

BEE_FUNC constexpr u64 wyR3(const u8* p, size_t k)
{
    return (((u64)p[0]) << 16) | (((u64)p[k >> 1]) << 8) | p[k - 1];
}

} // namespace internal

BEE_FUNC constexpr u64 wyHash(const void* key, size_t len, u64 seed, const u64* secret)
{
    const u8* p  = (const u8*)key;
    seed        ^= internal::wyMix(seed ^ secret[0], secret[1]);
    u64 a, b;
    if (len <= 16) {
        if (len >= 4) {
            a = (internal::wyR4(p) << 32) | internal::wyR4(p + ((len >> 3) << 2));
            b = (internal::wyR4(p + len - 4) << 32) | internal::wyR4(p + len - 4 - ((len >> 3) << 2));
        }
        else if (len > 0) {
            a = internal::wyR3(p, len);
            b = 0;
        }
        else
            a = b = 0;
    }
    else {
        size_t i = len;
        if (i > 48) {
            u64 see1 = seed, see2 = seed;
            do {
                seed  = internal::wyMix(internal::wyR8(p) ^ secret[1], internal::wyR8(p + 8) ^ seed);
                see1  = internal::wyMix(internal::wyR8(p + 16) ^ secret[2], internal::wyR8(p + 24) ^ see1);
                see2  = internal::wyMix(internal::wyR8(p + 32) ^ secret[3], internal::wyR8(p + 40) ^ see2);
                p    += 48;
                i    -= 48;
            } while (i > 48);
            seed ^= see1 ^ see2;
        }
        while (i > 16) {
            seed  = internal::wyMix(internal::wyR8(p) ^ secret[1], internal::wyR8(p + 8) ^ seed);
            i    -= 16;
            p    += 16;
        }

        a = internal::wyR8(p + i - 16);
        b = internal::wyR8(p + i - 8);
    }
    a ^= secret[1];
    b ^= seed;
    internal::wyMum(&a, &b);
    return internal::wyMix(a ^ secret[0] ^ len, b ^ secret[1]);
}

BEE_FUNC constexpr uint64_t wyHash64(uint64_t A, uint64_t B)
{
    A ^= 0x2d358dccaa6c78a5ull;
    B ^= 0x8bb84b93962eacc9ull;
    internal::wyMum(&A, &B);

    return internal::wyMix(A ^ 0x2d358dccaa6c78a5ull, B ^ 0x8bb84b93962eacc9ull);
}

} // namespace bee