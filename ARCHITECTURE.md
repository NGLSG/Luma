# Luma 引擎 - 架构设计详解

本文档详细阐述了 Luma 引擎的核心架构和主要系统的数据流图。

[返回主文档 (Return to Main Document)](README.md)

---

## 🏗️ 整体架构

下图展示了 Luma 引擎的模块化架构和数据流：

```mermaid
graph TD
    subgraph "🎮 应用层"
        Editor[编辑器应用]
        Runtime[运行时/打包应用]
    end

    subgraph "🎭 场景与逻辑层"
        SceneManager[场景管理器]
        RuntimeScene[运行时场景]
        Systems[系统集合]
        Registry[ECS 注册表]
        AnimController[动画控制器]
        RuntimeScene --> Systems
        RuntimeScene --> Registry
        RuntimeScene --> AnimController
    end

    subgraph "⚙️ 并行处理层"
        JobSystem[JobSystem 工作窃取]
        TaskQueue[任务队列]
        WorkerThreads[工作线程池]
        JobSystem --> TaskQueue
        JobSystem --> WorkerThreads
    end

    subgraph "📦 资产与资源层"
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

    subgraph "🎨 渲染层"
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
````

-----

## ⚙️ 核心系统数据流

### 📦 资产管线工作流 (Asset Pipeline Workflow)

```mermaid
graph TD
    A[👨‍💻 开发者创建<br/>Player.png] --> B{🔍 AssetManager<br/>扫描目录}
B --> C{🔧 匹配 Importer}
C --> D[📖 读取源文件<br/>提取数据和哈希]
D --> E[📋 创建 AssetMetadata]
E --> F[💾 序列化为<br/>Player.png.meta]

style A fill:#e1f5fe
style F fill: #e8f5e8
```

### 💾 运行时资源加载 (Runtime Resource Loading)

```mermaid
graph TD
    A[🎯 系统请求资源] --> B{💾 检查缓存}
B -->|命中|C[✅ 返回缓存资源]
B -->|未命中|D{🔧 调用加载器}
D --> E{📋 获取元数据}
E --> F[🏗️ 创建运行时实例]
F --> G[💾 存入缓存]
G --> C

style C fill: #e8f5e8
style G fill: #fff3e0
```

### 🎭 场景实例化 (Scene Instantiation)

```mermaid
graph TD
    A[🎬 加载场景] --> B[🏗️ 创建 RuntimeScene]
B --> C[📋 解析场景数据]
C --> D{🎭 遍历实体节点}
D --> E{❓ 节点类型}
E -->|预制体实例| F[📦 加载预制体]
E -->|普通对象|G[🎮 创建游戏对象]
F --> H[🔧 应用组件覆盖]
G --> H
H --> I[🌳 递归创建子节点]
I --> J[✅ 完成场景创建]

style J fill:#e8f5e8
```

### 🔗 脚本系统互操作 (Scripting System Interop)

```mermaid
graph LR
    subgraph "C++ 引擎核心"
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

    subgraph "C# 脚本层"
        I[脚本组件]
        J[事件处理器]
        K[Interop 类]
        L[IJob 接口]
    end

    A --> C; D --> C; C --> G; C --> H; G --> K; H --> L; K --> J; J --> I; I --> K
```

### ✨ 可视化蓝图系统 (Visual Blueprint System)

```mermaid
graph TD
    subgraph "设计阶段"
        A[👨‍🎨 在编辑器中编辑蓝图] --> B{💾 保存为 .blueprint}
    end
    subgraph "编译阶段"
        B --> C{⚙️ C# 代码生成器}
        C --> D[📄 生成 MyBlueprint.cs]
        D --> E{🔧 C# 编译器}
    end
    subgraph "运行阶段"
        E --> F[📦 GameScripts.dll]
        F --> G[🚀 引擎在运行时加载]
    end
```

### ⚡ 物理更新循环 (Physics Update Loop)

```mermaid
graph TD
    A[⏰ 物理系统更新] --> B[📥 同步运动学刚体]
B --> C[🔄 读取 Transform 组件]
C --> D[📐 计算所需速度]
D --> E[⚡ 设置刚体速度]
E --> F[🌍 执行物理步进]
F --> G[📤 同步动态刚体]
G --> H[📍 读取刚体位置]
H --> I[🔄 更新 Transform 组件]

style F fill: #ffecb3
```

### 🎨 渲染管线 (Render Pipeline)

```mermaid
graph TD
subgraph "🖼️ 编辑器渲染循环"
A[🚀 开始帧] --> B[🎭 场景视图渲染]
B --> C[🔧 设置视口参数]
C --> D[📦 提取渲染数据]
D --> E[🎨 生成渲染包]
E --> F[⚡ 批处理优化]
F --> G[🖌️ 执行绘制命令]
G --> H[🖥️ UI 界面渲染]
H --> I[📺 呈现最终画面]
end

style I fill: #e8f5e8
```

### 🧩 Tilemap 系统 (Tilemap System)

```mermaid
graph TD
    subgraph "编辑时"
        A[🎨 创建 Tile Palette] --> B[🖌️ 添加笔刷]
        B --> C[👨‍🎨 在场景中绘制]
        C --> D[💾 序列化到场景文件]
    end
    subgraph "运行时"
        D --> E[⚙️ HydrateResources 初始化]
        E --> F[🧩 生成 Mesh/Sprites]
        F --> G[⚡️ 渲染与物理更新]
    end
```

### 🔊 音频系统 (Audio System)

```mermaid
graph TD
    subgraph "🎮 游戏逻辑 (C# / C++)"
        A["系统请求播放声音"]
    end

    subgraph "🎧 音频管理器 (主线程)"
        B{"Play(soundRequest)"}
        C["创建 Voice 实例"]
        D["将 Voice 添加到<br/>活动列表 (线程安全)"]
    end

    subgraph "🔊 音频线程回调"
        E["音频设备请求数据"]
        F{"Mix() 循环"}
        G["遍历所有活动 Voice"]
        H["计算空间衰减和声相"]
        I["混合音频样本"]
        J["写入输出缓冲区"]
    end

%% 定义主线程逻辑
    A --> B --> C --> D

%% 定义音频线程逻辑
    E --> F --> G --> H --> I --> J

%% 定义跨线程交互
    D -.->|共享活动列表| G
```

### ⚙️ JobSystem 并行处理 (JobSystem Parallel Processing)

```mermaid
graph TD
    subgraph "主线程"
        A[任务提交]
        B[任务完成等待]
    end

    subgraph "JobSystem 核心"
        C[全局任务队列]
        D[工作窃取调度器]
    end

    subgraph "工作线程池"
        E[工作线程 1]
        F[工作线程 2]
        G[工作线程 N]
        H[本地任务队列 1]
        I[本地任务队列 2]
        J[本地任务队列 N]
    end

    A --> C; C --> D; D --> E; D --> F; D --> G;
    E --> H; F --> I; G --> J;
    E -.-> I; E -.-> J; F -.-> H; F -.-> J; G -.-> H; G -.-> I;
    E --> B; F --> B; G --> B
```

### 🎬 动画状态机 (Animation State Machine)

```mermaid
graph TD
    subgraph "动画控制器"
        A[AnimationController]
        B[状态机实例]
        C[参数集合]
    end

    subgraph "状态节点"
        D[Idle 状态]
        E[Running 状态]
        F[Jump 状态]
    end

    subgraph "过渡条件"
        G[Bool参数: running]
        H[Trigger参数: jump]
        I[Float参数: speed]
    end

    A --> B; A --> C; B --> D; B --> E; B --> F;
    D -->|running = true| E; E -->|running = false| D;
    D -->|jump trigger| F; F -->|完成| D;
    C --> G; C --> H; C --> I
```

-----

[⬆️ 返回主文档 (Return to Main Document)](README.md)