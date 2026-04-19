# Bee Benchmarks

基于 [google-benchmark](https://github.com/google/benchmark) 的性能基准集。

## 启用与构建

基准默认关闭，需显式打开：

```powershell
cmake -S . -B build -DBEE_BUILD_BENCHMARKS=ON
cmake --build build --target Task.Bench --config Release
```

依赖：`google-benchmark` 须可通过 `find_package(benchmark CONFIG)` 找到
（本仓库 vcpkg toolchain 下开箱即用）。未找到时 `BEE_BUILD_BENCHMARKS`
会被自动关闭，不影响主构建。

## 运行

```powershell
# 全部
.\build\Benchmarks\Task\Release\Task.Bench.exe

# 按名过滤
.\build\Benchmarks\Task\Release\Task.Bench.exe --benchmark_filter=BM_TaskSubmitGetMinimal

# 输出 JSON（便于脚本比较）
.\build\Benchmarks\Task\Release\Task.Bench.exe --benchmark_format=json --benchmark_out=before.json
```

## 当前基准集

| 文件 | 覆盖点 |
|---|---|
| `Task/TaskSubmitBench.cpp` | Task 单体 `spawn_task + get` 延迟 |
| `Task/WorkPoolThroughputBench.cpp` | WorkPool 单/多生产者吞吐 |
| `Task/TaskGraphScaleBench.cpp` | TaskGraph 在 chain / fan 拓扑下的 execute wall-clock |
| `Task/WhenAllDepthBench.cpp` | `when_all` 宽度扩展 |

## 定位

这些基准是 **P3 候选（NUMA / 对象池 / DCAS）的"重启条件"触发器**。
在没有基准数据之前，任何宣称"某某优化能提升性能"的提议都应被视为
推测而非事实。详见 `Doc/Task 系统设计哲学与改进路线.md` §8.3。

## 新增基准

1. 在对应组件子目录（例如 `Benchmarks/Task/`）新增 `.cpp`。
2. 在该子目录 `CMakeLists.txt` 的 `SOURCES` 列表追加文件名。
3. 在文件内使用 `BENCHMARK(name)->Arg(...)` 注册。

基准源码同样遵循 `Doc/Task 系统设计哲学与改进路线.md` 的五条不变式
——基准 harness 不应成为新的并发风险来源。
