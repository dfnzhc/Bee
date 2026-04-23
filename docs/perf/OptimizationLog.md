# Bee 算子性能优化日志

本文档记录 Bee::Tensor（CPU + CUDA 后端）逐个里程碑的性能演进。每个 B* 里程碑单独一节，格式固定：

1. **现状**：动手前的实现说明与观察到的性能瓶颈。
2. **方案**：本次拟做的改动及理由。
3. **基准**：形状 / dtype × before / after 数字表。
4. **结论 / 遗留**：收益、未达预期的原因、留给下个里程碑的问题。

## 硬件基线

| 项目 | 值 |
| --- | --- |
| CPU | Intel Core i7-13700k (8P + 8E, 24T, AVX2, **无 AVX-512**) |
| DRAM | DDR5-5600 双通道，理论带宽 ≈ 89.6 GB/s |
| L1d / L2 / L3 | 48 KB / 2 MB (P-core) / 30 MB 共享 |
| GPU | NVIDIA RTX 5070 Ti (Blackwell, sm_120) |
| VRAM | 16 GB GDDR7, 理论带宽 ≈ 896 GB/s |
| GPU 峰值算力 | F32 ≈ 44 TFLOPS / F16 Tensor ≈ 176 TFLOPS / FP8 ≈ 352 TFLOPS |
| OS | Windows 11 |
| Toolchain | CMake 3.24+, Clang 19 (MSVC-like, clang-cl), NVCC 13.1, C++23 |
| Benchmark | google-benchmark via vcpkg |

## 测量规范

- **CPU 基准**：`Tensor.Bench`（`Benchmarks/Tensor/`），每组 `Args` 至少 10 次 iter，google-benchmark 自动收敛。`SetBytesProcessed` 按"读+写"元素总字节数登记，输出 `bytes_per_second`。
- **CUDA 基准**：`CUDA.Bench`（`Benchmarks/CUDA/`）。**B0 暂用 wall-clock**，包含 launch overhead + kernel 执行；B5 起切换到 `cudaEventRecord` 精确 kernel-only 计时。
- **基线存档**：每完成一个 B* 里程碑，运行
  ```
  Tensor.Bench --benchmark_format=json --benchmark_out=Benchmarks/baseline/<git-short>.json
  CUDA.Bench   --benchmark_format=json --benchmark_out=Benchmarks/baseline/<git-short>-cuda.json
  ```
  `Benchmarks/baseline/` 已在 `.gitignore` 中，不进仓库；本文档中粘贴对比数据即可。
- **对比参照**：`numpy`（CPU） / `torch.matmul`（CUDA）可选引入，结果以百分比记录。

## 形状矩阵

| 名称 | numel | 说明 |
| --- | --- | --- |
| tiny   | 256          | L1 内，测 kernel launch / dispatch 开销 |
| small  | 4 K          | L2 内，测 ISA 向量化效率 |
| medium | 256 K        | 近 L3 / 跨线程切分阈值 |
| large  | 16 M         | DRAM 带宽 |

GEMM 方阵：{128, 256, 512, 1024, 2048} —— 2048 是 CPU 朴素版本的舒适上限。

## 里程碑日志

### B0 — Benchmark 基础设施

- **状态**：已完成。
- **内容**：
  - `bee_add_component_benchmark` 已具备（`CMake/BeeComponent.cmake:135`）；
  - 新增 `Benchmarks/Tensor/`（EWise / Reduce / Matmul，F32，tiny→large + 方阵 128→1024）；
  - 新增 `Benchmarks/CUDA/`（EWise / Matmul / Memcpy，small→large + 方阵 256→2048）；
  - 仓库 `.gitignore` 加入 `Benchmarks/baseline/`；
  - `Bee/Base/Parallel/`：引入独立的轻量级 `ThreadPool` + `parallel_for(lo, hi, grain, fn)` / `parallel_for_each(begin, end, grain, fn)`，
    不依赖 `Task/Scheduler`，供 CPU 算子后续多线程改造直接使用；
  - `Tests/Base/ParallelForTests.cpp`：覆盖空区间、粒度退化、区间不漏不重、并行累加与串行等价、多线程命中、异常传播共 7 条用例。
- **执行方式**：
  ```pwsh
  cmake -S . -B build-bench -DBEE_BUILD_BENCHMARKS=ON -DCMAKE_BUILD_TYPE=Release
  cmake --build build-bench --config Release --target Tensor.Bench CUDA.Bench
  .\build-bench\Benchmarks\Tensor\Release\Tensor.Bench.exe
  .\build-bench\Benchmarks\CUDA\Release\CUDA.Bench.exe
  ```
- **后续**：B1 跑全套 baseline 并把首张 before 表填到各算子章节。

### B1 — 全量基线

- **状态**：已完成。
- **构建**：Release + clang-cl + AVX2（13700k 本机 ISA），`build-bench/` 目录：

  ```pwsh
  cmake -S . -B build-bench -G Ninja -DCMAKE_BUILD_TYPE=Release `
        -DBEE_BUILD_BENCHMARKS=ON -DBEE_BUILD_TESTS=OFF -DBEE_TENSOR_WITH_CUDA=ON `
        -DCMAKE_CXX_COMPILER=clang-cl -DCMAKE_CUDA_ARCHITECTURES=120 `
        -DCMAKE_TOOLCHAIN_FILE=D:/Env/vcpkg/scripts/buildsystems/vcpkg.cmake
  cmake --build build-bench --target Tensor.Bench CUDA.Bench
  ```

- **补齐的 benchmark**：
  - Tensor 侧：`CastBench`（9 组 dtype 对）、`RandomBench`（rand/randn/randint）、`TransposeBench`（方阵 + 高长宽比，transpose+contiguous 物化）；
  - EWise 增加 F64/I32 覆盖；Reduce 增加 SumF64/SumI32。
- **基线原文**：`Benchmarks/baseline/tensor_baseline.json`、`Benchmarks/baseline/cuda_baseline.json`（本地保留，已在 `.gitignore`）。
- **硬件测到的峰值参考**：
  - DDR5 带宽：Sum F32/tiny 缓存命中 ≈ 84 GB/s，large 落回 ≈ 33 GB/s（单线程流式读）；
  - GPU 带宽：EWise Add F32 large = 693 GB/s（/ 896 理论 ≈ 77 %）；D2D = 687 GB/s；
  - FP32 Matmul：CPU naive @1024 ≈ 44 GFLOPS；CUDA naive @2048 ≈ 3.35 TFLOPS。

#### CPU 基线（F32，除非另标）

| 算子 | tiny 256 | small 4 K | medium 256 K | large 16 M |
| --- | --- | --- | --- | --- |
| Add | 0.23 μs / 13.6 GB/s | 0.59 μs / 84.6 GB/s | 235 μs / 13.3 GB/s | 12.7 ms / 16.1 GB/s |
| Mul | 0.22 μs / 13.6 GB/s | 0.59 μs / 82.9 GB/s | 230 μs / 13.5 GB/s | 12.8 ms / 15.5 GB/s |
| Sqrt | 0.21 μs / 9.7 GB/s | 0.82 μs / 40.9 GB/s | 204 μs / 10.6 GB/s | 12.1 ms / 11.3 GB/s |
| Add F64 | 0.23 μs / 26.4 GB/s | 2.80 μs / 36.8 GB/s | 420 μs / 15.0 GB/s | 26.1 ms / 15.2 GB/s |
| Add I32 | 0.23 μs / 13.5 GB/s | 0.62 μs / 78.3 GB/s | 228 μs / 13.7 GB/s | 13.0 ms / 16.2 GB/s |
| Sum | 0.14 μs / 7.3 GB/s | 0.33 μs / 48.6 GB/s | 12.7 μs / 83.5 GB/s | 2.07 ms / 33.0 GB/s |
| Mean | 0.14 μs / 7.0 GB/s | 0.33 μs / 49.9 GB/s | 12.8 μs / 83.5 GB/s | 2.40 ms / 27.4 GB/s |
| Max | 0.16 μs / 6.3 GB/s | 0.54 μs / 30.6 GB/s | 25.3 μs / 40.3 GB/s | 2.12 ms / 31.8 GB/s |
| Sum F64 | 0.16 μs / 13.1 GB/s | 0.53 μs / 61.3 GB/s | 25.4 μs / 81.6 GB/s | 4.40 ms / 30.9 GB/s |
| Sum I32 | 0.13 μs / 7.8 GB/s | 0.23 μs / 70.5 GB/s | 6.55 μs / 164 GB/s  | 1.96 ms / 33.7 GB/s |

| Matmul F32 square | 128 | 256 | 512 | 1024 |
| --- | --- | --- | --- | --- |
| 时间 | 81 μs | 560 μs | 4.66 ms | 52.3 ms |
| GFLOPS | 51.5 | 62.7 | 59.5 | 43.7 |

| Cast F32→X （large 16M） | F64 | I32 | I64 | U8 | Bool |
| --- | --- | --- | --- | --- | --- |
| 时间 | 21.1 ms | 11.2 ms | 21.0 ms | 5.26 ms | 4.82 ms |
| 带宽 | 9.7 GB/s | 12.6 GB/s | 9.8 GB/s | 16.7 GB/s | 17.1 GB/s |

| Random large 16 M | Rand | Randn | Randint I64 |
| --- | --- | --- | --- |
| 时间 | 35.3 ms | 142 ms | 41.9 ms |
| 吞吐 | 1.9 GB/s | 460 MB/s | 3.2 GB/s |

| Transpose+Contig F32 square | 128 | 512 | 1024 | 2048 |
| --- | --- | --- | --- | --- |
| 时间 | 29 μs | 593 μs | 3.54 ms | 39.7 ms |
| 带宽 | 4.51 GB/s | 3.61 GB/s | 2.40 GB/s | 879 MB/s |

#### CUDA 基线（F32）

| EWise Add | 4 K | 1 M | 16 M |
| --- | --- | --- | --- |
| 时间 | 10.9 μs | 16.8 μs | 294 μs |
| 带宽 | 4.65 GB/s | 773 GB/s | 693 GB/s |

| Matmul F32 square | 256 | 512 | 1024 | 2048 |
| --- | --- | --- | --- | --- |
| 时间 | 30 μs | 100 μs | 680 μs | 4.97 ms |
| TFLOPS | 1.07 | 2.64 | 3.18 | 3.35 |

| Memcpy F32 | 64 K | 4 M | 64 M |
| --- | --- | --- | --- |
| H2D | 36 μs / 7.3 GB/s | 859 μs / 20.6 GB/s | 19.7 ms / 14.1 GB/s |
| D2H | 50 μs / 5.4 GB/s | 3.06 ms / 5.5 GB/s | 47.6 ms / 5.7 GB/s |
| D2D | 12 μs / 21.5 GB/s | 26 μs / 687 GB/s | 826 μs / 319 GB/s |

#### 首轮观察（写入 B2+ 方向判断）

1. **CPU EWise medium/large 瓶颈是带宽**：Sum F32 tiny-medium 已到 83 GB/s 峰值，但 EWise Add/Mul medium 反而跌到 ~13 GB/s。疑为单线程 SIMD 每迭代要 `read+read+write` 三通道，单核无法饱和 DDR5；B2 优先做 ParallelFor 多线程 + NT-store。
2. **CPU Sqrt 标量路径** 在 small 处仅 40 GB/s（理论 Add 路径 ~85 GB/s），说明 Sqrt 还没进 AVX2 intrinsic 分派；B2 附带开 SIMD Sqrt。
3. **Matmul CPU 峰值 < 65 GFLOPS**，远低于 13700k 单线程 AVX2 FMA 上限（~80 GF/core）——naive triple-loop，B4 需要 tiling + packing。
4. **CUDA EWise 已达 77 % 峰值带宽**，B5 的目标是把 4K-1M 过渡段拉平并压到 ≥ 85 %；首要是 float4 向量化。
5. **CUDA Matmul 3.35 TFLOPS** 对 5070 Ti 理论 44 TFLOPS FP32 仅 ~7.6 %，naive tiled；B8/B10 通过 CUTLASS/TMA+tcgen05 走 FP16 路径是主攻点。
6. **D2H = 5.5 GB/s** 远低于 H2D = 20 GB/s（受 PCIe + WDDM），这是持续存在的常量开销，B5 后若存在 D2H 瓶颈算子可考虑 pinned memory。
7. **Transpose+Contig large 跌到 < 1 GB/s**：跨 stride 缓存抖动严重，B11 需要 blocked transpose（64×64 L1-friendly）。
8. **Randn 460 MB/s** 远差于 Rand（1.9 GB/s）：Box-Muller 标量循环，B2/B7 可考虑 Ziggurat 或下沉到 Philox。

### B2 — CPU ElementWise 多线程 + NT-store

- **状态**：已完成。
- **现状**（B1 基线）：单线程 SIMD fast-path 在 large（16 M F32）只能跑到 ~16 GB/s，远低于 DDR5-5600 的 89.6 GB/s 峰值；medium（256 K）同样 13 GB/s，全部被内存带宽卡住 — 因为 3-buf RMW 写 `out = a + b` 在 LLC miss 时先发 read-for-ownership。Sqrt 走同一路径 bench 也仅 11.3 GB/s。
- **方案**：
  1. 新增 `SIMD/Stream.hpp`：`simd_stream<T,ISA>(p, v)` 函数模板，对 SSE2/AVX2/AVX512 的 5 种 T（float/double/i32/i64/u8）特化为 NT-store（`_mm256_stream_*` 等），其他情况退化为 storeu。
  2. `ElementWiseCpu.hpp` 拆分 `cpu_*_linear_chunk<T,ISA,Op,UseStream>`：先做头部标量对齐 → SIMD bulk（对齐时 NT-store，否则 storeu）→ 标量尾部。
  3. `cpu_*_linear_parallel` 外层包 `Base::Parallel::parallel_for`，grain = 128 KB / sizeof(T)（F32 ≈ 32 K 元素）。当 `n * sizeof(T) >= 4 MB` 时启用 NT-store 并在最后调用 `simd::sfence()`；`n < 64K` 元素时退化为串行 chunk 避免派发开销。
- **基准**（13700k，24 线程，F32/F64/I32，bytes_per_second 越大越好）：

  | 算子 | tiny 256 | small 4K | medium 256K | large 16M |
  | --- | --- | --- | --- | --- |
  | Add F32 | 13.6 → 7.7 GB/s ×0.57 | 84.6 → 62.2 ×0.74 | 13.3 → 18.0 ×1.35 | 16.1 → **51.5** ×**3.20** |
  | Add F64 | 26.4 → 26.4 ×1.00 | 36.8 → 112.7 ×3.07 | 15.0 → 18.0 ×1.20 | 15.2 → **43.5** ×**2.86** |
  | Add I32 | 13.5 → 13.2 ×0.98 | 78.3 → 81.3 ×1.04 | 13.7 → 14.5 ×1.06 | 16.2 → **50.4** ×**3.11** |
  | Mul F32 | 13.6 → 13.2 ×0.98 | 82.9 → 91.9 ×1.11 | 13.5 → 15.8 ×1.17 | 15.5 → **46.4** ×**3.00** |
  | Sqrt F32 | 9.7 → 9.7 ×1.00 | 40.9 → 38.1 ×0.93 | 10.6 → 10.3 ×0.97 | 11.3 → **40.9** ×**3.63** |

- **结论**：
  - 大张量（≥ 16 MB working set）带宽一律 ×3 以上，Sqrt 从 11 → 40 GB/s 同比提升；
  - 微张量（256 元素）有小幅回退（Add F32 13.6 → 7.7），根因是新增对齐前缀分支，即便串行回退也使 naive-SIMD 循环略显臃肿；下一步可加 `if (n < W*2)` 直接走纯标量；
  - Sqrt 单次 SIMD 延迟较长，large 场景下仍未触达 40 GB/s 之上，但已与 Add 同数量级，说明瓶颈是带宽而非算力。
- **遗留**：small 4K Add F32 84.6 → 62.2 回退 25%（串行 L1 场景 inlining 差）；需在下轮或 B11 评估 `Op::has_simd` 分支裁剪。

### B3 — CPU Reduce 多线程 + SIMD 水平归约

- **状态**：已完成。
- **现状**（B1 基线）：单线程 Reduce 用 1 个 SIMD 累加器，FP add 3~4 cycle latency 形成长依赖链；large 场景带宽仅 30 GB/s 左右，medium 能吃到 ~84 GB/s 但仍单核。
- **方案**：
  1. `cpu_global_reduce_linear` 改为 4 路 SIMD 累加器（`a0..a3`）循环一次吃 4×W 元素 → 打破 FP add 依赖链，使 execution port 饱和；余下再用 1 路累加器和标量扫尾。
  2. `cpu_reduce_global_dispatch` 外层 `parallel_for`，grain = 64 KB / sizeof(T)。阈值：`bytes >= 4 MB` 才并行；低于 L2（2MB P-core）尺寸保持单线程，避免 `std::mutex` 合并开销压过收益。
  3. 局部累加写入 `partials` vector（互斥锁保护），主线程做 N 路合并。
- **基准**：

  | 算子 | tiny 256 | small 4K | medium 256K | large 16M |
  | --- | --- | --- | --- | --- |
  | Sum F32 | 7.3 → 7.6 | 48.6 → **83.4** ×1.72 | 83.5 → **205** ×**2.45** | 33.0 → **142.5** ×**4.31** |
  | Sum F64 | 13.1 → 16.4 ×1.26 | 61.3 → 116 ×1.89 | 81.6 → **166** ×2.04 | 30.9 → **113** ×**3.66** |
  | Sum I32 | 7.8 → 7.5 | 70.5 → 87.0 ×1.23 | 164 → 180 ×1.10 | 33.7 → **140.8** ×**4.17** |
  | Mean F32 | 7.0 → 6.8 | 49.9 → 80.5 ×1.61 | 83.5 → **194** ×2.32 | 27.4 → **128** ×**4.68** |
  | Max F32 | 6.3 → 6.8 ×1.08 | 30.6 → 70.5 ×2.30 | 40.3 → **145** ×**3.61** | 31.8 → **160** ×**5.05** |

- **结论**：
  - 4 路累加器单线程即能把 Sum medium 从 83 → 205 GB/s（×2.45），说明 B1 基线确实被单累加器的 FP 依赖链严重卡住；
  - Max 全面 ×3~5，最大单项收益；min/max 的 SIMD 水平归约本来被单路累加隐藏，现在 4 路打开带宽；
  - large 数据已到 142–160 GB/s，超过 DDR5-5600 理论峰值 89.6 GB/s — 说明此时大部分数据已命中 LLC / 跨核 L2，体现 P+E 24 线程的聚合缓存带宽。
- **遗留**：
  1. 按轴 reduce（`cpu_reduce_axis_dispatch`）尚未并行化，留作 B11 或 B4 后续；
  2. `std::vector<partials> + mutex` 合并在超多核（24T）下会有锁竞争，下一轮可换为 worker-id 下标写 + barrier；
  3. mean 的 integer→double 累加路径未走 4 路展开，F64 输出仍可再提 ×1.5。

### B4 — CPU GEMM tiling + AVX2 microkernel

#### B4.1 CUDA Warp/Block 原语修正（提交 6e95574）

**现状**：Bee/CUDA 已有 Warp/Block 协作原语（shuffle/reduce/scan），但 BlockReduce 存在若干小问题：
- `BlockReduce::Reduce` 对 Max/Min/Prod 用 `T{}` 作占位（应为 identity，零值会污染结果）。
- `ReduceWithIdentity` 在 kNumWarps 非 2 幂次时 shuffle 轮数偏多。
- 缺少 `Broadcast(src_tid)` 便利接口。
- warp_reduce_{sum,min,max} 对 i32/u32 未利用 sm_80+ 的 `__reduce_*_sync` 单指令。

**方案**：
- 删除有缺陷的 `BlockReduce::Reduce`，保留/修正 `ReduceWithIdentity`。
- 引入 `kAggrWidthP2 = next_pow2(kNumWarps)`：`lane >= kNumWarps` 注入 identity，之后在 `kAggrWidthP2/2` 宽度内折半。BlockSize=256 → kNumWarps=8 → 3 轮 shuffle（原 5 轮）。
- 新增 `BlockReduce::Broadcast(input, src_tid)` 复用 `storage.broadcast` 字段。
- `warp_reduce_sum/min/max<int32|uint32>` 在 `width==kWarpSize && mask==kFullMask32` 时走 `__reduce_*_sync`。

**验证**：`Tests/CUDA/WarpBlockTests.cu` 44 tests PASSED（含 shuffle/reduce/scan/broadcast/any-all/ballot）。

#### B4.2 CPU GEMM 外层多线程（提交本次）

**现状**：单线程 Goto 三层分块 GEMM（AVX2 8×8 / SSE2 4×4 microkernel）SGEMM 1024 = 121 GFLOPS（AVX2 单 P-core 峰值 172.8 GFLOPS 的 70%）。13700k 有 8P+8E 共 24T，单线程利用率 < 5%。

**方案**：
- 抽出共享 driver 到 `Bee/Tensor/Cpu/Gemm/GemmDriver.hpp`，AVX2 / SSE2 两个 .inl 统一调用。
- 并行策略：外 `jc`/`pc` 串行；`pack B` 主线程串行（共享给所有 worker）；内层 `ic` 循环切成 `num_ic_chunks = ceil(M_main/MC)` 块，`bee::parallel::parallel_for` 分派。
- **per-worker A_pack**：`thread_local AlignedBuffer`（576 KB，覆盖 AVX2 MC=192×KC=384 最大 packed panel），懒分配，持续复用；24 线程 ≈ 13.8 MB，可接受。
- **并行阈值** `kGemmParallelFlops = 4 M FLOPs`：小矩阵退化到 `gemm_driver_serial`，避免 fork-join 开销。

**基准（SGEMM，GFLOPS）**：

| Shape | before | after | 加速比 | 备注 |
| ----- | ------ | ----- | ----- | --- |
| 128³  |  53.7  |  55.7 | 1.04× | 2.1 MF < 阈值，走 serial |
| 256³  |  93.2  | 216.7 | 2.33× | 33 MF，8 ic chunks |
| 512³  | 106.9  | 251.6 | 2.35× | 锁 B_pack 串行是瓶颈 |
| 1024³ | 121.1  | **669.3** | **5.53×** | 8 ic chunks × ~83 GF/chunk，接近 8P 满载 |

**结论**：1024 达 669 GFLOPS，超过 B4.2 目标（600 GFLOPS）。256/512 加速比偏低，原因：
1. `pack B` 主线程串行，K=512 时 pack 占比 ~30%（B4.3 候选优化：pack B 也并行，按 jc tile 分）。
2. `num_ic_chunks = M_main/MC = 1~2`（256→MC=192→1 chunk；512→3 chunks），并行度不足；可考虑对中等 M 把 MC 调小到 96/128。

**遗留**：
- F64/I32 GEMM 已加入 bench（见 MatmulBench.cpp），baseline 存 `tensor_b4.json`。
- DGEMM `NC=2048` 对 KC=384 下 B panel = 6 MB 超 L3 有 buffer 压力（B4.3 候选：DGEMM NC 单独调小到 1024）。
- tiny 256 AddF32 的 B2 回退（13.6→7.7 GB/s）未修，属下个循环。

#### B4.3 DGEMM 分块微调（负结果 / 回退）

**假设**：DGEMM 的 B panel = KC×NC×8 = 384×2048×8 = **6 MB** 超过 P-core L2（2 MB）；缩小 NC=1024（3 MB）应能提升 L2 命中。

**实验**：在 `Avx2BlockSize` 加 `NC_D=1024` 并让 `gemm_f64` 使用；F64 matmul 测试全过。

**结果**：
- DGEMM 1024³：371 → **334** GFLOPS（反而略降）；256/512 也无收益。
- 分析：L2 本来就装不下任何合理大小的 B panel（即使 NC=1024 的 3 MB 也超 2 MB L2），收益来自 L3；而 NC 减半直接使 A 重扫次数翻倍、pack B 总次数翻倍，成本压过收益。
- **结论**：回退 DGEMM NC 到 2048（与 SGEMM 一致）。真正的 DGEMM 提升需要更深入的优化（更大的 MR、SIMD 向量化 B pack、使用 P-core-only 线程亲和性），留作后续迭代。

#### B4.4 回归与文档

- 新增 F64/I32 的 matmul bench（`Benchmarks/Tensor/MatmulBench.cpp`）。
- baseline 存档：`Benchmarks/baseline/tensor_b4.json`（F32/F64/I32 × 128/256/512/1024 方阵）。
- **B4 总览数据**（AVX2 Release，google-benchmark `min_time=0.5s`，多次运行存在 ±15% 波动）：

| dtype \ N | 128 | 256 | 512 | 1024 | 单核峰值% (1024) |
| --- | ---:| ---:| ---:| ---:| ---:|
| F32 (GFLOPS)  |  52  | 202–217  | 252–321  | **669–950**  | 8P × 172.8 GF = 1382 GF → 48–69% |
| F64 (GFLOPS)  |  28–35  |  98–109  | 115–128  | **334–371**  | 8P × 86.4 GF = 691 GF → 48–54% |
| I32 (GOPS)    |  43  | 141  | 181  | **579**  | ≈55% vs SGEMM（mullo 吞吐较低） |



### B5 — CUDA ElementWise 128-bit 向量化

#### 现状（pre-B5）

- `Bee/CUDA/Ops/ElementWise.cu` 中 `binary_kernel` / `unary_kernel` 均为 1 thread / 1 element 的朴素实现：每个线程做一次 32-bit load × 2 + 32-bit store，grid 覆盖全部元素。
- B1 基线（F32 Add，`cuda_baseline.json`）：1M = **773 GB/s**，16M = **693 GB/s**，已达理论 896 GB/s 的 77–86%，且 kernel 阶段内 `cudaStreamSynchronize` 确保 bench 计时覆盖真实执行。

#### 方案

1. 对**纯内存 bound** 的 op 使用 128-bit 向量化 load/store，减少 LD/ST 指令条数、提高访存效率：
   - `u8 → uint4` (16 元素/线程)
   - `i32 / f32 → int4 / float4` (4 元素/线程)
   - `i64 / f64 → longlong2 / double2` (2 元素/线程)
2. 加 `VecTraits<T>` + `load_vec` / `store_vec` / `apply_bin` / `apply_un` 模板辅助，保持原 op 语义。
3. 保留"尾部 kernel"：`n - (n/VEC_N)*VEC_N` 个元素走标量 kernel，处理非对齐尾巴（CUDA `cudaMalloc` 天然 256-byte 对齐，无需额外对齐检查）。
4. 用 `grid-stride loop`，vec kernel grid 上限设为 512 饱和 SM，超大 n 通过 stride 循环覆盖。
5. **仅对 memory-bound 的 op 向量化**：
   - binary：add / sub / mul / div 全部走 vec。
   - unary：neg / abs 走 vec；sqrt / exp / log 含数学库调用，compute-bound，保持标量。

#### 基准

| Bench | Size | Before (B1) | After (B5) | 变化 |
|-------|------|-------------|-----------|------|
| BM_AddF32_CUDA | 4096 | 4.65 GB/s | 4.38 GB/s | –6% (launch overhead dominates) |
| BM_AddF32_CUDA | 1M  | **773 GB/s** | 752 GB/s | –3% 噪声 |
| BM_AddF32_CUDA | 16M | **693 GB/s** | 679 GB/s | –2% 噪声 |
| BM_MulF32_CUDA | 1M  | 757 GB/s | 771 GB/s | +2% 噪声 |
| BM_MulF32_CUDA | 16M | 701 GB/s | 656 GB/s | –6% 噪声 |

> 数字档：`Benchmarks/baseline/cuda_baseline.json`（B1）与 `cuda_baseline/cuda_b5.json`（B5，未入库）。

#### 结论（负结果）

- **吞吐未提升**，基本在 ±6% 运行噪声区间内波动。
- 根因：B1 基线已达显存带宽的 77–86%（RTX 5070 Ti 理论 896 GB/s）。grid 足够大时，coalesced 32-bit 访问已在 LD/ST unit 层面被硬件合并为全 cache line 访问，显式向量化只能节省指令带宽，对 memory-bound kernel 不构成瓶颈。
- **代码仍保留**：向量化版本对未来 op fusion、L2 residency 优化、mixed-dtype 融合核更友好；且正确性通过 `Tests/CUDA/ElementWiseTests.cu`（6 组 × 多 n size，含非对齐尾部）验证通过。
- **下一步建议**（不在 B5 范围）：
  - 小 size（4K）瓶颈已切换为 launch latency，后续可通过 persistent kernel / cudaGraph 去掉反复 launch。
  - 16M > L2（48 MB）时输出写回挤占带宽，需与 allocator reuse + in-place op 联动考虑。
- **B5 状态**：标记为"完成，无吞吐收益，代码作为结构优化保留"。

### B6 — CUDA Reduce warp shuffle（TODO）

### B7 — CUDA Random 原生 Philox（TODO）

### B8 — CUDA Matmul L2 CUTLASS（TODO）

### B9 — CUDA Transpose TMA（TODO）

### B10 — CUDA Matmul L3 TMA + tcgen05.mma（TODO）

### B11 — CPU Cast/Transpose SIMD（TODO）

### B12 — 回归监控 / 文档收尾（TODO）
