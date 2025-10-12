# Luma 引擎架构设计

- [返回主文档](README.md)
- [English Version](ARCHITECTURE_EN.md)

---

## 整体架构

下图示意引擎的模块化分层与数据流。

```mermaid
graph TD
    subgraph "应用层"
        Editor[编辑器应用]
        Runtime[运行时/打包应用]
    end

    subgraph "场景与逻辑层"
        SceneManager[场景管理器]
        RuntimeScene[运行时场景]
        Systems[系统集合]
        Registry[ECS 注册表]
        AnimController[动画控制器]
        RuntimeScene --> Systems
        RuntimeScene --> Registry
        RuntimeScene --> AnimController
    end

    subgraph "并行处理层"
        JobSystem[JobSystem 工作窃取]
        TaskQueue[任务队列]
        WorkerThreads[工作线程池]
        JobSystem --> TaskQueue
        JobSystem --> WorkerThreads
    end

    subgraph "资产与资源层"
        AssetManager[资产管理器]
        Importers[导入器集合]
        Loaders[加载器集合]
        RuntimeCaches[运行时缓存]
        SourceFiles[源文件]
        RuntimeAssets[运行时资源]
        AssetManager --> Importers
        Importers --> SourceFiles
        Loaders --> AssetManager
        Loaders --> RuntimeAssets
        RuntimeCaches --> RuntimeAssets
    end

    subgraph "渲染层"
        SceneRenderer[场景渲染器]
        RenderSystem[渲染系统]
        GraphicsBackend[图形后端]
        RenderPackets[渲染包]
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

## 核心系统数据流

### 资产管线

```mermaid
graph TD
    A[开发者创建 Player.png] --> B{AssetManager 扫描目录}
    B --> C{匹配 Importer}
    C --> D[读取源文件/生成哈希]
    D --> E[创建 AssetMetadata]
    E --> F[序列化为 Player.png.meta]
```

### 运行时资源加载

```mermaid
graph TD
    A[系统请求资源] --> B{缓存命中?}
    B -->|是| C[返回缓存资源]
    B -->|否| D{调用加载器}
    D --> E{读取元数据}
    E --> F[创建运行时实例]
    F --> G[写入缓存]
    G --> C
```

### 场景实例化

```mermaid
graph TD
    A[加载场景] --> B[创建 RuntimeScene]
    B --> C[解析场景数据]
    C --> D{遍历实体节点}
    D --> E{节点类型}
    E -->|预制体| F[加载预制体]
    E -->|普通对象| G[创建游戏对象]
    F --> H[应用组件覆盖]
    G --> H
    H --> I[递归创建子节点]
    I --> J[完成]
```

### 脚本系统互操作

```mermaid
graph LR
    subgraph "C++ 核心"
        A[事件管理器]
        B[系统管理器]
        C[C-API 接口]
        D[JobSystem]
    end

    subgraph "互操作层"
        E[CoreCLR 宿主]
        F[函数指针缓存]
        G[P/Invoke 桥接]
        H[JobSystem 绑定]
    end

    subgraph "C# 层"
        I[脚本组件]
        J[事件处理器]
        K[Interop 类]
        L[IJob 接口]
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

### 物理更新循环

```mermaid
graph TD
    A[物理系统更新] --> B[同步运动学刚体]
    B --> C[读取 Transform]
    C --> D[计算速度]
    D --> E[设置刚体速度]
    E --> F[物理步进]
    F --> G[同步动态刚体]
    G --> H[读取刚体位置]
    H --> I[更新 Transform]
```

### 渲染管线

```mermaid
graph TD
subgraph "模拟线程"
Sim_A[Systems Update] --> Sim_B[更新 ECS 数据]
Sim_B --> Sim_C["SceneRenderer: 提取 Renderable"]
Sim_C --> Sim_D["RenderableManager.SubmitFrame"]
end

subgraph "同步与插值"
SyncPoint[RenderableManager: 维护 Sₙ₋₁, Sₙ]
end

subgraph "渲染线程"
Render_A["GetInterpolationData -> 插值 Sₙ₋₁/Sₙ"]
Render_A --> Render_B["SceneRenderer: 生成 RenderPackets"]
Render_B --> Render_C["RenderSystem: 批处理"]
Render_C --> Render_D["GraphicsBackend: Draw Calls"]
Render_D --> Render_E[GPU 渲染]
end

Sim_D -- "线程安全写入" --> SyncPoint
SyncPoint -- "线程安全读取" --> Render_A
```

### Tilemap 系统

```mermaid
graph TD
    subgraph "编辑"
        A[创建 Tile Palette] --> B[添加笔刷]
        B --> C[场景绘制]
        C --> D[序列化至场景文件]
    end
    subgraph "运行时"
        D --> E[HydrateResources 初始化]
        E --> F[生成 Mesh/Sprites]
        F --> G[渲染与物理更新]
    end
```

### 音频系统

```mermaid
graph TD
    subgraph "游戏逻辑"
        A[请求播放声音]
    end

    subgraph "音频管理器（主线程）"
        B["Play(soundRequest)"]
        C["创建 Voice"]
        D["加入活动列表（线程安全）"]
    end

    subgraph "音频线程"
        E["设备回调请求数据"]
        F["Mix 循环"]
        G["遍历活动 Voice"]
        H["空间衰减与声相"]
        I["混合样本"]
        J["写入输出缓冲"]
    end

    A --> B --> C --> D
    E --> F --> G --> H --> I --> J
    D -.-> G
```

### JobSystem 并行处理

```mermaid
graph TD
    subgraph "主线程"
        A[提交任务]
        B[等待完成]
    end

    subgraph "JobSystem"
        C[全局任务队列]
        D[工作窃取调度]
    end

    subgraph "线程池"
        E[工作线程1]
        F[工作线程2]
        G[工作线程N]
        H[本地队列1]
        I[本地队列2]
        J[本地队列N]
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

### 动画状态机

```mermaid
graph TD
    subgraph "动画控制器"
        A[AnimationController]
        B[状态机实例]
        C[参数集合]
    end

    subgraph "状态"
        D[Idle]
        E[Running]
        F[Jump]
    end

    subgraph "过渡条件"
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
    D -->|jump 触发| F
    F -->|完成| D
    C --> G
    C --> H
    C --> I
```

---

## 实施建议

- **线程安全**：渲染数据提交与读取使用双缓冲或帧队列，避免竞争。  
- **确定性**：固定物理步长与时间同步策略，确保回放与网络一致性。  
- **可测性**：关键系统（资产、渲染、物理、JobSystem）提供可视化 Profiling 与统计接口。  
- **演进性**：模块接口稳定，允许后端替换（例如图形后端从 Skia/Dawn 迁移至其他实现）。

---

## Android 支持（arm64）

- External 预编译包：
  - `External/mono-android-arm64`（.NET 9 Mono，含 `DotNetRuntimeConfig.cmake`）
  - `External/skia-android`（Skia 静态库，仅 arm64）
- CMake：
  - Android 自动切换到 `DotNet::Mono`；桌面平台使用 `DotNet::Host`。
  - Android 不强制查找 Vulkan/OpenSSL，优先 GLES/EGL（Skia 链接静态库）。
- 代码：
  - `Scripting/ManagedHost.h` 在 Android 采用 `MonoHost`（与 `CoreCLRHost` 统一 API）。
  - `Luma_CAPI` 暴露 `Engine_InitWithANativeWindow/Engine_Frame/Engine_Shutdown` 供 Java 层调用。
- Android 工程：
  - `Android/app/src/main/cpp/CMakeLists.txt` 引入引擎根目录并链接 `LumaEngine`。
  - Gradle 任务将 Mono 运行时与 Skia 运行时库复制到 `src/main/jniLibs/arm64-v8a`：
    - `External/mono-android-arm64/bin/*.so`
    - `External/skia-android/bin/*.so`（如 Dawn/webgpu 运行时，若使用）
    - 同时打包 `External/skia-android/bin/icudtl.dat` 至 APK 资源（文本排版所需）
  - 默认启用脚本（Mono），不再定义 `LUMA_DISABLE_SCRIPTING`。
