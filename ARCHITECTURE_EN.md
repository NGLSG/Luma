Luma Engine – Architecture Deep-Dive  
*(English Version)*

This document details the core architecture and data-flow diagrams of every major system in Luma Engine.

[Return to Main Document](README_EN.md)

---

## 🏗️ High-Level Architecture

The diagram below illustrates the modular architecture and data flow of Luma Engine:

```mermaid
graph TD
    subgraph "🎮 Application Layer"
        Editor[Editor Application]
        Runtime[Runtime / Shipping Application]
    end

    subgraph "🎭 Scene & Logic Layer"
        SceneManager[Scene Manager]
        RuntimeScene[Runtime Scene]
        Systems[System Collection]
        Registry[ECS Registry]
        AnimController[Animation Controller]
        RuntimeScene --> Systems
        RuntimeScene --> Registry
        RuntimeScene --> AnimController
    end

    subgraph "⚙️ Parallel Processing Layer"
        JobSystem[JobSystem Work-Stealing]
        TaskQueue[Task Queue]
        WorkerThreads[Worker Thread Pool]
        JobSystem --> TaskQueue
        JobSystem --> WorkerThreads
    end

    subgraph "📦 Asset & Resource Layer"
        AssetManager[Asset Manager]
        Importers[Importer Collection]
        Loaders[Loader Collection]
        RuntimeCaches[Runtime Caches]
        SourceFiles[Source Files]
        RuntimeAssets[Runtime Assets]
        AssetManager --> Importers
        Importers --> SourceFiles
        Loaders --> AssetManager
        Loaders --> RuntimeAssets
        RuntimeCaches --> RuntimeAssets
    end

    subgraph "🎨 Rendering Layer"
        SceneRenderer[Scene Renderer]
        RenderSystem[Rendering System]
        GraphicsBackend[Graphics Backend]
        RenderPackets[Render Packets]
        SceneRenderer --> RenderPackets
        RenderSystem --> RenderPackets
        GraphicsBackend --> RenderSystem
    end

    Editor --> SceneManager
    Runtime --> SceneManager
    Editor --> RuntimeScene
    Runtime --> RuntimeScene
    RuntimeScene --> Systems
    Systems --> Registry
    Systems --> RuntimeCaches
    Systems --> JobSystem
    SceneManager --> Loaders
    Editor --> SceneRenderer
    Runtime --> SceneRenderer
    SceneRenderer --> Registry
```

---

## ⚙️ Core-System Data Flows

### 📦 Asset Pipeline Workflow

```mermaid
graph TD
    A[👨‍💻 Developer creates<br/>Player.png] --> B{🔍 AssetManager<br/>scans directory}
    B --> C{🔧 Match Importer}
    C --> D[📖 Read source file<br/>extract data & hash]
    D --> E[📋 Create AssetMetadata]
    E --> F[💾 Serialize to<br/>Player.png.meta]

    style A fill:#e1f5fe
    style F fill:#e8f5e8
```

### 💾 Runtime Resource Loading

```mermaid
graph TD
    A[🎯 System requests asset] --> B{💾 Check cache}
    B -->|Hit| C[✅ Return cached resource]
    B -->|Miss| D{🔧 Invoke loader}
    D --> E{📋 Fetch metadata}
    E --> F[🏗️ Create runtime instance]
    F --> G[💾 Insert into cache]
    G --> C

    style C fill:#e8f5e8
    style G fill:#fff3e0
```

### 🎭 Scene Instantiation

```mermaid
graph TD
    A[🎬 Load scene] --> B[🏗️ Create RuntimeScene]
    B --> C[📋 Parse scene data]
    C --> D{🎭 Iterate entity nodes}
    D --> E{❓ Node type}
    E -->|Prefab instance| F[📦 Load prefab]
    E -->|Plain object| G[🎮 Create game object]
    F --> H[🔧 Apply component overrides]
    G --> H
    H --> I[🌳 Recursively create children]
    I --> J[✅ Scene creation complete]

    style J fill:#e8f5e8
```

### 🔗 Scripting Interop

```mermaid
graph LR
    subgraph "C++ Engine Core"
        A[Event Manager]
        B[System Manager]
        C[C-API Interface]
        D[JobSystem]
    end

    subgraph "Interop Layer"
        E[CoreCLR Host]
        F[Function-pointer cache]
        G[P/Invoke bridge]
        H[JobSystem bindings]
    end

    subgraph "C# Script Layer"
        I[Script Components]
        J[Event Handlers]
        K[Interop classes]
        L[IJob interface]
    end

    A --> C; D --> C; C --> G; C --> H; G --> K; H --> L; K --> J; J --> I; I --> K
```

### ✨ Visual Blueprint System

```mermaid
graph TD
    subgraph "Design Phase"
        A[👨‍🎨 Edit blueprint in editor] --> B{💾 Save as .blueprint}
    end
    subgraph "Compilation Phase"
        B --> C{⚙️ C# code generator}
        C --> D[📄 Generate MyBlueprint.cs]
        D --> E{🔧 C# compiler}
    end
    subgraph "Runtime Phase"
        E --> F[📦 GameScripts.dll]
        F --> G[🚀 Engine loads at runtime]
    end
```

### ⚡ Physics Update Loop

```mermaid
graph TD
    A[⏰ Physics system update] --> B[📥 Sync kinematic bodies]
    B --> C[🔄 Read Transform components]
    C --> D[📐 Compute desired velocity]
    D --> E[⚡ Set body velocity]
    E --> F[🌍 Step physics world]
    F --> G[📤 Sync dynamic bodies]
    G --> H[📍 Read body positions]
    H --> I[🔄 Update Transform components]

    style F fill:#ffecb3
```

### 🎨 Render Pipeline

```mermaid
graph TD
    subgraph "🖼️ Editor Render Loop"
        A[🚀 Begin frame] --> B[🎭 Render scene view]
        B --> C[🔧 Set viewport parameters]
        C --> D[📦 Extract render data]
        D --> E[🎨 Generate render packets]
        E --> F[⚡ Batch optimization]
        F --> G[🖌️ Execute draw commands]
        G --> H[🖥️ Render UI]
        H --> I[📺 Present final frame]
    end

    style I fill:#e8f5e8
```

### 🧩 Tilemap System

```mermaid
graph TD
    subgraph "Edit Time"
        A[🎨 Create Tile Palette] --> B[🖌️ Add brushes]
        B --> C[👨‍🎨 Paint in scene]
        C --> D[💾 Serialize to scene file]
    end
    subgraph "Runtime"
        D --> E[⚙️ HydrateResources init]
        E --> F[🧩 Generate mesh/sprites]
        F --> G[⚡ Rendering & physics update]
    end
```

### 🔊 Audio System

```mermaid
graph TD
    subgraph "🎮 Game Logic (C# / C++)"
        A["System requests play sound"]
    end

    subgraph "🎧 Audio Manager (Main Thread)"
        B{"Play(soundRequest)"}
        C["Create Voice instance"]
        D["Add Voice to<br/>active list (thread-safe)"]
    end

    subgraph "🔊 Audio Thread Callback"
        E["Audio device requests data"]
        F{"Mix() loop"}
        G["Iterate active Voices"]
        H["Compute spatial attenuation & pan"]
        I["Mix audio samples"]
        J["Write to output buffer"]
    end

    A --> B --> C --> D
    E --> F --> G --> H --> I --> J
    D -.->|shared active list| G
```

### ⚙️ JobSystem Parallel Processing

```mermaid
graph TD
    subgraph "Main Thread"
        A[Task submission]
        B[Wait for completion]
    end

    subgraph "JobSystem Core"
        C[Global task queue]
        D[Work-stealing scheduler]
    end

    subgraph "Worker Thread Pool"
        E[Worker 1]
        F[Worker 2]
        G[Worker N]
        H[Local queue 1]
        I[Local queue 2]
        J[Local queue N]
    end

    A --> C; C --> D; D --> E; D --> F; D --> G;
    E --> H; F --> I; G --> J;
    E -.-> I; E -.-> J; F -.-> H; F -.-> J; G -.-> H; G -.-> I;
    E --> B; F --> B; G --> B
```

### 🎬 Animation State Machine

```mermaid
graph TD
    subgraph "Animation Controller"
        A[AnimationController]
        B[State-machine instance]
        C[Parameter set]
    end

    subgraph "State Nodes"
        D[Idle state]
        E[Running state]
        F[Jump state]
    end

    subgraph "Transition Conditions"
        G[Bool: running]
        H[Trigger: jump]
        I[Float: speed]
    end

    A --> B; A --> C; B --> D; B --> E; B --> F;
    D -->|running = true| E; E -->|running = false| D;
    D -->|jump trigger| F; F -->|completed| D;
    C --> G; C --> H; C --> I
```

---

[⬆️ Return to Main Document](README_EN.md)