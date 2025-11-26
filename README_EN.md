# Luma Engine

**Modern · Modular · Data-Driven · High-Performance 2D Game Engine**
Built with **C++20** and **C# (.NET 9 CoreCLR)**, designed to become a powerful alternative to Unity 2D.

---

* [中文文档](README.md)
* [Architecture](ARCHITECTURE_EN.md)

---

## Table of Contents

* [Overview & Philosophy](#overview--philosophy)
* [Key Features & Performance](#key-features--performance)

  * [Performance Comparison: Luma vs. Unity DOTS](#performance-comparison-luma-vs-unity-dots)
  * [Feature Summary](#feature-summary)
* [Tech Stack](#tech-stack)
* [Rendering System (New: Nut Renderer)](#rendering-system-new-nut-renderer)
* [Shader Module System (New)](#shader-module-system-new)
* [Getting Started](#getting-started)
* [Editor Debugging Support](#editor-debugging-support)
* [Core System Overview](#core-system-overview)
* [Project Status & Roadmap](#project-status--roadmap)
* [Contribution Guide](#contribution-guide)
* [License](#license)
* [Community & Support](#community--support)

---

## Overview & Philosophy

Luma Engine targets modern 2D game development with extreme performance and a streamlined workflow.

Core principles:

* **Data-Driven** – scenes, entities, components, animations are all serialized & editable.
* **Modular Architecture** – rendering, physics, audio, UI are fully decoupled.
* **Performance First** – ECS + high-parallel JobSystem.

---

## Key Features & Performance

### Performance Comparison: Luma vs. Unity DOTS

> Unity version: 6.1 LTS
> Same hardware & equivalent scene setups.

|  Entities | Luma Engine (FPS) | Unity DOTS (FPS) | Speed-up |
| :-------: | :---------------: | :--------------: | :------: |
|  100,000  |        ~200       |        ~30       |   ~6.6×  |
|  200,000  |        ~112       |        ~15       |   ~7.0×  |
| 1,000,000 |        ~25        |        ~2        |  ~12.5×  |

**Physics Simulation (Box2D, 10,000 dynamic bodies)**

|      Metric     |   Luma  |   Unity  | Speed-up |
| :-------------: | :-----: | :------: | :------: |
|    Frame Time   | 2.40 ms | 45.45 ms |   18.9×  |
| Theoretical FPS |   ~416  |    ~22   |   18.9×  |

---

### Feature Summary

* **Modern ECS (EnTT)**
* **Parallel JobSystem (C++/C# collaborative)**
* **Visual Blueprint System (compiled to C#)**
* **C++/C# Interop + Hot Reloading**
* **Physics, Tilemap, UI, Audio Systems**
* **Hybrid Rendering: Skia + Nut Renderer (NEW)**
* **Custom Material Pipeline & Instanced Rendering (NEW)**
* **Shader Module System + Prewarming (NEW)**
* **Cross-Platform Editor: Windows / Linux / Android**
* **Multi-backend Build Support: DX12 / DX11 / Vulkan / OpenGL / GLES**

---

## Tech Stack

| Category      | Technology                 | Description                       |
| :------------ | :------------------------- | :-------------------------------- |
| Languages     | C++20 / C# (.NET 9)        | Performance + flexibility         |
| ECS           | EnTT                       | High-performance ECS              |
| Rendering     | **Skia + Nut (wgpu/dawn)** | 2D drawing + custom GPU pipelines |
| Physics       | Box2D                      | Real-time 2D physics              |
| Audio         | SDL3 Audio                 | Spatial mixing                    |
| Editor UI     | Dear ImGui                 | Immediate-mode tools              |
| Serialization | yaml-cpp / json            | Data-driven workflows             |

---

## Rendering System (New: Nut Renderer)

Luma now uses a **hybrid rendering architecture**:

### **Skia Rendering Path**

* Used for **UI**, vector graphics, debug shapes
* Easy integration, high compatibility

### **Nut Renderer (NEW)**

A modern GPU renderer built on **Dawn (wgpu)**:

* Supports **custom material pipelines**
* High-performance **instanced rendering**
* Large-scale **batch rendering**
* Fully extensible GPU pipeline

### **Hybrid Pipeline Flow**

```
UI / Debug → Skia
Sprites / Tilemap / Custom Mesh → Nut Renderer
```

Both paths composite seamlessly into the final frame.

---

## Shader Module System (New)

A fully modular shader system with import/export semantics.

### **Exporting modules**

```glsl
export ModuleA;
export F.C1;
```

### **Importing modules**

```glsl
import ModuleA;
import ::C1;
```

Supported features:

* Multi-module shader composition
* Export/import symbols
* Module linking across shader files
* Automatic dependency resolution

### Shader Prewarming (Preheat)

All shaders can be precompiled on startup:

* Prevent first-frame stutters
* Load + compile on background threads
* Uses JobSystem for multi-threaded compilation

---

## Getting Started

### Requirements

1. Install Git, CMake (≥3.21), Vulkan SDK, modern C++ compiler
2. Use **Vcpkg** to manage dependencies
3. Extract **Skia** and **CoreCLR**:

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

## Editor Debugging Support

Supports **Rider** and **Visual Studio 2022/2026** for C# script breakpoints directly inside the editor runtime.

---

## Core System Overview

* **Asset System** – GUID-based import pipeline, metadata + caching
* **Script System** – CoreCLR hosting + P/Invoke bridge
* **Rendering System** – Skia+Nut hybrid, interpolation, batching
* **Physics** – Box2D fixed-step with Transform sync
* **Audio** – multi-thread spatial mixing
* **JobSystem** – work-stealing scheduler
* **Animation** – parametric state machine + visual editor

---

## Project Status & Roadmap

### Completed

* ECS / JobSystem / Animation / UI / Tilemap
* Hot Reload, Interop, Profiler
* Cross-platform editor (Win/Linux/Android)
* Multi-backend rendering (DX12/DX11/VK/GL/GLES)
* **Nut Renderer (NEW)**
* **Shader Module System + Prewarm (NEW)**

### Roadmap

| Priority | Feature                 | Status      |  Target |
| :------: | :---------------------- | :---------- | :-----: |
|  Medium  | Expanded C-API          | Planned     | 2025 Q3 |
|  Medium  | Nut Renderer public API | In Progress | 2025 Q3 |
|  Medium  | Custom Material Editor  | Planned     | 2025 Q4 |
|    Low   | Editor UI modernization | Researching | 2025 Q4 |
|    Low   | 2D Particle System      | Planned     | 2025 Q4 |

---

## Contribution Guide

* Types/functions: PascalCase
* Variables: camelCase
* Public APIs: Doxygen format

Workflow:

1. Branch from `master`
2. Build & test
3. Submit PR with details

---

## License

Released under the **MIT License**.

---

## Community & Support

| Platform     | Link                                                       |
| :----------- | :--------------------------------------------------------- |
| **Discord**  | [https://discord.gg/BJBXcRkh](https://discord.gg/BJBXcRkh) |
| **QQ Group** | 913635492                                                  |

Business contact:
📧 **Email:** [gug777514@gmail.com](mailto:gug777514@gmail.com)

---

**Luma Engine** — Lighting up the next generation of 2D game development.