# **Luma 引擎**

**现代化 · 模块化 · 数据驱动 · 高性能 2D 游戏引擎**
基于 **C++20** 与 **C# (.NET 9 CoreCLR)** 构建，旨在成为 Unity 2D 的强大替代方案。

---

* [English Version](README_EN.md)
* [引擎架构](ARCHITECTURE.md)

---

## 目录

* [概览与设计哲学](#概览与设计哲学)
* [核心特性与性能概览](#核心特性与性能概览)

  * [性能对比：Luma vs. Unity DOTS](#性能对比luma-vs-unity-dots)
  * [功能一览](#功能一览)
* [技术栈](#技术栈)
* [渲染系统说明（新增 Nut 渲染器）](#渲染系统说明)
* [Shader 模块系统（新增）](#shader-模块系统)
* [快速开始](#快速开始)
* [编辑器断点调试支持](#编辑器断点调试支持)
* [核心系统概述](#核心系统概述)
* [项目状态与路线图](#项目状态与路线图)
* [贡献指南](#贡献指南)
* [许可证](#许可证)
* [社区与支持](#社区与支持)

---

## 概览与设计哲学

Luma 引擎专注于构建极致性能与现代化工作流的 2D 游戏引擎：

* **数据驱动**：场景、组件、动画以可序列化数据为核心。
* **模块化架构**：渲染、物理、音频均可独立替换。
* **性能优先**：ECS + 并行 JobSystem。

---

## 核心特性与性能概览

### 性能对比：Luma vs. Unity DOTS

> Unity 版本：6.1 LTS；同一硬件与等价场景设置。

|    实体数量   | Luma 引擎 (FPS) | Unity DOTS (FPS) |  性能倍数  |
| :-------: | :-----------: | :--------------: | :----: |
|  100,000  |      ~200     |        ~30       |  ~6.6× |
|  200,000  |      ~112     |        ~15       |  ~7.0× |
| 1,000,000 |      ~25      |        ~2        | ~12.5× |

**物理模拟（Box2D，10,000 动态刚体）**

|   指标   |   Luma  |   Unity  |  性能倍数 |
| :----: | :-----: | :------: | :---: |
|  总帧时间  | 2.40 ms | 45.45 ms | 18.9× |
| 理论 FPS |   ~416  |    ~22   | 18.9× |

---

### 功能一览

* **现代 ECS（EnTT）**
* **JobSystem 并行计算（C++/C# 协同）**
* **蓝图节点系统（可编译为 C#）**
* **C++ / C# 热重载互操作**
* **Box2D 物理、Tilemap、UI、音频系统**
* **Skia + Nut 混合渲染管线**
* **自定义材质管线 / 实例化渲染**
* **Shader 模块系统 + Shader 预热**
* **跨平台编辑器：Windows / Linux / Android**
* **发行支持：DX12 / DX11 / Vulkan / OpenGL / GLES**
* **高性能粒子系统**

  * 粒子碰撞（与物理系统协作）
  * 动画粒子（Flipbook/帧动画）
  * 粒子自定义材质
  * 效果器组合（Modifiers Stack）
  * 完整生命周期管理

---

## 技术栈

| 分类     | 技术                        | 说明                    |
| :----- | :------------------------ | :-------------------- |
| 核心语言   | C++20 / C# (.NET 9)       | 性能与灵活性                |
| ECS 框架 | EnTT                      | 高性能 ECS               |
| 渲染后端   | **Skia + Nut(wgpu/dawn)** | 2D + 自定义 GPU pipeline |
| 物理引擎   | Box2D                     | 2D 动力学                |
| 音频     | SDL3 Audio                | 多声道混音                 |
| 编辑器 UI | Dear ImGui                | 即时模式 UI               |
| 序列化    | yaml-cpp / json           | 数据驱动                  |

---

## 渲染系统说明

Luma 的渲染系统现支持 **双路径混合渲染**：

### **Skia 渲染路径**

* 主要用于 **UI / 矢量 / 调试图形**
* 兼容性高、开发效率高

### **Nut Renderer**

基于 **Dawn (wgpu)** 的现代 GPU 渲染器：

* 支持 **自定义材质 Shader Pipeline**
* 支持 **实例化渲染（Instancing）**
* 支持 **批处理渲染**
* 高性能且完全可扩展

### **混合渲染架构**

```
UI / Debug → Skia
Sprites / Tilemap / Custom Mesh / Particles → Nut Renderer
```

两者在同一帧内进行分层合成。

---

## Shader 模块系统

全新的 shader 构建系统支持模块化语义：

### **模块导出**

```glsl
export ModuleA;
export F.C1;
```

### **模块导入**

```glsl
import ModuleA;
import ::C1;
```

支持跨模块引用与符号聚合。

### 特性总结

* 多模块组合
* 导出 / 导入符号
* 支持 `shader1 → shader2` 模块注入
* 构建时自动依赖解析

### **Shader 预热（Shader Preheat）**

运行前预编译所有 shader：

* 避免首帧卡顿
* 支持后台并行加载
* 借助 JobSystem 进行多线程编译

---

## 快速开始

### 环境依赖

1. 安装：Git、CMake (≥ 3.21)、Vulkan SDK、现代 C++ 编译器。
2. 使用 Vcpkg 管理依赖。
3. 手动解压 Skia 与 CoreCLR：

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

---

## 编辑器断点调试支持

支持 Rider 与 Visual Studio 2022/2026 的 **C# 脚本断点调试**。

---

## 核心系统概述

* **资产系统**：GUID 管理 + 缓存 + 热重载
* **脚本系统**：CoreCLR + P/Invoke 双向调用
* **渲染系统**：Skia + Nut、插值补帧、批处理
* **物理系统**：Box2D 固定步长
* **音频系统**：多线程空间混音
* **任务调度**：工作窃取算法
* **动画系统**：参数化状态机
* **粒子系统**：

  * 基于 Nut Renderer 的高性能实现
  * 粒子碰撞（与物理系统交互）
  * 效果器组合（Modifiers）
  * 动画粒子（Sprite Sheet / Flipbook）
  * 自定义材质粒子
  * 完整生命周期管理（Spawn/Update/Recycle）

---

## 项目状态与路线图

### 已完成功能

* ECS / JobSystem / 动画状态机 / UI / Tilemap
* C# 热重载、双向互操作、Profiler
* 多后端渲染（DX12/DX11/VK/GL/GLES）
* **Nut 渲染器**
* **自定义材质**
* **Shader 模块 + Shader 预热**
* **粒子系统**

---

### 路线图

| 优先级 | 功能                          | 状态  |   目标时间  |
| :-: |:----------------------------| :-- | :-----: |
|  中  | **Editor 适配 Android Pad设备** | 计划中 | 2025 Q4 |
|  低  | 编辑器 UI 现代化                  | 研究中 | 2025 Q4 |

---

## 贡献指南

* 类型/函数：PascalCase
* 变量：camelCase
* 公共 API：Doxygen 注释
* 工作流：

  1. 从 `master` 创建分支
  2. 通过构建与测试
  3. 发起 PR 并附上说明

---

## 许可证

基于 **MIT License** 开源。

---

## 社区与支持

| 平台          | 加入方式                                                       |
| :---------- | :--------------------------------------------------------- |
| **Discord** | [https://discord.gg/BJBXcRkh](https://discord.gg/BJBXcRkh) |
| **QQ 群**    | 913635492                                                  |

商务合作：
📧 **Email**：[gug777514@gmail.com](mailto:gug777514@gmail.com)

---

**Luma Engine** — 点亮 2D 世界的下一代游戏引擎。

---