#pragma once

// 元素级算子自由函数声明：二元（add/sub/mul/div）、一元（neg/abs/sqrt/exp/log）
// 及对应的 in-place 变体（add_inplace 等）

#include "Base/Diagnostics/Error.hpp"
#include "Tensor/Core/Tensor.hpp"

namespace bee
{

[[nodiscard]] auto add(const Tensor& a, const Tensor& b) -> Result<Tensor>;
[[nodiscard]] auto sub(const Tensor& a, const Tensor& b) -> Result<Tensor>;
[[nodiscard]] auto mul(const Tensor& a, const Tensor& b) -> Result<Tensor>;
[[nodiscard]] auto div(const Tensor& a, const Tensor& b) -> Result<Tensor>;

[[nodiscard]] auto neg(const Tensor& a)  -> Result<Tensor>;
[[nodiscard]] auto abs(const Tensor& a)  -> Result<Tensor>;
[[nodiscard]] auto sqrt(const Tensor& a) -> Result<Tensor>;
[[nodiscard]] auto exp(const Tensor& a)  -> Result<Tensor>;
[[nodiscard]] auto log(const Tensor& a)  -> Result<Tensor>;

[[nodiscard]] auto add_inplace(Tensor& dst, const Tensor& src) -> Result<void>;
[[nodiscard]] auto sub_inplace(Tensor& dst, const Tensor& src) -> Result<void>;
[[nodiscard]] auto mul_inplace(Tensor& dst, const Tensor& src) -> Result<void>;
[[nodiscard]] auto div_inplace(Tensor& dst, const Tensor& src) -> Result<void>;

[[nodiscard]] auto neg_inplace(Tensor& dst)  -> Result<void>;
[[nodiscard]] auto abs_inplace(Tensor& dst)  -> Result<void>;
[[nodiscard]] auto sqrt_inplace(Tensor& dst) -> Result<void>;
[[nodiscard]] auto exp_inplace(Tensor& dst)  -> Result<void>;
[[nodiscard]] auto log_inplace(Tensor& dst)  -> Result<void>;

} // namespace bee
