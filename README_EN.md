<div align="center" style="margin-top: 12px; margin-bottom: 24px;">

  <h1 style="margin: 0; font-weight: 800;">Luma Engine</h1>

  <p style="margin: 8px 0 16px; line-height: 1.6;">
    <strong>Modern · Modular · Data-Driven · High-Performance 2D Game Engine</strong><br/>
    Built with <strong>C++20</strong> and <strong>C# (.NET 9 CoreCLR)</strong>, designed as a powerful alternative to Unity 2D.
  </p>

  <!-- Badges Row -->
  <p style="display: inline-flex; gap: 6px; flex-wrap: wrap; justify-content: center;">
    <!-- Stars -->
    <a href="https://github.com/NGLSG/Luma/stargazers" target="_blank">
      <img alt="GitHub Stars"
           src="https://img.shields.io/github/stars/NGLSG/Luma?style=for-the-badge&logo=github" />
    </a>
    <!-- Forks -->
    <a href="https://github.com/NGLSG/Luma/fork" target="_blank">
      <img alt="GitHub Forks"
           src="https://img.shields.io/github/forks/NGLSG/Luma?style=for-the-badge&logo=github" />
    </a>
    <!-- License -->
    <a href="LICENSE" target="_blank">
      <img alt="License"
           src="https://img.shields.io/github/license/NGLSG/Luma?style=for-the-badge" />
    </a>
    <!-- Top language (dynamic) -->
    <img alt="Top Language"
         src="https://img.shields.io/github/languages/top/NGLSG/Luma?style=for-the-badge&logo=cplusplus&logoColor=white" />
  </p>

</div>

---

- [中文版本](README.md)
- [Engine Architecture](ARCHITECTURE.md)

---

## Table of Contents

- [Overview & Design Philosophy](#overview--design-philosophy)
- [Core Features & Performance](#core-features--performance)
  - [Performance Comparison: Luma vs Unity DOTS](#performance-comparison-luma-vs-unity-dots)
  - [Feature Highlights](#feature-highlights)
- [Tech Stack](#tech-stack)
- [Getting Started](#getting-started)
  - [Environment & Dependencies](#environment--dependencies)
  - [Build Instructions](#build-instructions)
- [Editor Debugging Support](#editor-debugging-support)
- [Core Systems Overview](#core-systems-overview)
- [Project Status & Roadmap](#project-status--roadmap)
- [Contribution Guidelines](#contribution-guidelines)
- [License](#license)
- [Community & Support](#community--support)

---

## Overview & Design Philosophy

**Luma Engine** targets developers seeking ultimate performance and a modern workflow for 2D game development.  
Core design principles include:

- **Data-Driven Architecture** – All scenes, entities, components, and animations are data-centric, enabling live reloading and procedural generation.
- **Modular & Extensible** – Rendering, physics, audio, and UI systems are fully decoupled and replaceable.
- **Performance First** – ECS architecture and multithreaded JobSystem for large-scale real-time computation.

---

## Core Features & Performance

### Performance Comparison: Luma vs Unity DOTS

> Unity Version: 6.1 LTS — identical hardware, identical scene setup.

| Entity Count | Luma (FPS) | Unity DOTS (FPS) | Multiplier |
|:--:|:--:|:--:|:--:|
| 100,000 | ~130 | ~30 | ~4.3× |
| 200,000 | ~60 | ~15 | ~4.0× |
| 1,000,000 | ~15 | ~2 | ~7.0× |

**Physics Simulation (Box2D, 10,000 dynamic bodies)**

| Metric | Luma | Unity | Multiplier |
|:--:|:--:|:--:|:--:|
| Total Frame Time | 2.40 ms | 45.45 ms | 18.9× |
| Theoretical FPS | ~416 | ~22 | 18.9× |

> Note: Benchmark results are for reference only. Actual performance varies with hardware and scene complexity.

### Feature Highlights

- **Modern ECS Architecture** (powered by EnTT)
- **JobSystem Parallelism** (C++/C# interoperability)
- **Visual Blueprint System** (node-based → compiled C#)
- **C++ / C# Interop & Hot Reloading**
- **Physics / Tilemap / UI / Audio Systems**
- **High-Performance Rendering Pipeline (Skia + Dawn)**

---

## Tech Stack

| Category | Technology | Description |
|:--|:--|:--|
| Core Languages | C++20 / C# (.NET 9) | High-performance native and managed code |
| Build System | CMake 3.21+ | Cross-platform build |
| ECS Framework | EnTT | Data-oriented entity management |
| Physics Engine | Box2D | Real-time 2D dynamics |
| Renderer | Skia + Dawn | GPU-accelerated, cross-platform |
| Audio | SDL3 Audio | Multichannel spatial mixing |
| Editor UI | Dear ImGui | Immediate-mode editor interface |
| Serialization | yaml-cpp / json | YAML & JSON read/write |

---

## Getting Started

### Environment & Dependencies

1. Install: Git, CMake (≥ 3.21), Vulkan SDK, and a modern C++ compiler (VS 2022 / Clang 14 / GCC 11+).
2. Use **Vcpkg** for dependency management.
3. Extract **Skia** and **CoreCLR** into the `External/` directory:

```
Luma/
├── External/
│   ├── coreclr-win-x64/
│   └── skia-win/
```

### Build Instructions

```bash
git clone https://github.com/NGLSG/Luma.git
cd Luma
mkdir build && cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=E:/vcpkg/scripts/buildsystems/vcpkg.cmake
cmake --build . --config Release
```

Build output will be located in `build/bin/Release/`.

---

## Editor Debugging Support

Luma Engine supports **Rider** and **Visual Studio 2022 / 2026 Insider** for in-editor breakpoint debugging.

### Plugin Installation

```
IDEPlugins/
├── Rider_Debug_Plugin.jar
└── VS_Debug_Plugin.vsix
```

- **Rider**: *Settings → Plugins → Install Plugin from Disk* → select `.jar`.
- **Visual Studio**: Double-click `.vsix` to install.

### Usage

**Visual Studio**

1. Open your game script project.
2. Set breakpoints.
3. Menu: **Tools → Attach To Luma Process**.
4. Click “Play” in Luma Editor to enter debug mode.

**Rider**

1. Open the project and set breakpoints.
2. Choose “Attach to Luma Process”.
3. Click **Debug**, select `LumaEditor`.
4. Click “Play” to enter debugging mode.

> Supports C# hot reload, exception handling, and variable inspection.

---

## Core Systems Overview

- **Asset System** – GUID-based import & caching.
- **Scripting System** – CoreCLR hosting + PInvoke bridge.
- **Rendering System** – Thread-safe submission, batching, interpolation.
- **Physics System** – Fixed timestep with transform sync.
- **Audio System** – Multithreaded mixing and spatialization.
- **Job Scheduling** – Global task queue with work-stealing.
- **Animation System** – Parameterized state machine and visual editor integration.

---

## Project Status & Roadmap

### Completed Features

- Asset pipeline, ECS, JobSystem, animation state machine, visual blueprints, Tilemap, UI, spatial audio
- C# hot reloading, C++/C# interop, profiler, physics debug visualization

### Roadmap

| Priority | Feature | Status | Target |
|:--:|:--|:--|:--:|
| Medium | C-API Expansion | Planned | 2025 Q3 |
| Low | Editor UI Modernization | Research | 2025 Q4 |
| Low | Particle System | Planned | 2025 Q4 |

---

## Contribution Guidelines

- **Naming**: PascalCase for types/functions, camelCase for variables.
- **Comments**: Use Doxygen-style documentation for public APIs.
- **Workflow**:
  1. Create a feature branch from `master`.
  2. Ensure builds and tests pass.
  3. Submit a Pull Request with a detailed description.

---

## License

This project is licensed under the [MIT License](LICENSE).  
Free to use, modify, and distribute.

---

## Community & Support

Join the Luma developer community and help shape the next generation of 2D engines.

| Platform | Join |
|:--|:--|
| <img src="https://cdn.jsdelivr.net/gh/simple-icons/simple-icons/icons/discord.svg" width="20"/> **Discord** | [https://discord.gg/BJBXcRkh](https://discord.gg/BJBXcRkh) |
| <img src="https://cdn.jsdelivr.net/gh/simple-icons/simple-icons/icons/tencentqq.svg" width="20"/> **QQ Group** | 913635492 |

For business or technical inquiries:  
📧 **Email**: gug777514@gmail.com

---

**Luma Engine** — Lighting up the next generation of 2D game worlds.
