/**
 * @File Vector3Swizzle.inl
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2024/5/30
 * @Brief 
 */

BEE_FUNC constexpr auto xx() const noexcept { return vec_type(x, x); }
BEE_FUNC constexpr auto xy() const noexcept { return vec_type(x, y); }
BEE_FUNC constexpr auto xz() const noexcept { return vec_type(x, z); }
BEE_FUNC constexpr auto yx() const noexcept { return vec_type(y, x); }
BEE_FUNC constexpr auto yy() const noexcept { return vec_type(y, y); }
BEE_FUNC constexpr auto yz() const noexcept { return vec_type(y, z); }
BEE_FUNC constexpr auto zx() const noexcept { return vec_type(z, x); }
BEE_FUNC constexpr auto zy() const noexcept { return vec_type(z, y); }
BEE_FUNC constexpr auto zz() const noexcept { return vec_type(z, z); }
BEE_FUNC constexpr auto xxx() const noexcept { return vec_type(x, x, x); }
BEE_FUNC constexpr auto xxy() const noexcept { return vec_type(x, x, y); }
BEE_FUNC constexpr auto xxz() const noexcept { return vec_type(x, x, z); }
BEE_FUNC constexpr auto xyx() const noexcept { return vec_type(x, y, x); }
BEE_FUNC constexpr auto xyy() const noexcept { return vec_type(x, y, y); }
BEE_FUNC constexpr auto xyz() const noexcept { return vec_type(x, y, z); }
BEE_FUNC constexpr auto xzx() const noexcept { return vec_type(x, z, x); }
BEE_FUNC constexpr auto xzy() const noexcept { return vec_type(x, z, y); }
BEE_FUNC constexpr auto xzz() const noexcept { return vec_type(x, z, z); }
BEE_FUNC constexpr auto yxx() const noexcept { return vec_type(y, x, x); }
BEE_FUNC constexpr auto yxy() const noexcept { return vec_type(y, x, y); }
BEE_FUNC constexpr auto yxz() const noexcept { return vec_type(y, x, z); }
BEE_FUNC constexpr auto yyx() const noexcept { return vec_type(y, y, x); }
BEE_FUNC constexpr auto yyy() const noexcept { return vec_type(y, y, y); }
BEE_FUNC constexpr auto yyz() const noexcept { return vec_type(y, y, z); }
BEE_FUNC constexpr auto yzx() const noexcept { return vec_type(y, z, x); }
BEE_FUNC constexpr auto yzy() const noexcept { return vec_type(y, z, y); }
BEE_FUNC constexpr auto yzz() const noexcept { return vec_type(y, z, z); }
BEE_FUNC constexpr auto zxx() const noexcept { return vec_type(z, x, x); }
BEE_FUNC constexpr auto zxy() const noexcept { return vec_type(z, x, y); }
BEE_FUNC constexpr auto zxz() const noexcept { return vec_type(z, x, z); }
BEE_FUNC constexpr auto zyx() const noexcept { return vec_type(z, y, x); }
BEE_FUNC constexpr auto zyy() const noexcept { return vec_type(z, y, y); }
BEE_FUNC constexpr auto zyz() const noexcept { return vec_type(z, y, z); }
BEE_FUNC constexpr auto zzx() const noexcept { return vec_type(z, z, x); }
BEE_FUNC constexpr auto zzy() const noexcept { return vec_type(z, z, y); }
BEE_FUNC constexpr auto zzz() const noexcept { return vec_type(z, z, z); }