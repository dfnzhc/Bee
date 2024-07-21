/**
 * @File Vector2Swizzle.inl
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2024/5/30
 * @Brief 
 */

BEE_FUNC constexpr auto xx() const noexcept { return vec_type(x, x); }
BEE_FUNC constexpr auto xy() const noexcept { return vec_type(x, y); }
BEE_FUNC constexpr auto yx() const noexcept { return vec_type(y, x); }
BEE_FUNC constexpr auto yy() const noexcept { return vec_type(y, y); }