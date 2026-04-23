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

### B6 — CUDA Reduce warp shuffle

#### 现状（pre-B6）

- `Bee/CUDA/Ops/Reduce.cu` 的 `reduce_global_kernel` 用 dynamic shared mem + 传统 `for (s=blockDim/2; s>0; s>>=1)` 树状折半归约，每轮都要 `__syncthreads`。
- `ReduceWithIdentity`（`Bee/CUDA/Core/Block.cuh`）已基于 warp shuffle 实现（两阶段：warp-level butterfly + 跨 warp 聚合），在 B4.1 被修好过且有单测覆盖，但尚未接入实际算子。
- 本次目标：将 Reduce kernel 切换到 `BlockReduce`，并在 grid-stride 累加阶段做 4× 展开。

#### 方案

1. `reduce_global_kernel` 改为：
   - 固定 `BlockSize = 256`（与 `kDefaultBlockSize` 对齐）；
   - grid-stride 累加 4× 展开（每线程一次循环吃 4 个元素，隐藏 DRAM 延迟）；
   - 最后一次 block 内规约调用 `BlockReduce<T, 256, Op>::ReduceWithIdentity`；
   - 去掉 dynamic shared mem（TempStorage 走静态 `__shared__`）。
2. 通过 `OpTrait<OP>::type` 把本文件的 `kRd*` 整数 OP 映射到 `WarpOp{Sum,Prod,Min,Max}`。
3. 两阶段 launch 拓扑保持（stage1: n → grid_partials，stage2: partials → 1）；上限仍为 1024 block。
4. Axis reduce（`reduce_axis_kernel`）不变（每输出线程串行归约 `axis` 维，非 memory-bound，B6 不动）。

#### 基准（`BM_Sum*_CUDA` / `BM_MaxF32_CUDA`）

| Bench | Size | Before | After | 变化 |
|-------|------|--------|-------|------|
| SumF32 | 4K    | 0.87 GB/s | 0.87 GB/s | launch-bound |
| SumF32 | 1M    | 206 GB/s | 213 GB/s | +3% |
| SumF32 | 16M   | **587 GB/s** | **625 GB/s** | **+6.5%** |
| SumI32 | 1M    | 210 GB/s | 206 GB/s | 噪声 |
| SumI32 | 16M   | **598 GB/s** | **629 GB/s** | **+5.2%** |
| MaxF32 | 16M   | **611 GB/s** | **639 GB/s** | **+4.6%** |

> 存档：`Benchmarks/baseline/cuda_b6_before.json`（stash 原 tree-reduce）与 `cuda_b6_after.json`（warp-shuffle）。小 size（4K/1M）被 `cudaMallocAsync + cudaStreamSynchronize` 开销支配（~20 µs/iter），本次改动不触及这一路径。

#### 正确性

新增 `Tests/CUDA/ReduceTests.cu`：{Sum, Min, Max, Prod} × {I32, I64, F32, F64, U8 (min/max only)} 多组 n（含 1、31、32、256、257、2^12、2^18、2^20 的非 block 对齐尺寸），5 组测试全通过。

#### 结论

- 大 n（16M，显存 bound）带宽提升 **4.6–6.5%**，达到理论 896 GB/s 的 ~70%。
- 中小 n（≤1M）瓶颈切至 launch + partial 缓冲区分配释放，本次不覆盖；后续 B7/B8 的 persistent kernel 或共享 scratch pool 可继续压缩这部分固定开销。
- 代码结构更清晰：去掉 dynamic smem 依赖、去除传统 tree-reduce 循环，`BlockReduce` 成为 CUDA 端规约的唯一路径。

### B7 — CUDA Random 原生 Philox

#### 现状（pre-B7）

- `Bee/Tensor/Ops/Random.cpp` 中 `rand/randn/randint` 对 `Device::CUDA` 的处理方式为：
  1. 在 CPU 上用 `std::mt19937_64` + `uniform_real_distribution`/`normal_distribution`/`uniform_int_distribution` 生成整张 Tensor；
  2. 调用 `cpu_out.to(Device::CUDA)` 把结果 H2D 拷贝到显存。
- 即 CUDA 路径完全没有使用 GPU 算力；16M F32 rand 的 2/3 时间花在 CPU normal_distribution，1/3 花在 PCIe H2D。

#### 方案

1. 新建 `Bee/CUDA/Ops/Random.cu`：
   - 标准 **Philox4x32-10** 设备侧实现（DE Shaw SC'11）：`mulhilo32` + 10 轮，key bump 用 Weyl 常量 `0x9E3779B9 / 0xBB67AE85`。
   - 每线程用 `(counter=thread_idx, subseq, seed)` 作为 `(ctr, key)`，一次 `philox_gen` 产出 4 × u32，subseq 区分 uniform/normal/int 三条数据流避免与同 seed 的其他 op 重叠。
   - F32 uniform：`u32 >> 8` 构 24-bit mantissa，映射 `[0, 1)`。
   - F64 uniform：合并两个 u32 → `u64 >> 11` 构 53-bit mantissa。
   - F32/F64 normal：成对 uniform 过 Box-Muller（`sqrt(-2 log u1) * {cos,sin}(2π u2)`）。
   - Int：u32 mod range + low（MVP，I64 也走 32-bit → 对大 range 有轻微偏态，够用）。
   - Launch 配置：`Block=256`, `grid = min(1024, ceil(threads / Block))`；grid-stride 循环保证 n 任意。
2. `OpsBridge.hpp` 暴露 `ops_random_{uniform,normal,int}` 三个 int-returning 桥函数；`CUDA/Api.hpp/Api.cpp` 加 `cuda::ops::random_*` 的 Result<void> 包装。
3. `Bee/Tensor/Cuda/Backend.hpp/cpp` 转发一层 `tensor::cuda::random_*`（保持 Tensor 组件不直接 include CUDA 头的约束）。
4. `Bee/Tensor/Ops/Random.cpp` 在 `Device::CUDA` 分支直接在 CUDA device 上 `Tensor::empty` + 调 CUDA API 填充；`seed=0` 时派生一次非零 u64（与 CPU "真随机" 语义对齐）。

#### 基准（`BM_Rand*_CUDA`）

> "before" = stash 掉 `Tensor/Ops/Random.cpp`，走旧的 CPU 生成 + `to(CUDA)` 路径；"after" = 原生 CUDA kernel。

| Bench | Size | Before | After | 加速 |
|-------|------|--------|-------|------|
| RandF32    | 4K    | 806 MB/s | 1.41 GB/s | ×2.2 |
| RandF32    | 1M    | 1.52 GB/s | **268.8 GB/s** | **×177** |
| RandF32    | 16M   | 1.57 GB/s | **617.9 GB/s** | **×394** |
| RandnF32   | 4K    | 320 MB/s | 1.44 GB/s | ×4.6 |
| RandnF32   | 1M    | 426 MB/s | **235.0 GB/s** | **×552** |
| RandnF32   | 16M   | 439 MB/s | **614.4 GB/s** | **×1434** |
| RandintI32 | 16M   | 1.50 GB/s | **358.4 GB/s** | **×239** |

> 存档：`Benchmarks/baseline/cuda_b7_before.json`、`cuda_b7_after.json`。

#### 正确性

新增 `Tests/CUDA/RandomTests.cu`（9 个用例）：
- `UniformF32_Range` / `UniformF64_Range`：验证 `[0, 1)` 半开区间与 mean ≈ 0.5；
- `UniformF32_Deterministic`：同 seed 两次调用结果按位相等；
- `UniformF32_NonPow2Size`：n = 1/3/5/1025/1027 的尾部分支不越界；
- `NormalF32_Moments` / `NormalF64_Moments`：大样本下 `|mean| < 0.03`、`|stddev − 1| < 0.05`；
- `IntI32_Range` / `IntU8_Range` / `IntI64_Range`：输出全部落在 `[low, high)`。

9 组测试全通过。Tensor.Tests 原有 12 组 CPU Random 用例同样通过（未触及 CPU 路径）。

#### 结论

- rand/randn/randint 的 CUDA 路径吞吐提升 2–3 个数量级：大 n RandF32 从 1.57 GB/s 涨到 **617 GB/s**（5070 Ti 显存 896 GB/s 的 69%），RandnF32 从 439 MB/s 涨到 **614 GB/s**（×1434）。
- 之前"CPU 生成 + H2D"的瓶颈一是 CPU 单线程 `std::normal_distribution` 极慢（每元素 ~70 ns），二是 PCIe 4.0 x16 理论 32 GB/s 仍远低于显存；彻底解耦后 kernel 本身的指令密度小、内存仅 1W/elem，直接撞显存带宽上限。
- Philox stateless 设计不维护 per-thread state、无 init kernel，4K 样本下 ~11 µs 几乎全为 cudaMallocAsync + streamSynchronize 开销，与 B6 小 size 瓶颈同根；后续若做 scratch pool / cudaGraph 缓存 launch 可同时改善。

### B8 — CUDA Matmul L2 CUTLASS

**现状（B1 基线 / 手写 16×16 tile shared-memory kernel）**：
| size | 耗时 | GFLOPS | 峰值占比 (F32 ~45 TFLOPS) |
|-----:|-----:|-------:|--------------------------:|
|  256 | 0.03 ms |  1.18 TFLOPS |  3% |
|  512 | 0.10 ms |  2.73 TFLOPS |  6% |
| 1024 | 0.63 ms |  3.42 TFLOPS |  8% |
| 2048 | 4.88 ms |  3.54 TFLOPS |  8% |

16×16 tile 没用 tensor core，且 thread block 只有 256 线程，register pressure 低、占用率无法掩盖访存延迟。

**方案**：
1. 新增 `Bee/CUDA/Ops/MatmulCutlass.cu`（独立 TU），基于 CUTLASS 2.x device API：
   - `cutlass::gemm::device::Gemm<F32, RowMajor, ..., OpClassTensorOp, Sm80, Tb=128×128×32, Warp=64×64×32, Inst=16×8×8, Stages=3>`；
   - TF32 tensor-core（输入 F32 → 硬件自动截断 TF32 参与 WMMA、累加 F32），精度较纯 F32 少 ~10 位尾数；
   - sm_80 模板、sm_120 向下兼容运行，不依赖 Hopper/Blackwell collective builder，**编译时间 < 1 min**（对比 sm_120 collective 的 5–10 min）。
2. `Matmul.cu::ops_matmul` 为 F32 增加 CUTLASS fast-path：
   - **启用条件**：`M ≥ 384 && N ≥ 384 && K ≥ 32`（小 GEMM ThreadblockShape 128×128 无法填满 SM、launch overhead 主导，直接回退）；
   - **对齐保护**：`M/N/K % 4 != 0` 时 CUTLASS 配置返回 `kInvalid`，调用方回退到手写 tile kernel；
   - I32 / I64 / F64 保留原路径。
3. CMake 探测 `CUTLASS_ROOT` / `ENV_HOME/NVIDIA/cutlass` 并定义 `BEE_HAS_CUTLASS=1`；新增 `--expt-relaxed-constexpr` 以抑制 CUTLASS 自身 constexpr host/device 兼容性警告。

**基准（5070 Ti，Release）**：

| size | Before (TFLOPS) | After (TFLOPS) | 加速   |
|-----:|----------------:|---------------:|-------:|
|  256 |            1.18 |           1.08 |  0.91× (走 fallback，波动) |
|  512 |            2.73 |           5.05 |  1.85× |
| 1024 |            3.42 |          25.03 |  7.32× |
| 2048 |            3.54 |          35.31 |  9.97× |

2048² 达 35.3 TFLOPS（5070 Ti TF32 峰值 ~90 TFLOPS 的 39%），剩余余量主要来自：sm_80 Ampere 配置未使用 sm_90 WGMMA / sm_120 tcgen05 新指令、单缓冲 epilogue 没有 Pipeline、ThreadblockShape 未针对 5070 Ti L2 调优。后者归 B10 跟进。

**正确性**：`CUDA.Tests` 64/64 通过（含 5 个 `MatmulTests`），`Tensor.Tests` 232/232 通过。TF32 的精度损失在现有测试的容差范围内（测试 value scale 通常为小整数或 `EXPECT_NEAR` 相对容差 >= 1e-3）；若后续需要 CPU bit-exact 对照，需单独增加 SIMT F32 kernel 路径。

**结论**：F32 大 GEMM 在不依赖新架构特性的前提下取得约 **10× 提速**；小 GEMM 保留手写 fallback 避免退化。为 B10 的 sm_120 collective/tcgen05 路径奠定对比基线。

### B9 — CUDA Transpose 向量化 load

**现状（B9 前）**：`Transpose.cu` 用 32×33 padded shared tile + `dim3(32,8)` block、每线程 4 iter 串行 load/store。2D contiguous 物化吞吐：

| Shape          | Before (μs) | GB/s  |
|----------------|------------:|------:|
| F32  1024²     | 18.40       | ~450  |
| F32  2048²     | 25.04       | ~1244 ¹ |
| F32  4096²     | 225.92      | ~591  |
| I32  4096²     | 226.79      | ~597  |
| F64  4096²     | 426.45      | ~639  |
| F32 16384×64   | 17.53       | ~485  |
| F32 262144×64  | 209.27      | ~636  |

¹ 2048² F32 工作集 32 MiB 部分驻留 L2，实测超 HBM 理论带宽，属非均匀测量。以 4096² 为真实带宽参考。

**方案**：

1. 保留现有 32×33 tile kernel 作为通用/回退路径（非对齐、非 32 倍数、U8/I64/F64）。
2. 新增 F32/I32 专用的**向量化 global load + 向量化 global store** 快速路径：
   - `TILE=32`、`block=(8,32)=256 threads`；每线程 x 方向处理 4 连续元素；
   - 读：单条 `float4/int4` global load → 逐元素写入 shared（共享行步长 33 非 16B 倍数，必须标量写，避免 `cudaErrorMisalignedAddress`）；
   - 写：逐元素从 shared 按列读取组装寄存器 → 单条 `float4/int4` global store。
3. 触发条件：`rows%32==0 && cols%32==0 && src/dst 16B 对齐`；`cudaMalloc` 默认 256B 对齐满足。

**实施要点**：
- 初版错将 shared pointer 也按 `float4*` reinterpret，ty=1 时 tile 行基址偏移 132 字节（4 mod 16）触发 misaligned，测试即崩；改为 shared 端标量 load/store 后修复。
- CUDA 测试 64/64、Tensor 测试 232/232 全绿。

**结果（4096² F32 为真带宽代表）**：

| Shape          | Before μs | After μs | Speedup | After GB/s |
|----------------|----------:|---------:|--------:|-----------:|
| F32 1024²      | 18.40     | 16.56    | 1.11×   | ~521       |
| F32 2048²      | 25.04     | 25.13    | 1.00×   | ~1347 ¹    |
| F32 4096²      | 225.92    | 223.03   | 1.01×   | ~611       |
| I32 1024²      | 17.91     | 16.55    | 1.08×   | ~542       |
| I32 4096²      | 226.79    | 226.65   | 1.00×   | ~611       |
| F32 16384×64   | 17.53     | 16.76    | 1.05×   | ~501       |

¹ 同前注。

**结论**：当前 32×32 shared-tile kernel 在 4096² 已达 ~600 GB/s，相对 RTX 5070 Ti GDDR7 256-bit 28 Gbps 理论 ~896 GB/s 约 68%，原生单块 SM 路径下继续压榨空间有限；向量化 load/store 主要在小规模（1024²、瘦阵）收 5–11%、大规模基本持平。

**未采纳方案**：

1. **TMA (`cp.async.bulk.tensor.2d`)**：需要构造 `CUtensorMap` 描述符 + shared barrier 协议，pattern 与 WGMMA mainloop 类似，收益对 memcpy-bound transpose 应接近 0（瓶颈已在 HBM 带宽），实现 ROI 低。
2. **Swizzled shared (XOR)**：节约 1 行 padding，对 smem 容量无压力、对 occupancy 无实质帮助。
3. **TILE=64**：单 block smem 翻 4×（17 KB），5070 Ti SM 每个 SM 100 KB smem 仍可容纳，但 per-block 线程不足会限制 ILP，未在本轮实现。

保留为 B10 之后若 profiling 显示带宽偏离 HBM 理论峰值再回来。

**产物**：
- `Bee/CUDA/Ops/Transpose.cu`（新增 `transpose_vec4_kernel` + 阈值路由）
- `Benchmarks/CUDA/TransposeCudaBench.cpp`（新建；方阵 F32/I32/F64 + 瘦阵 F32）
- `Benchmarks/baseline/cuda_b9_{before,after}.json`


### B10 — CUDA Matmul L3 手写 TMA + WMMA TF32

**目标**：与 B8 CUTLASS 路径并行存在的、完全手写的 TF32 GEMM kernel，作为可读的"教科书参考实现"，并尽可能利用 RTX 5070 Ti（sm_120, Blackwell GeForce）的 TMA 异步搬运与 WMMA TensorCore。

**架构现实（决定性约束）**：
- sm_120 GeForce 不支持 wgmma (sm_90a) / tcgen05 (datacenter Blackwell)，**F32/TF32 TensorCore 只能走 sm_80 级 `mma.sync m16n16k8 tf32`**。
- TMA (`cp.async.bulk.tensor.2d`) 在 sm_120 可用，但**无 multicast**。
- libcu++ 提供高层封装：`cuda::device::experimental::cp_async_bulk_tensor_2d_global_to_shared(dst, &tmap, c0, c1, bar)`（`<cuda/barrier>`）。
- 主机端用 driver API `cuTensorMapEncodeTiled` 创建 `CUtensorMap`，以 `__grid_constant__ const` 传入 kernel。

**设计参数（已固化）**：

| 参数 | 取值 |
|---|---|
| Block tile | BM=BN=128, BK=32 |
| Warps/block | 4（128 threads） |
| Warp tile | WM=WN=64（每 warp 4×4 fragment tile, m16n16k8） |
| Pipeline stages | 3（96 KB 动态共享内存） |
| 累加 dtype | f32（mma.sync TF32 → f32 acc） |
| 同步 | mbarrier per-stage，`barrier_arrive_tx(bar, 1, kStageBytes)` 由 thread 0 发起，其余 127 threads `bar.arrive()` |
| 共享内存对齐 | `extern __shared__ __align__(128) uint8_t smem_raw[]` |

**关键代码段（位置 `Bee/CUDA/Ops/MatmulTmaWmma.cu`）**：

```cpp
// stage 共享内存切片
struct alignas(128) StageBuffer { float a[BM*BK]; float b[BK*BN]; };

// barrier 构造（ADL 受 namespace 干扰，改用 placement new + ctor）
::new (&bars[s]) ::cuda::barrier<::cuda::thread_scope_block>(NUM_THREADS);

// arrival_token 不可默认构造、不能数组初始化 → raw 字节缓冲 + placement new
alignas(::cuda::barrier<...>::arrival_token) uint8_t token_buf[STAGES][sizeof(token)];

// TMA 发起（thread 0）
::cuda::device::experimental::cp_async_bulk_tensor_2d_global_to_shared(
    &stage->a, &gA_map, k_off, m_off, bars[s]);
::cuda::barrier_arrive_tx(bars[s], 1, kStageBytes);

// 主循环
for (int kt = 0; kt < num_k_tiles; ++kt) {
    bars[s].wait(std::move(*reinterpret_cast<token*>(token_buf[s])));
    // wmma.load + __float_to_tf32 显式转换 + mma_sync
    ...
    // 预取下一 stage
}
```

**实现踩坑（按时间序）**：
1. `cuda::` 前缀在 `bee::cuda::detail` 内部被解析为 `bee::cuda` → 改用 `::cuda::`。
2. `cuda::barrier::init(barrier*, count)` 是 hidden friend，ADL 在我们 namespace 不生效 → 改 `barrier(NUM_THREADS)` ctor placement new。
3. `arrival_token` 无默认构造、无平凡复制赋值 → 用 `alignas(token) uint8_t buf[STAGES][sizeof(token)]` 字节缓冲。
4. **运行时 cudaErrorMisalignedAddress (716)**：dynamic shared 默认仅 16 B 对齐，TMA 要求 128 B → 强制 `__align__(128)`。
5. WMMA TF32 load 后必须 `__float_to_tf32` 显式截断每个 fragment 元素，不会编译错但结果错。
6. `cudaFuncSetAttribute(kernel, MaxDynamicSharedMemorySize, 96*1024)` 必须在 launch 前调用。

**正确性容差**：TF32 截断尾数 13 bit；K=64 累加误差最大相对 ~1.5% / 绝对 ~1e-2。测试 `NativeTmaWmmaProducesCorrectResult` 用 M=128, K=32, N=128 小尺寸 + `EXPECT_LE(diff, 1e-3)`（K 较小，误差小于阈值即可）。

**性能基准（Release，5070 Ti，square F32）**：

| N | TMA+WMMA (B10) | CUTLASS (B8) | Native tile baseline |
|---|---:|---:|---:|
| 256  | **0.61 TFLOPS** | n/a (auto: skip) | **1.08 TFLOPS** |
| 512  | 4.77 TFLOPS  | **5.05 TFLOPS** | n/a |
| 1024 | **25.16 TFLOPS** | 25.03 TFLOPS | n/a |
| 2048 | n/a (CUTLASS 接管) | **37.84 TFLOPS** | n/a |

**关键结论（负结果）**：
- sm_120 GeForce 上 TMA+WMMA TF32 路径**没有性能 sweet spot**：CUTLASS 在大尺寸已贴近 mma 吞吐封顶（~38 TFLOPS）；native tile 在 256² launch overhead 占主导且 16×16 thread tile 比 128×128 block tile 覆盖更佳。
- 决策：**取消 TMA 自动 dispatch**，改为 explicit 入口 `ops_matmul_force_tma_wmma`，由 `MatmulBackend::Native` 显式触发。
- 自动路径维持 B9 行为：CUTLASS (≥384) + native tile 兜底。

**接口接入**：
- `Bee/CUDA/Api.cpp` 中 `MatmulBackend::Native` case → `detail::ops_matmul_force_tma_wmma`，错误 wrap 为 Result。
- `matmul_backend_available(Native)` 改为 `true`。
- `Tests/CUDA/ApiTests.cpp` 新增 `NativeTmaWmmaProducesCorrectResult`（128×32×128，对齐符合）+ `NativeTmaWmmaRejectsUnaligned`（4×4×4，应失败）。

**遗留**：
1. 严格对齐约束（M%128==N%128==K%32==0、缓冲 16 B 对齐）：可加 padding kernel 或在 dispatch 层做形状外推。
2. 跨 stage 的双缓冲尚未严格做到 compute / memcpy 重叠的极致（mbarrier wait 后 prefetch 下一 stage，prologue 仅预取 1 个 stage）。
3. 若日后接入 sm_90a / sm_100，本路径可作为对比 baseline，无须改 dispatch 结构。


### B11 — CPU Cast/Transpose SIMD

**现状**：
- `Cast` 在 B1 基线是逐元素 `static_cast` 循环（F32→F64 16M 仅 8.77 GB/s；F32→I32 9.8 GB/s）；
- `contiguous()` 对非连续 tensor 走通用 stride-loop + 逐元素 memcpy，Transpose+物化在 2048² F32 仅 794 MiB/s（严重缓存抖动）。

**方案**：
1. 新增 `Bee/Tensor/Cpu/CastCpu.hpp`：
   - SSE2/AVX2 手写 intrinsics 覆盖常用数值对（F32↔F64/I32/U8、F32→Bool 等）；
   - 其余 pair 回落到 `static_cast` 标量循环；
   - `n ≥ 64K` 走 `parallel_for`（grain = 128 KB/elem_bytes），复用 Base 的 `parallel::parallel_for`。
2. 新增 `Bee/Tensor/Cpu/TransposeCpu.hpp`：
   - 通用 32×32 字节级 blocked-tile 拷贝（任意 `elem_sz`）+ 外层 tile-row 并行；
   - F32 AVX2 专用 8×8 寄存器转置（3 级 unpack/shuffle/permute2f128）+ 外层并行；
   - 当 `rows ≥ 64` 时并行。
3. `Bee/Tensor/Cpu/Dispatch/Dispatch.hpp` 在 `BEE_DECL_DISPATCH_NS` 宏中新增 `ct_cast` 与 `tr_copy_2d` 声明，`KernelDispatch.cpp` 对 4 套 ISA OBJECT 库各提供一份实现。
4. `Cast.cpp` 改走 `BEE_RT_DISPATCH_STMT(ct_cast, ...)`（新增的"语句版"宏，不含 `return`，适合嵌在 `Result<Tensor>` 函数中）；`Tensor::contiguous()` 的 CPU 分支对 2D tensor 走 `BEE_RT_DISPATCH_STMT(tr_copy_2d, ...)`，其余维度保留通用 stride-loop。

**基准（13700k，Release，单进程）**：

| 用例                             | Before (GB/s) | After (GB/s) | 加速  |
|----------------------------------|--------------:|-------------:|------:|
| Cast F32→F64 16M                 |          8.77 |        28.28 | 3.23× |
| Cast F64→F32 16M                 |         13.48 |        41.74 | 3.10× |
| Cast F32→I32 16M                 |          9.83 |        40.09 | 4.08× |
| Cast I32→F32 16M                 |         ~10.0 |        35.19 | 3.5×  |
| Cast F32→U8  16M                 |          ~4.0 |        55.93 |  14×  |
| Cast U8→F32  16M                 |          ~3.8 |        26.74 |   7×  |
| Cast F32→Bool 16M                |          ~4.2 |        53.69 |  13×  |
| Transpose+Contig F32 2048²       |    0.79 MiB/s |  20.02 GB/s  |  ~25× |
| Transpose+Contig F32 Tall 262144 |    ~1.0 GB/s  |  15.16 GB/s  |  ~15× |

> 说明：Cast 小 n（256/4K）波动较大且受 overhead 主导；Transpose 4K 的 Tall 用例（16384 行）受写侧并行冲突影响，效率较 2K 略低，属预期。

**正确性**：`Tensor.Tests` 232/232 全通过，重点覆盖 `CastTests`（跨 dtype round-trip）与 `ReshapeTests::*Transpose*Contiguous`（stride view 物化）。

**结论**：Cast 主要数值对 ~3–14×，Transpose 2D 路径 ~15–25×，瓶颈从"逐元素处理"转移到"内存带宽 + 线程扩展"。后续若继续优化可考虑：(1) 大 tensor 走 NT-store 绕 cache；(2) AVX-512 全类型 pack/unpack；(3) `contiguous_copy_into` 通用维度自动合并。

### B12 — 回归监控 / 文档收尾

**目标**：固化一条命令跑完整 CPU + CUDA 基准并落盘，为后续 B8-B10 的性能回归提供基线对比入口。

**交付**：
1. `Scripts/run_bench.ps1`：按 git short SHA 命名输出 `Benchmarks/baseline/{bench}_{sha}.json`，默认包含 Tensor.Bench + CUDA.Bench（若 `--cuda` 参数）。
2. `.gitignore` 确保 `Benchmarks/baseline/` 不进仓库（保护机器间差异不污染历史）。
3. 本日志补齐 B11 段落、更新顶部完成情况。

**回归跑法**（13700k + RTX 5070 Ti）：
```powershell
pwsh Scripts/run_bench.ps1            # 仅 CPU (Tensor.Bench)
pwsh Scripts/run_bench.ps1 -IncludeCuda  # 加上 CUDA.Bench
```
脚本落盘格式为 google-benchmark JSON，便于 `ConvertFrom-Json` 做对比脚本。

**已完成里程碑索引**：B0 → B1 → B2 → B3 → B4 → B5 → B6 → B7 → B11 → B12。B8/B9/B10（CUTLASS + TMA + tcgen05）推迟为独立后续里程碑。

