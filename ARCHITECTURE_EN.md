# Luma Engine - Architecture Design Details

[Return to Main Document](README_EN.md)

---

## 🏗️ Overall Architecture

The diagram below shows the modular architecture and data flow of the Luma Engine:

```mermaid
graph TD
    subgraph "🎮 Application Layer"
        Editor[Editor Application]
        Runtime[Runtime/Packaged Application]
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
        JobSystem[JobSystem Work Stealing]
        TaskQueue[Task Queue]
        WorkerThreads[Worker Thread Pool]
        JobSystem --> TaskQueue
        JobSystem --> WorkerThreads
    end

    subgraph "📦 Asset & Resource Layer"
        AssetManager[Asset Manager]
        Importers[Importer Collection]
        Loaders[Loader Collection]
        RuntimeCaches[Runtime Cache]
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

-----

## ⚙️ Core System Data Flow

### 📦 Asset Pipeline Workflow

```mermaid
graph TD
    A[👨‍💻 Developer creates<br/>Player.png] --> B{🔍 AssetManager<br/>scans directory}
B --> C{🔧 Match Importer}
C --> D[📖 Read source file<br/>Extract data and hash]
D --> E[📋 Create AssetMetadata]
E --> F[💾 Serialize to<br/>Player.png.meta]

style A fill:#e1f5fe
style F fill: #e8f5e8
```

### 💾 Runtime Resource Loading

```mermaid
graph TD
    A[🎯 System requests resource] --> B{💾 Check cache}
B -->|Hit| C[✅ Return cached resource]
B -->|Miss| D{🔧 Call loader}
D --> E{📋 Get metadata}
E --> F[🏗️ Create runtime instance]
F --> G[💾 Store in cache]
G --> C

style C fill: #e8f5e8
style G fill: #fff3e0
```

### 🎭 Scene Instantiation

```mermaid
graph TD
    A[🎬 Load scene] --> B[🏗️ Create RuntimeScene]
B --> C[📋 Parse scene data]
C --> D{🎭 Traverse entity nodes}
D --> E{❓ Node type}
E -->|Prefab instance| F[📦 Load prefab]
E -->|Regular object| G[🎮 Create game object]
F --> H[🔧 Apply component overrides]
G --> H
H --> I[🌳 Recursively create child nodes]
I --> J[✅ Complete scene creation]

style J fill:#e8f5e8
```

### 🔗 Scripting System Interop

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
        F[Function Pointer Cache]
        G[P/Invoke Bridge]
        H[JobSystem Binding]
    end

    subgraph "C# Scripting Layer"
        I[Script Component]
        J[Event Handler]
        K[Interop Class]
        L[IJob Interface]
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
        B --> C{⚙️ C# Code Generator}
        C --> D[📄 Generate MyBlueprint.cs]
        D --> E{🔧 C# Compiler}
    end
    subgraph "Runtime Phase"
        E --> F[📦 GameScripts.dll]
        F --> G[🚀 Engine loads at runtime]
    end
```

### ⚡ Physics Update Loop

```mermaid
graph TD
    A[⏰ Physics system update] --> B[📥 Sync kinematic rigidbodies]
B --> C[🔄 Read Transform component]
C --> D[📐 Calculate required velocity]
D --> E[⚡ Set rigidbody velocity]
E --> F[🌍 Execute physics step]
F --> G[📤 Sync dynamic rigidbodies]
G --> H[📍 Read rigidbody position]
H --> I[🔄 Update Transform component]

style F fill: #ffecb3
```

### 🎨 Render Pipeline

```mermaid
graph TD
subgraph "⚙️ Simulation Thread"
Sim_A[Physics/AI/Script/Animation<br/>Systems Update] --> Sim_B[Update ECS data];
Sim_B --> Sim_C["SceneRenderer:<br/>Traverse ECS, extract Renderable data"];
Sim_C --> Sim_D[RenderableManager.SubmitFrame<br/>Atomically submit a full frame of Renderable data<br/>to the back buffer];
end

subgraph "🔗 Thread Synchronization & Data Interpolation"
SyncPoint[RenderableManager<br/>Holds complete state of last two frames Sₙ₋₁, Sₙ];
end

subgraph "🎨 Render Thread"
Render_A["GetInterpolationData<br/>Calculate Alpha based on current time<br/>Interpolate Sₙ₋₁ and Sₙ, generate final transforms"];
Render_A --> Render_B["SceneRenderer:<br/>Pack interpolated data into RenderPackets"];
Render_B --> Render_C["RenderSystem:<br/>Batching Packets"];
Render_C --> Render_D["GraphicsBackend:<br/>Convert batches into graphics API calls Draw Calls"];
Render_D --> Render_E[GPU Rendering];
end

Sim_D -- "Thread-safe write" --> SyncPoint;
SyncPoint -- "Thread-safe read & copy" --> Render_A;
```

### 🧩 Tilemap System

```mermaid
graph TD
    subgraph "Editing Time"
        A[🎨 Create Tile Palette] --> B[🖌️ Add brushes]
        B --> C[👨‍🎨 Paint in scene]
        C --> D[💾 Serialize to scene file]
    end
    subgraph "Runtime"
        D --> E[⚙️ HydrateResources initialization]
        E --> F[🧩 Generate Mesh/Sprites]
        F --> G[⚡️ Rendering & physics updates]
    end
```

### 🔊 Audio System

```mermaid
graph TD
    subgraph "🎮 Game Logic C# / C++"
        A["System requests sound playback"]
    end

    subgraph "🎧 Audio Manager Main Thread"
        B{"Play(soundRequest)"}
        C["Create Voice instance"]
        D["Add Voice to<br/>active list (thread-safe)"]
    end

    subgraph "🔊 Audio Thread Callback"
        E["Audio device requests data"]
        F{"Mix() loop"}
        G["Iterate all active Voices"]
        H["Calculate spatial attenuation and panning"]
        I["Mix audio samples"]
        J["Write output buffer"]
    end

%% Define main thread logic
    A --> B --> C --> D

%% Define audio thread logic
    E --> F --> G --> H --> I --> J

%% Define cross-thread interaction
    D -.->|Shared active list| G
```

### ⚙️ JobSystem Parallel Processing

```mermaid
graph TD
    subgraph "Main Thread"
        A[Task submission]
        B[Task completion wait]
    end

    subgraph "JobSystem Core"
        C[Global task queue]
        D[Work-stealing scheduler]
    end

    subgraph "Worker Thread Pool"
        E[Worker Thread 1]
        F[Worker Thread 2]
        G[Worker Thread N]
        H[Local task queue 1]
        I[Local task queue 2]
        J[Local task queue N]
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
        B[State machine instance]
        C[Parameter collection]
    end

    subgraph "State Nodes"
        D[Idle state]
        E[Running state]
        F[Jump state]
    end

    subgraph "Transition Conditions"
        G[Bool parameter: running]
        H[Trigger parameter: jump]
        I[Float parameter: speed]
    end

    A --> B; A --> C; B --> D; B --> E; B --> F;
    D -->|running = true| E; E -->|running = false| D;
    D -->|jump trigger| F; F -->|Complete| D;
    C --> G; C --> H; C --> I
```

-----

[⬆️ Return to Main Document](README_EN.md)