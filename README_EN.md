# Luma Engine

**Modern · Modular · Data-Driven · High-Performance 2D Game Engine**  
Built on **C++20** and **C# (.NET 9 CoreCLR)** — designed to be a powerful modern alternative to Unity 2D.

---

- [中文版](README.md)
- [Engine Architecture](ARCHITECTURE.md)

---

## Table of Contents

- [Overview & Design Philosophy](#overview--design-philosophy)
- [Core Features & Performance](#core-features--performance)
  - [Performance Comparison: Luma vs. Unity DOTS](#performance-comparison-luma-vs-unity-dots)
  - [Feature Overview](#feature-overview)
- [Tech Stack](#tech-stack)
- [Quick Start](#quick-start)
  - [Environment & Dependencies](#environment--dependencies)
  - [Build](#build)
- [Editor Debugging Support](#editor-debugging-support)
- [Core Systems Overview](#core-systems-overview)
- [Project Status & Roadmap](#project-status--roadmap)
- [Contribution Guide](#contribution-guide)
- [License](#license)
- [Community & Support](#community--support)

---

## Overview & Design Philosophy

Luma Engine is built for 2D game projects demanding extreme performance and modern workflows.  
Its design philosophy focuses on:

- **Data-Driven Architecture** — all scenes, entities, components, and animations are driven by structured data with hot-reload and visual editing.
- **Modularity & Extensibility** — rendering, physics, audio, and UI systems are fully decoupled.
- **Performance First** — ECS and JobSystem optimized for massive real-time computation.

---

## Core Features & Performance

### Performance Comparison: Luma vs. Unity DOTS

> Unity Version: 6.1 LTS; identical hardware and equivalent scene setup.

| Entities | Luma (FPS) | Unity DOTS (FPS) | Speedup |
|:--:|:--:|:--:|:--:|
| 100,000 | ~130 | ~30 | ~4.3× |
| 200,000 | ~60 | ~15 | ~4.0× |
| 1,000,000 | ~15 | ~2 | ~7.0× |

**Physics Simulation (Box2D, 10,000 dynamic bodies)**

| Metric | Luma | Unity | Speedup |
|:--:|:--:|:--:|:--:|
| Total Frame Time | 2.40 ms | 45.45 ms | 18.9× |
| Theoretical FPS | ~416 | ~22 | 18.9× |

> Note: Benchmark results may vary depending on hardware and scene complexity.

### Feature Overview

- **Modern ECS Architecture** (based on EnTT)
- **JobSystem Parallel Execution** (C++/C# hybrid)
- **Visual Blueprint System** (Node → compiled C#)
- **C++ / C# Interop + Hot Reload**
- **Physics / Tilemap / UI / Audio Systems**
- **High-Performance Rendering Pipeline (Skia + Dawn)**
- **Cross-Platform Editor Support**: Windows / Linux / Android (requires mouse)
- **Cross-Platform Runtime Packages**: Windows (DX12, DX11) / Linux (Vulkan, OpenGL) / Android (Vulkan, OpenGL ES)

---

## Tech Stack

| Category | Technology | Description |
|:--|:--|:--|
| Core Languages | C++20 / C# (.NET 9) | High performance and flexibility |
| Build System | CMake 3.21+ | Cross-platform build system |
| ECS Framework | EnTT | High-performance entity-component system |
| Physics Engine | Box2D | Real-time 2D dynamics |
| Rendering Backend | Skia + Dawn | GPU-accelerated rendering |
| Audio System | SDL3 Audio | Multichannel mixing and spatialization |
| Editor UI | Dear ImGui | Immediate mode editor interface |
| Data Serialization | yaml-cpp / json | YAML / JSON I/O |

---

## Quick Start

### Environment & Dependencies

1. Install: Git, CMake (≥ 3.21), Vulkan SDK, modern C++ compiler (VS 2022 / Clang 14 / GCC 11+).
2. Manage dependencies using **Vcpkg**.
3. Extract **Skia** and **CoreCLR** into the `External/` directory:

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

Output binaries are located in `build/bin/Release/`.

---

## Editor Debugging Support

Supports **Rider** and **Visual Studio 2022 / 2026 Insider** for full in-editor C# debugging.

---

## Core Systems Overview

- **Asset System**: GUID-based import & caching.
- **Script System**: CoreCLR host with PInvoke bridge.
- **Rendering System**: Interpolated frame updates, batching, thread-safe submission.
- **Physics System**: Fixed timestep with transform sync.
- **Audio System**: Multithreaded mixing and 3D spatialization.
- **Task Scheduler**: Work-stealing algorithm with global queue.
- **Animation System**: Parametric state machine with visual editor support.

---

## Project Status & Roadmap

### Completed Features

- Asset pipeline, ECS, JobSystem, Animation FSM, Blueprint system, Tilemap, UI, Spatial Audio
- C# hot reload, C++/C# interop, Profiler, Physics Debug Visualization
- Cross-platform Editor (Windows/Linux/Android)
- Multi-backend rendering runtime (DX12/DX11/Vulkan/OpenGL/GL ES)

### Roadmap

| Priority | Feature | Status | Target |
|:--:|:--|:--|:--:|
| Medium | C-API Expansion | Planned | 2025 Q3 |
| Low | Modern Editor UI | Researching | 2025 Q4 |
| Low | Particle System | Planned | 2025 Q4 |

---

## Contribution Guide

- **Naming**: Types/functions use PascalCase; variables use camelCase.
- **Comments**: Public APIs use Doxygen-style documentation.
- **Workflow**:
  1. Create a feature branch from `master`.
  2. Build and test successfully.
  3. Submit a Pull Request with details.

---

## License

This project is released under the [MIT License](LICENSE).  
Free to use, modify, and distribute.

---

## Community & Support

Join the Luma Developer Community and help build the next-generation 2D game ecosystem.

| Platform | Join |
|:--|:--|
| **Discord** | [https://discord.gg/BJBXcRkh](https://discord.gg/BJBXcRkh) |
| **QQ Group** | 913635492 |

For business or technical inquiries:  
📧 **Email**: gug777514@gmail.com

---

**Luma Engine** — Lighting up the 2D world.
