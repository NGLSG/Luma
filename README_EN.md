# Luma Engine

A modern, modular, data‑driven, high‑performance real‑time 2D game engine built with C++20 and C#, designed as a capable alternative to Unity 2D.

- [中文文档](README.md)
- [Architecture](ARCHITECTURE_EN.md)

---

## Contents

- [Overview and Design Principles](#overview-and-design-principles)
- [Key Capabilities and Performance](#key-capabilities-and-performance)
  - [Performance Comparison: Luma vs. Unity DOTS](#performance-comparison-luma-vs-unity-dots)
  - [Feature Summary](#feature-summary)
- [Technology Stack](#technology-stack)
- [Getting Started](#getting-started)
  - [Environment and Dependencies](#environment-and-dependencies)
  - [Build](#build)
- [Core Systems](#core-systems)
- [Project Status and Roadmap](#project-status-and-roadmap)
- [Contributing](#contributing)
- [License](#license)

---

## Overview and Design Principles

Luma targets 2D projects that require top‑tier performance and a modern workflow.

- **Data‑driven**: scenes, entities, components, and animation are all data‑centric, enabling hot‑reload, editor extensibility, and procedural generation.
- **Modular and extensible**: rendering, physics, audio, and other systems are decoupled for independent evolution.
- **Performance‑first**: an ECS architecture and parallel JobSystem designed for large‑scale, real‑time workloads.

---

## Key Capabilities and Performance

### Performance Comparison: Luma vs. Unity DOTS

> Unity version: 6.1 LTS; identical hardware and scenario settings.

**Scene rendering (dynamic sprites)**

| Entities | Luma (FPS) | Unity DOTS (FPS) | Multiplier |
|:--:|:--:|:--:|:--:|
| 100,000 | ~100 | ~30 | ~3.3× |
| 200,000 | ~50 | ~15 | ~3.3× |
| 1,000,000 | ~10 | ~2 | ~5.0× |

**Physics (Box2D, 10,000 dynamic bodies)**

| Metric | Luma | Unity | Multiplier |
|:--:|:--:|:--:|:--:|
| Total frame time | 2.40 ms | 45.45 ms | 18.9× |
| Theoretical FPS | ~416 | ~22 | 18.9× |

> Note: Values come from internal tests under equivalent conditions. Real‑world results vary with hardware and content.

### Feature Summary

**Modern ECS**  
- Built on EnTT, optimized for data locality and fast iteration.  
- Clean separation of logic and data for maintainability.

**High‑performance parallelism**  
- Work‑stealing JobSystem with dynamic load balancing.  
- C# JobSystem bindings allow parallel gameplay code.

**Visual Blueprint**  
- Node editor that emits compiled C# code (no runtime interpretation).  
- Supports events, branches, loops, custom functions/variables.

**C++/C# Interop**  
- .NET 9 CoreCLR host for reliable bidirectional calls.  
- Script hot‑reload supported.

**Physics, Tilemap, UI, Audio**  
- Box2D with fixed timestep; parallelized via JobSystem.  
- Tilemap with standard/rule tiles and prefab brushes.  
- ECS‑driven UI components (Text/Image/Button/InputText).  
- SDL3 audio with spatialization and multi‑channel mixing.

---

## Technology Stack

| Area | Technology | Version/Lib |
|:--|:--|:--|
| Language | C++ | C++20 |
| Scripting | C# | .NET 9 (CoreCLR) |
| Build | CMake | 3.21+ |
| ECS | EnTT | latest |
| Physics | Box2D | latest |
| Rendering | Skia + Dawn | cross‑platform wrapper |
| Window/Input | SDL3 | cross‑platform |
| Editor UI | Dear ImGui | immediate‑mode GUI |
| Serialization | yaml-cpp / json | YAML/JSON |

---

## Getting Started

### Environment and Dependencies

1. Install Git, CMake (≥ 3.21), Vulkan SDK, and a modern C++ toolchain (VS 2022 / GCC 11 / Clang 14).  
2. Dependencies: **Vcpkg** is recommended; CPM.cmake supplements remaining libraries; system package managers are also fine for common libs.  
3. Special dependencies: download prebuilt **Skia** and **CoreCLR** packages and extract them to `External/` in the repo root. Example layout:
   ```
   Luma/
   ├── External/
   │   ├── coreclr-win-x64/    # or coreclr-linux-x64/
   │   └── skia-win/           # or skia-linux/
   └── ...
   ```

### Build

```bash
git clone https://github.com/NGLSG/Luma.git
cd Luma

# Using Vcpkg (adjust path as needed)
mkdir build && cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=E:/vcpkg/scripts/buildsystems/vcpkg.cmake
cmake --build . --config Release
```

Artifacts are located under `build/bin/Release/` (or the configured output directory).

---

## Core Systems

- **Assets and resources**: GUID‑based pipeline with `.meta` files; `AssetManager` and caches provide efficient runtime loading/sharing.  
- **Scripting**: CoreCLR hosting with C‑API/PInvoke bridge; C# events, interfaces, and Job bindings.  
- **Rendering**: simulation thread submits frame data; render thread interpolates between full frame states; batching and API dispatch in the backend.  
- **Physics**: fixed timestep; kinematic/dynamic synchronization with consistent Transform write‑back.  
- **Audio**: voices managed on the main thread; mixing and spatialization in the audio callback.  
- **Parallel scheduling**: global queue plus per‑thread deques with work‑stealing; completion waits supported.  
- **Animation and state machines**: parameterized states, transitions, and triggers; visual configuration and C# control.

See [ARCHITECTURE_EN.md](ARCHITECTURE_EN.md) for diagrams and flows.

---

## Project Status and Roadmap

### Completed
- Asset pipeline, ECS, JobSystem, animation state machine, visual blueprint, Tilemap, UI, spatial audio  
- C# hosting (hot‑reload), C++/C# interop, physics integration  
- Editor, packaging, profiler, physics visualization

### Roadmap

| Priority | Item | Status | Target |
|:--:|:--|:--|:--:|
| Medium | C‑API expansion | Planned | 2025 Q3 |
| Low | Editor UI modernization | Research | 2025 Q4 |
| Low | Particle system | Planned | 2025 Q4 |

---

## Contributing

- Naming: PascalCase for types/functions; camelCase for variables.  
- Comments: public APIs use Doxygen‑style documentation.  
- Process:
  1. Fork and branch from `master`.  
  2. Ensure style and builds pass.  
  3. Open a PR with motivation and implementation details.

---

## License

Licensed under the [MIT](LICENSE).
