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

### B2 — CPU ElementWise 多线程 + NT-store（TODO）

### B3 — CPU Reduce 多线程 + SIMD 水平归约（TODO）

### B4 — CPU GEMM tiling + AVX2 microkernel（TODO）

### B5 — CUDA ElementWise float4 向量化（TODO）

### B6 — CUDA Reduce warp shuffle（TODO）

### B7 — CUDA Random 原生 Philox（TODO）

### B8 — CUDA Matmul L2 CUTLASS（TODO）

### B9 — CUDA Transpose TMA（TODO）

### B10 — CUDA Matmul L3 TMA + tcgen05.mma（TODO）

### B11 — CPU Cast/Transpose SIMD（TODO）

### B12 — 回归监控 / 文档收尾（TODO）
