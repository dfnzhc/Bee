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

### B1 — 全量基线（TODO）

（完成 B1 后填入各算子 baseline 数字。）

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
