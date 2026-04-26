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
围绕简单 LLM 推理与基础 MNIST 训练，Bee 已补强 Tensor、CUDA 与 SIMD 的底层基础能力：

- **Tensor**：提供稳定 `softmax`、`cross_entropy`，覆盖核心创建、连续化、矩阵乘与批次矩阵乘路径；相关低精度矩阵乘会将 F16/BF16 累加提升到 F32，高层 AI primitive 在 CUDA 张量上保持 CPU bridge 后回写设备的行为。
- **CUDA**：明确 reduce 的 grid cap 仅限制 partial block 数量，不截断输入；直接测试覆盖 reduce、matmul fallback 与 strided copy。
- **SIMD**：保留运行期门控的 Scalar/SSE2/AVX2/AVX512 后端，并补充 AVX2 int32 乘法及边界语义测试。

这些能力适合作为简单 LLM 推理和基础 MNIST 训练的基础设施；模型结构、训练循环、优化器、数据加载等更高层抽象仍应由应用层或框架层承担。

## 适用场景
Bee 主要考虑面向以下项目方向：
- 游戏开发
- 游戏引擎
- 渲染器
- AI 推理与训练框架
- 其他需要高性能 C++ 基础设施的实验性项目

## 许可证
本项目使用 MIT 许可证
