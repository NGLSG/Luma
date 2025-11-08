# Luma 引擎

**现代化 · 模块化 · 数据驱动 · 高性能 2D 游戏引擎**  
基于 **C++20** 与 **C# (.NET 9 CoreCLR)** 构建，旨在成为 Unity 2D 的强大替代方案。

---

- [English Version](README_EN.md)
- [引擎架构](ARCHITECTURE.md)

---

## 目录

- [概览与设计哲学](#概览与设计哲学)
- [核心特性与性能概览](#核心特性与性能概览)
  - [性能对比：Luma vs. Unity DOTS](#性能对比luma-vs-unity-dots)
  - [功能一览](#功能一览)
- [技术栈](#技术栈)
- [快速开始](#快速开始)
  - [环境与依赖](#环境与依赖)
  - [构建](#构建)
- [编辑器断点调试支持](#编辑器断点调试支持)
- [核心系统概述](#核心系统概述)
- [项目状态与路线图](#项目状态与路线图)
- [贡献指南](#贡献指南)
- [许可证](#许可证)
- [社区与支持](#社区与支持)

---

## 概览与设计哲学

Luma 引擎面向需要极致性能与现代化工作流的 2D 游戏项目。  
其核心设计理念包括：

- **数据驱动**：场景、实体、组件与动画均以数据为核心，支持热重载与可视化编辑。
- **模块化与可扩展**：渲染、物理、音频、UI 等系统完全解耦，便于独立升级与替换。
- **性能优先**：ECS 架构与并行 JobSystem，面向大规模实时运算。

---

## 核心特性与性能概览

### 性能对比：Luma vs. Unity DOTS

> Unity 版本：6.1 LTS；同一硬件、等价场景设置。

| 实体数量 | Luma 引擎 (FPS) | Unity DOTS (FPS) | 性能倍数 |
|:--:|:--:|:--:|:--:|
| 100,000 | ~130 | ~30 | ~4.3× |
| 200,000 | ~60 | ~15 | ~4.0× |
| 1,000,000 | ~15 | ~2 | ~7.0× |

**物理模拟（Box2D，10,000 动态刚体）**

| 指标 | Luma | Unity | 性能倍数 |
|:--:|:--:|:--:|:--:|
| 总帧时间 | 2.40 ms | 45.45 ms | 18.9× |
| 理论 FPS | ~416 | ~22 | 18.9× |

> 注：测试结果仅供参考，实际性能因硬件和场景差异而异。

### 功能一览

- **现代 ECS 架构**（基于 EnTT）
- **JobSystem 并行计算**（C++/C# 协同）
- **可视化蓝图系统**（节点 → 编译型 C#）
- **C++ / C# 互操作 + 热重载**
- **物理 / Tilemap / UI / 音频系统**
- **高性能渲染管线（Skia + Dawn）**
- **跨平台编辑器支持**：Windows / Linux / Android（需外接鼠标）
- **多平台发行包支持**：Windows(DX12, DX11) / Linux(Vulkan, OpenGL) / Android(Vulkan, OpenGL ES)

---

## 技术栈

| 分类 | 技术 | 说明 |
|:--|:--|:--|
| 核心语言 | C++20 / C# (.NET 9) | 高性能与灵活性兼备 |
| 构建系统 | CMake 3.21+ | 跨平台构建 |
| ECS 框架 | EnTT | 高性能实体组件系统 |
| 物理引擎 | Box2D | 实时 2D 动力学 |
| 渲染后端 | Skia + Dawn | GPU 加速渲染 |
| 音频系统 | SDL3 Audio | 多声道混音与空间化 |
| 编辑器 UI | Dear ImGui | 即时模式编辑器界面 |
| 数据序列化 | yaml-cpp / json | YAML / JSON 读写 |

---

## 快速开始

### 环境与依赖

1. 安装：Git、CMake (≥ 3.21)、Vulkan SDK、现代 C++ 编译器（VS 2022 / Clang 14 / GCC 11+）。
2. 使用 **Vcpkg** 管理依赖。
3. 手动解压 **Skia** 与 **CoreCLR** 至 `External/` 目录：

```
Luma/
├── External/
│   ├── coreclr-win-x64/
│   └── skia-win/
```

### 构建

```bash
git clone https://github.com/NGLSG/Luma.git
cd Luma
mkdir build && cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=E:/vcpkg/scripts/buildsystems/vcpkg.cmake
cmake --build . --config Release
```

构建产物位于 `build/bin/Release/`。

---

## 编辑器断点调试支持

Luma 引擎支持 **Rider** 与 **Visual Studio 2022 / 2026 Insider** 的编辑器级断点调试。

---

## 核心系统概述

- **资产系统**：基于 GUID 的导入与缓存机制。
- **脚本系统**：CoreCLR 宿主 + PInvoke 桥接。
- **渲染系统**：插值补帧、批处理、线程安全提交。
- **物理系统**：Box2D 固定步长 + Transform 同步。
- **音频系统**：多线程混音与空间化。
- **任务调度**：工作窃取算法 + 全局任务队列。
- **动画系统**：参数化状态机与可视化编辑支持。

---

## 项目状态与路线图

### 已完成功能

- 资产管线、ECS、JobSystem、动画状态机、蓝图系统、Tilemap、UI、空间音频
- C# 热重载、C++/C# 双向调用、Profiler、物理调试可视化
- 跨平台编辑器（Windows/Linux/Android）
- 多后端渲染管线发行支持（DX12/DX11/Vulkan/OpenGL/GL ES）

### 路线图

| 优先级 | 功能 | 状态 | 目标时间 |
|:--:|:--|:--|:--:|
| 中 | C-API 扩展 | 计划中 | 2025 Q3 |
| 低 | 编辑器界面现代化 | 研究中 | 2025 Q4 |
| 低 | 粒子系统 | 计划中 | 2025 Q4 |

---

## 贡献指南

- **命名规范**：类型/函数用 PascalCase；变量用 camelCase。
- **注释规范**：公共 API 使用 Doxygen 风格。
- **提交流程**：
  1. 从 `master` 创建功能分支；
  2. 通过构建与测试；
  3. 提交 Pull Request 并说明实现细节。

---

## 许可证

本项目基于 [MIT License](LICENSE) 开源。  
欢迎自由使用、修改与分发。

---

## 社区与支持

加入 Luma 开发者社区，共同构建下一代 2D 游戏生态。

| 平台 | 加入方式 |
|:--|:--|
| **Discord** | [https://discord.gg/BJBXcRkh](https://discord.gg/BJBXcRkh) |
| **QQ 群** | 913635492 |

如需商务合作或技术支持：  
📧 **Email**：gug777514@gmail.com

---

**Luma Engine** — 点亮 2D 世界的下一代游戏引擎。
