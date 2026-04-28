#pragma once

#include <cstddef>
#include <cstdint>

namespace bee::simd
{

// 编译期 ISA 标签类型。
//
// SimdBackend<T, ISA> 通过这些空类型选择具体后端实现；运行时分派层先用
// detect_isa() 得到 Isa 枚举，再映射到对应标签。标签本身不携带状态，
// 因而可以作为模板参数安全使用。
struct IsaScalar
{
};
struct IsaSse2
{
};
struct IsaAvx2
{
};
struct IsaAvx512
{
};

// SIMD 后端主模板。
//
// 主模板只声明不定义，所有可用组合都必须在 Backends/*.hpp 中显式特化。
// 这样可以让不受支持的 (T, ISA) 或未提供的操作在编译期暴露，而不是在
// 运行期静默退化到低性能路径。
template <typename T, typename ISA>
struct SimdBackend;

} // namespace bee::simd
