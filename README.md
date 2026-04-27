# Bee

> Bee - 基于现代 C++20 的基础工具库。

[![License](https://img.shields.io/badge/License-MIT-green.svg)](LICENSE)
[![Status](https://img.shields.io/badge/Status-Early%20Stage-orange.svg)](#路线图)

## 目录
- [Bee](#bee)
  - [目录](#目录)
  - [项目简介](#项目简介)
  - [核心特性](#核心特性)
  - [AI 基础组件状态](#ai-基础组件状态)
  - [适用场景](#适用场景)
  - [许可证](#许可证)

## 项目简介
Bee 是我准备长期构建和打磨的 C++ 基础库项目，目标是为个人技术项目提供稳定、清晰、可复用的底层能力。

项目将基于现代 C++20 开发，并持续吸收优秀开源项目中的实践经验与设计思想。

## 核心特性
- [ ] 日志、断言与调试支持设施
- [ ] 数学模块（服务图形与引擎）
- [ ] 基础容器与算法
- [ ] 并发与任务系统基础封装
- [ ] 序列化
- [ ] ...

> 当前仓库仍处于早期阶段，接口与目录结构会在迭代中逐步稳定。

## AI 基础组件状态
Bee 的 Tensor、CUDA 与 SIMD 模块共同构成 MNIST MLP 与微型 LLM 推理系统的底层张量基础设施：

- **Tensor**：作为 MNIST MLP 与微型 LLM 推理系统的底层张量库，提供 CPU/CUDA Tensor、native CUDA softmax/ReLU/Sigmoid/RMSNorm/RoPE/Embedding，以及 F16/BF16 CUDA GEMM（F32 累加并输出 F32）。模型结构、训练循环、优化器、tokenizer、权重加载与采样仍由应用层承担。
- **CUDA**：提供同步可观察的基础 runtime API、内存/拷贝/事件/workspace、elementwise、cast、reduce、matmul、strided copy、random 与 AI primitive baseline kernels。第一阶段优先正确性与语义一致，后续再接入更高性能的 cuBLASLt/CUTLASS/fused attention。
- **SIMD**：保留运行期门控的 Scalar/SSE2/AVX2/AVX512 后端作为 CPU 路径基础，后续继续补 FMA、稳定 exp、sin/cos、GELU/SiLU 与量化原语。

## 适用场景
Bee 主要考虑面向以下项目方向：
- 游戏开发
- 游戏引擎
- 渲染器
- AI 推理与训练框架
- 其他需要高性能 C++ 基础设施的实验性项目

## 许可证
本项目使用 MIT 许可证
