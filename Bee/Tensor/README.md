# Bee::Tensor

## 概述

`Bee::Tensor` 是 Bee 框架中的通用张量组件，提供 **CPU 主路径 + 可选 CUDA 后端** 的多维数组运算能力，当前仍处于 **MVP 向可用实现演进** 的阶段。

核心设计原则：

- **Result 错误模型**：所有工厂函数与运算均返回 `Result<Tensor>`，通过 `BEE_TRY*` 宏或 `.value()` 解包，杜绝异常传播。
- **值语义**：`Tensor` 对象轻量可拷贝，内部通过 `shared_ptr<TensorImpl>` 共享存储；`clone()` 获得独立副本。
- **视图零拷贝**：`reshape`/`transpose`/`slice` 等操作返回共享同一底层 `Storage` 的视图，不复制数据。

---

## 构建与依赖

### 前置条件

| 工具         | 最低版本 |
|--------------|----------|
| CMake        | 3.24     |
| C++ 编译器   | C++20    |
| Bee::Base    | 同仓库   |

### 构建步骤

```powershell
# Windows PowerShell（在仓库根目录）
mkdir build; cd build
cmake -S .. -B . -DCMAKE_BUILD_TYPE=Debug
cmake --build . --config Debug
```

### CMake 选项

| 选项                        | 默认值   | 说明 |
|-----------------------------|----------|------|
| `BEE_BUILD_TESTS`           | `OFF`    | 打开后构建 `Tests/Tensor` 下的 GTest 测试套件（需要 GTest） |
| `BEE_TENSOR_SIMD`           | `AUTO`   | CPU 运行期 ISA 分发目标集合：`AUTO`（自动探测）/ `SCALAR` / `SSE2` / `AVX2` / `AVX512` |
| `BEE_TENSOR_WITH_CUDA`      | `OFF`    | 打开后接入 `Bee::CUDA` 后端桥接；关闭时保留 CPU 路径，CUDA 调用返回可恢复错误 |

> **注意**：
> 1. CPU 主路径已经接入运行期 ISA 分发，具体可用集合取决于构建期探测与当前机器能力。
> 2. CUDA 后端不是“纯 stub”：启用 `BEE_TENSOR_WITH_CUDA` 且 `Bee::CUDA` 可用时，元素级算子、cast、reduce、matmul、random 与部分 `contiguous()` 路径会直接进入设备端实现。

### 运行测试

```powershell
# 在 build 目录
cmake -S .. -B . -DBEE_BUILD_TESTS=ON
cmake --build . --config Debug --target Tensor.Tests
.\Tests\Tensor\Debug\Tensor.Tests.exe --gtest_brief=1
# 或通过 CTest
ctest -R Tensor -V
```

---

## 快速开始

### 引入头文件

```cpp
#include "Tensor/Tensor.hpp"
using namespace bee;
```

### 创建张量

```cpp
// 未初始化
auto a = Tensor::empty({3, 4}, DType::F32);

// 全零 / 全一 / 指定值
auto zeros = Tensor::zeros({2, 3}, DType::F64);
auto ones  = Tensor::ones ({2, 3}, DType::I32);
auto fives = Tensor::full ({2, 3}, DType::F32, 5.0);

// 等差序列 [0, 12)，默认 I64
auto ar = Tensor::arange(0, 12);

// 带步长与 dtype
auto ar2 = Tensor::arange(0, 10, 2, DType::F32);  // {0,2,4,6,8}

// 解包（Result 模型）
if (!a.has_value()) { /* 处理错误 */ }
Tensor t = *a;
```

### 随机张量

```cpp
// 均匀分布 [0, 1)，seed 固定可复现
auto r  = rand  ({16}, DType::F32, /*seed=*/42);

// 标准正态分布
auto rn = randn ({16}, DType::F32, 42);

// 随机整数 [0, 10)
auto ri = randint(0, 10, {16}, DType::I32, 42);
```

### 视图与形状变换

```cpp
auto flat  = t.reshape({12});           // 非连续时会先物化为 contiguous
auto view  = t.view({2, 6});            // 要求 contiguous
auto tr    = t.transpose(0, 1);         // 交换维度，返回视图
auto perm  = t.permute({1, 0, 2});      // 任意维度排列
auto sq    = t.squeeze(0);              // 移除 size=1 的维度
auto usq   = t.unsqueeze(0);            // 插入 size=1 的维度
auto sl    = t.slice(0, 1, 3);          // 沿 dim=0 取 [1,3)
auto cont  = tr->contiguous();          // 保证内存连续
auto copy  = t.clone();                 // 独立深拷贝
```

### 元素级运算

```cpp
auto c = add(*a, *b);   // a + b
auto d = sub(*a, *b);   // a - b
auto e = mul(*a, *b);   // a * b
auto f = div(*a, *b);   // a / b
auto g = neg(*a);        // -a
auto h = abs(*a);        // |a|
auto i = sqrt(*a);       // √a
auto j = exp(*a);        // e^a
auto k = log(*a);        // ln(a)

// in-place（修改 a 本身，返回 Result<void> 或 Result<Tensor>）
add_inplace(*a, *b);
```

### Reduce 运算

```cpp
// 全局 reduce → 0-rank 标量张量
auto s   = sum (*a);
auto m   = mean(*a);
auto mn  = min (*a);
auto mx  = max (*a);
auto p   = prod(*a);

// 按轴 reduce
auto s1  = sum (*a, /*dim=*/1);
auto s1k = sum (*a, 1, /*keepdim=*/true);  // 保留 size=1 的维度
```

### 矩阵乘法

```cpp
// 仅支持 2D × 2D，dtype 必须相同
auto c = matmul(*a, *b);  // shape: {M,K} × {K,N} → {M,N}
```

### 类型转换

```cpp
auto f32 = cast(*i32_tensor, DType::F32);  // 非连续输入自动连续化
```

### 错误处理示例

```cpp
auto result = matmul(*a, *b);
if (!result.has_value()) {
    // 打印错误信息
    // result.error() 返回 bee::Error
}

// 推荐：用 BEE_TRY_ASSIGN 宏在 Result 函数中传播错误
Result<Tensor> my_func(const Tensor& a, const Tensor& b)
{
    Tensor c;
    Tensor d;
    BEE_TRY_ASSIGN(c, matmul(a, b));
    BEE_TRY_ASSIGN(d, sum(c));
    return d;
}
```

---

## dtype 支持矩阵

| 操作         | Bool | U8 | I32 | I64 | F32 | F64 |
|--------------|:----:|:--:|:---:|:---:|:---:|:---:|
| empty/zeros  | ✓    | ✓  | ✓   | ✓   | ✓   | ✓   |
| arange       | ✗    | ✓  | ✓   | ✓   | ✓   | ✓   |
| add/sub/mul  | ✗    | ✗  | ✓   | ✓   | ✓   | ✓   |
| div          | ✗    | ✗  | ✓   | ✓   | ✓   | ✓   |
| neg/abs      | ✗    | ✗  | ✓   | ✓   | ✓   | ✓   |
| sqrt/exp/log | ✗    | ✗  | ✗   | ✗   | ✓   | ✓   |
| sum/prod     | ✗    | ✗  | ✓   | ✓   | ✓   | ✓   |
| mean         | ✗    | ✗  | →F64| →F64| ✓   | ✓   |
| min/max      | ✗    | ✓  | ✓   | ✓   | ✓   | ✓   |
| matmul       | ✗    | ✗  | ✓   | ✓   | ✓   | ✓   |
| cast         | ✓    | ✓  | ✓   | ✓   | ✓   | ✓   |
| rand/randn   | ✗    | ✗  | ✗   | ✗   | ✓   | ✓   |
| randint      | ✗    | ✓  | ✓   | ✓   | ✗   | ✗   |

> `→F64` 表示整数输入时输出 dtype 提升为 F64。
>
> 额外说明：
> 1. `DType::I8` 已进入当前实现，但主要接通在 **CPU matmul 的 `I8 x I8 -> I32`** 等局部路径，尚未在 README 中整理成完整的统一矩阵。
> 2. `F16/BF16/FP8E4M3/FP8E5M2/FP4` 目前仍作为扩展占位 dtype，不纳入本表。

---

## 已知限制（MVP）

1. **无 autograd**：不追踪计算图，不支持反向传播。
2. **CUDA 语义仍偏保守**：CUDA 路径通常要求输入连续；二元 elementwise 目前不支持广播，`mean(I32/I64)` 也尚未接通。
3. **`contiguous()` 的 CUDA 通用路径仍不完整**：除 2D transpose 特化外，很多非连续 CUDA 物化最终仍会回退到 `D2H -> CPU 重排 -> H2D`。
4. **matmul 仅支持 2D**：不支持 batch matmul 或广播矩阵乘。
5. **无 dtype 自动提升**：二元运算（add/mul/matmul 等）要求两侧 dtype 完全相同，否则返回错误。
6. **无 pinned memory**：CPU 分配均为普通堆内存。
7. **API 语义保持同步**：CPU 路径虽然已经接入 `parallel_for` 与 ISA 分发，CUDA 桥接层也会在返回前同步，因此对上层仍表现为同步调用。
8. **不同算子的 ISA 覆盖度并不完全一致**：例如部分 reduce 能力按 dtype/ISA 细分，CPU matmul 的 AVX512 当前也复用 AVX2 GEMM 实现。

---

## 目录结构

```
Bee/Tensor/
├── Tensor.hpp          # 门面头文件（聚合所有子模块）
├── Tensor.cpp          # 组件入口实现
├── Core/               # 基础元数据（DType、Shape、Storage、TensorImpl、Tensor）
├── Cpu/                # CPU 后端：运行期 ISA 分发、SIMD / GEMM / transpose 等内核
├── Cuda/               # Tensor 到 Bee::CUDA 的桥接层
└── Ops/                # 运算实现（Broadcast、Cast、ElementWise、Matmul、Random、Reduce）
```

对应测试位于 `Tests/Tensor/`，与各模块一一对应，并包含集成测试 `IntegrationTests.cpp`。
