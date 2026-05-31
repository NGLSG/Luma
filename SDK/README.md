# LumaEngine SDK

## 集成方式 / Integration

### CMake find_package
```cmake
find_package(LumaEngine REQUIRED)
add_executable(mygame main.cpp)
target_link_libraries(mygame PRIVATE LumaEngine::LumaEngine)
```

### 手动集成 / Manual Integration
1. 复制 include/ 到项目
2. 复制 lib/ 到项目
3. 链接 LumaEngine.dll/.so
4. 包含 `LumaEngine/Luma_CAPI.h`

### 公开头文件 / Public Headers
- `Luma_CAPI.h` - 完整的 C API (实体、组件、场景、物理、音频、动画、输入、相机、UI、材质、纹理、GPU缓冲区)
- `EngineEntry.h` - 引擎入口点 (Game/Editor Entry)
- `Utils/Platform.h` - 平台宏定义 (LUMA_API)

### 基本用法 / Basic Usage
```c
#include <LumaEngine/Luma_CAPI.h>
#include <LumaEngine/EngineEntry.h>

int main(int argc, char** argv) {
    // 方式1: 直接启动游戏运行时
    LumaEngine_Game_Entry(argc, argv, "./", "");

    // 方式2: 嵌入式集成 (Android/自定义窗口)
    // Engine_InitWithANativeWindow(window);
    // while (running) { Engine_Frame(); }
    // Engine_Shutdown();
    return 0;
}
```

### 平台支持 / Platform Support
- Windows (x64) - LumaEngine.dll
- Linux (x64) - libLumaEngine.so
- Android (arm64/x86_64) - libLumaEngine.so

### 依赖 / Dependencies
运行时需要以下依赖库与引擎 DLL 同目录:
- SDL3.dll
- Skia 库
- Dawn/wgpu 库
- .NET 9 Runtime (如果使用 C# 脚本)
