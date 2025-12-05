# **Luma Engine**

**Modern · Modular · Data-Driven · High-Performance 2D Game Engine**
Built on **C++20** and **C# (.NET 9 CoreCLR)**, aiming to be a powerful alternative to Unity 2D.

---

* [中文版本](README.md)
* [Engine Architecture](ARCHITECTURE_EN.md)

---

## Table of Contents

* [Overview & Design Philosophy](#overview--design-philosophy)
* [Core Features & Performance Overview](#core-features--performance-overview)
  * [Performance Comparison: Luma vs. Unity DOTS](#performance-comparison-luma-vs-unity-dots)
  * [Feature Overview](#feature-overview)
* [Tech Stack](#tech-stack)
* [Rendering System (New Nut Renderer)](#rendering-system)
* [Shader Module System (New)](#shader-module-system)
* [Getting Started](#getting-started)
* [Editor Breakpoint Debugging Support](#editor-breakpoint-debugging-support)
* [Core System Overview](#core-system-overview)
* [Project Status & Roadmap](#project-status--roadmap)
* [Contributing Guide](#contributing-guide)
* [License](#license)
* [Community & Support](#community--support)

---

## Overview & Design Philosophy

The Luma Engine focuses on building a 2D game engine with ultimate performance and a modern workflow:

* **Data-Driven**: Scenes, components, and animations revolve around serializable data.
* **Modular Architecture**: Rendering, physics, and audio systems can be replaced independently.
* **Performance First**: ECS + parallel JobSystem.

---

## Core Features & Performance Overview

### Performance Comparison: Luma vs. Unity DOTS

> Unity Version: 6.1 LTS; Same hardware and equivalent scene setup.

| Entity Count | Luma Engine (FPS) | Unity DOTS (FPS) | Performance Multiplier |
| :----------- | :---------------- | :--------------- | :--------------------- |
| 100,000      | ~200              | ~30              | ~6.6×                 |
| 200,000      | ~112              | ~15              | ~7.0×                 |
| 1,000,000    | ~25               | ~2               | ~12.5×                |

**Physics Simulation (Box2D, 10,000 Dynamic Rigidbodies)**

| Metric                | Luma     | Unity    | Performance Multiplier |
| :-------------------- | :------- | :------- | :--------------------- |
| Total Frame Time      | 2.40 ms  | 45.45 ms | 18.9×                 |
| Theoretical FPS       | ~416     | ~22      | 18.9×                 |

---

### Feature Overview

* **Modern ECS (EnTT)**
* **JobSystem Parallel Computing (C++/C# Collaboration)**
* **Blueprint Node System (Compilable to C#)**
* **C++ / C# Hot Reload Interop**
* **Box2D Physics, Tilemap, UI, Audio System**
* **Skia + Nut Hybrid Rendering Pipeline**
* **Custom Material Pipeline / Instanced Rendering**
* **Shader Module System + Shader Preheat**
* **Cross-Platform Editor: Windows / Linux / Android**
* **Release Support: DX12 / DX11 / Vulkan / OpenGL / GLES**
* **High-Performance Particle System**
  * Particle Collision (Works with Physics System)
  * Animated Particles (Flipbook/Frame Animation)
  * Custom Particle Materials
  * Effector Combination (Modifiers Stack)
  * Full Lifecycle Management

---

## Tech Stack

| Category     | Technology                 | Description                        |
| :----------- | :------------------------- | :--------------------------------- |
| Core Language | C++20 / C# (.NET 9)        | Performance & Flexibility          |
| ECS Framework | EnTT                       | High-Performance ECS               |
| Render Backend | **Skia + Nut(wgpu/dawn)**  | 2D + Custom GPU Pipeline           |
| Physics Engine | Box2D                      | 2D Dynamics                        |
| Audio        | SDL3 Audio                 | Multichannel Mixing                |
| Editor UI    | Dear ImGui                 | Immediate Mode UI                  |
| Serialization | yaml-cpp / json            | Data-Driven                        |

---

## Rendering System

Luma's rendering system now supports a **Hybrid Dual-Path Rendering** approach:

### **Skia Render Path**

* Primarily used for **UI / Vector / Debug Graphics**
* High compatibility, high development efficiency

### **Nut Renderer**

A modern GPU renderer based on **Dawn (wgpu)**:

* Supports **Custom Material Shader Pipeline**
* Supports **Instanced Rendering**
* Supports **Batch Rendering**
* High-performance and fully extensible

### **Hybrid Rendering Architecture**

```
UI / Debug → Skia
Sprites / Tilemap / Custom Mesh / Particles → Nut Renderer
```

Both are composited in layers within the same frame.

---

## Shader Module System

A new shader building system supporting modular semantics:

### **Module Export**

```glsl
export ModuleA;
export F.C1;
```

### **Module Import**

```glsl
import ModuleA;
import ::C1;
```

Supports cross-module references and symbol aggregation.

### Feature Summary

* Multi-module composition
* Export / Import symbols
* Supports `shader1 → shader2` module injection
* Automatic dependency resolution at build time

### **Shader Preheat**

Precompiles all shaders before runtime:

* Avoids first-frame stutter
* Supports background parallel loading
* Leverages JobSystem for multi-threaded compilation

---

## Getting Started

### Prerequisites

1.  Install: Git, CMake (≥ 3.21), Vulkan SDK, a modern C++ compiler.
2.  Use Vcpkg for dependency management.
3.  Manually extract Skia and CoreCLR:

```
Luma/
├── External/
│   ├── coreclr-win-x64/
│   └── skia-win/
```

### Build

```bash
git clone https://github.com/NGLSG/Luma.git
cd Luma
mkdir build && cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=E:/vcpkg/scripts/buildsystems/vcpkg.cmake
cmake --build . --config Release
```

---

## Editor Breakpoint Debugging Support

Supports **C# script breakpoint debugging** in Rider and Visual Studio 2022/2026.

---

## Core System Overview

* **Asset System**: GUID management + Cache + Hot Reload
* **Script System**: CoreCLR + P/Invoke bidirectional calls
* **Rendering System**: Skia + Nut, interpolation, batching
* **Physics System**: Box2D fixed timestep
* **Audio System**: Multithreaded spatial audio mixing
* **Task Scheduler**: Work-stealing algorithm
* **Animation System**: Parameterized state machine
* **Particle System**:
  * High-performance implementation based on Nut Renderer
  * Particle Collision (interacts with Physics system)
  * Effector Combination (Modifiers)
  * Animated Particles (Sprite Sheet / Flipbook)
  * Custom Material Particles
  * Full Lifecycle Management (Spawn/Update/Recycle)

---

## Project Status & Roadmap

### Completed Features

* ECS / JobSystem / Animation State Machine / UI / Tilemap
* C# Hot Reload, Bidirectional Interop, Profiler
* Multi-Backend Rendering (DX12/DX11/VK/GL/GLES)
* **Nut Renderer**
* **Custom Materials**
* **Shader Module System + Shader Preheat**
* **Particle System**

---

### Roadmap

| Priority | Feature                          | Status  | Target Timeline |
| :------- | :------------------------------- | :------ | :-------------- |
| Medium   | **Editor Adaptation for Android Pad Devices** | Planned | 2025 Q4         |
| Low      | Editor UI Modernization          | Researching | 2025 Q4         |

---

## Contributing Guide

* Types/Functions: PascalCase
* Variables: camelCase
* Public API: Doxygen comments
* Workflow:
  1.  Create a branch from `master`
  2.  Pass build and tests
  3.  Submit a PR with description

---

## License

Open source under the **MIT License**.

---

## Community & Support

| Platform       | How to Join                                                    |
| :------------- | :------------------------------------------------------------- |
| **Discord**    | [https://discord.gg/BJBXcRkh](https://discord.gg/BJBXcRkh)     |
| **QQ Group**   | 913635492                                                      |

Business Inquiries:
📧 **Email**: [gug777514@gmail.com](mailto:gug777514@gmail.com)

---

**Luma Engine** — The next-generation game engine lighting up the 2D world.