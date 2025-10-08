# Luma Architecture

- [Back to README](README_EN.md)
- [中文文档](ARCHITECTURE.md)

---

## High‑Level Architecture

The following diagram outlines the modular layers and primary data flows.

```mermaid
graph TD
    subgraph "Application"
        Editor[Editor]
        Runtime[Runtime/Packaged App]
    end

    subgraph "Scene & Logic"
        SceneManager[Scene Manager]
        RuntimeScene[Runtime Scene]
        Systems[Systems]
        Registry[ECS Registry]
        AnimController[Animation Controller]
        RuntimeScene --> Systems
        RuntimeScene --> Registry
        RuntimeScene --> AnimController
    end

    subgraph "Parallel Processing"
        JobSystem[JobSystem (work‑stealing)]
        TaskQueue[Task Queue]
        WorkerThreads[Worker Thread Pool]
        JobSystem --> TaskQueue
        JobSystem --> WorkerThreads
    end

    subgraph "Assets & Resources"
        AssetManager[Asset Manager]
        Importers[Importers]
        Loaders[Loaders]
        RuntimeCaches[Caches]
        SourceFiles[Source Files]
        RuntimeAssets[Runtime Assets]
        AssetManager --> Importers
        Importers --> SourceFiles
        Loaders --> AssetManager
        Loaders --> RuntimeAssets
        RuntimeCaches --> RuntimeAssets
    end

    subgraph "Rendering"
        SceneRenderer[Scene Renderer]
        RenderSystem[Render System]
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

## Core Data Flows

### Asset Pipeline

```mermaid
graph TD
    A[Create Player.png] --> B{AssetManager scans}
    B --> C{Match Importer}
    C --> D[Read source / hash]
    D --> E[Create AssetMetadata]
    E --> F[Write Player.png.meta]
```

### Runtime Resource Loading

```mermaid
graph TD
    A[System requests resource] --> B{Cache hit?}
    B -->|Yes| C[Return cached]
    B -->|No| D{Invoke Loader}
    D --> E{Read metadata}
    E --> F[Create runtime instance]
    F --> G[Store in cache]
    G --> C
```

### Scene Instantiation

```mermaid
graph TD
    A[Load scene] --> B[Create RuntimeScene]
    B --> C[Parse scene data]
    C --> D[Iterate nodes]
    D --> E{Node type}
    E -->|Prefab| F[Load prefab]
    E -->|Object| G[Create game object]
    F --> H[Apply component overrides]
    G --> H
    H --> I[Recurse into children]
    I --> J[Done]
```

### Scripting Interop

```mermaid
graph LR
    subgraph "C++ Core"
        A[Event Manager]
        B[System Manager]
        C[C‑API]
        D[JobSystem]
    end

    subgraph "Interop Layer"
        E[CoreCLR Host]
        F[Function Pointer Cache]
        G[P/Invoke Bridge]
        H[JobSystem Bindings]
    end

    subgraph "C# Layer"
        I[Script Components]
        J[Event Handlers]
        K[Interop Classes]
        L[IJob Interface]
    end

    A --> C
    D --> C
    C --> G
    C --> H
    G --> K
    H --> L
    K --> J
    J --> I
    I --> K
```

### Physics Update Loop

```mermaid
graph TD
    A[Physics update] --> B[Sync kinematic bodies]
    B --> C[Read Transform]
    C --> D[Compute velocity]
    D --> E[Set body velocity]
    E --> F[Step simulation]
    F --> G[Sync dynamic bodies]
    G --> H[Read body pose]
    H --> I[Write back Transform]
```

### Render Pipeline

```mermaid
graph TD
subgraph "Simulation Thread"
Sim_A[Systems Update] --> Sim_B[Update ECS data]
Sim_B --> Sim_C["SceneRenderer: extract Renderables"]
Sim_C --> Sim_D["RenderableManager.SubmitFrame"]
end

subgraph "Sync & Interpolation"
SyncPoint[RenderableManager keeps Sₙ₋₁, Sₙ]
end

subgraph "Render Thread"
Render_A["GetInterpolationData -> interpolate Sₙ₋₁/Sₙ"]
Render_A --> Render_B["SceneRenderer: build RenderPackets"]
Render_B --> Render_C["RenderSystem: batching"]
Render_C --> Render_D["GraphicsBackend: draw calls"]
Render_D --> Render_E[GPU]
end

Sim_D -- "thread‑safe write" --> SyncPoint
SyncPoint -- "thread‑safe read" --> Render_A
```

### Tilemap System

```mermaid
graph TD
    subgraph "Authoring"
        A[Create Tile Palette] --> B[Add brushes]
        B --> C[Paint in scene]
        C --> D[Serialize into scene]
    end
    subgraph "Runtime"
        D --> E[HydrateResources]
        E --> F[Generate Mesh/Sprites]
        F --> G[Render and physics updates]
    end
```

### Audio System

```mermaid
graph TD
    subgraph "Gameplay"
        A[Request to play sound]
    end

    subgraph "Audio Manager (Main Thread)"
        B["Play(soundRequest)"]
        C["Create Voice"]
        D["Add to active list (thread‑safe)"]
    end

    subgraph "Audio Thread"
        E["Device callback requests data"]
        F["Mix loop"]
        G["Iterate active voices"]
        H["Apply attenuation / panning"]
        I["Mix samples"]
        J["Write output buffer"]
    end

    A --> B --> C --> D
    E --> F --> G --> H --> I --> J
    D -.-> G
```

### JobSystem Parallel Processing

```mermaid
graph TD
    subgraph "Main Thread"
        A[Submit tasks]
        B[Wait for completion]
    end

    subgraph "JobSystem"
        C[Global queue]
        D[Work‑stealing scheduler]
    end

    subgraph "Thread Pool"
        E[Worker 1]
        F[Worker 2]
        G[Worker N]
        H[Local deque 1]
        I[Local deque 2]
        J[Local deque N]
    end

    A --> C
    C --> D
    D --> E
    D --> F
    D --> G
    E --> H
    F --> I
    G --> J
    E -.-> I
    E -.-> J
    F -.-> H
    F -.-> J
    G -.-> H
    G -.-> I
    E --> B
    F --> B
    G --> B
```

### Animation State Machine

```mermaid
graph TD
    subgraph "Controller"
        A[AnimationController]
        B[State Machine]
        C[Parameters]
    end

    subgraph "States"
        D[Idle]
        E[Running]
        F[Jump]
    end

    subgraph "Transitions"
        G[Bool: running]
        H[Trigger: jump]
        I[Float: speed]
    end

    A --> B
    A --> C
    B --> D
    B --> E
    B --> F
    D -->|running=true| E
    E -->|running=false| D
    D -->|jump| F
    F -->|done| D
    C --> G
    C --> H
    C --> I
```

---

## Implementation Notes

- **Thread safety**: double‑buffering or frame queues for submission/reads to avoid contention.  
- **Determinism**: fixed physics timestep and consistent time sync for replay/network parity.  
- **Observability**: profiling and counters across assets, rendering, physics, and JobSystem.  
- **Evolvability**: stable module interfaces to allow backend swaps (e.g., alternative graphics backends).
