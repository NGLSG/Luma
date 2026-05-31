# LumaEngine Embedded Example

本示例展示如何将 LumaEngine 作为共享库集成到外部项目。

## 构建步骤

### 前置条件
- CMake >= 3.21
- LumaEngine SDK (构建产物或安装后的 SDK)

### 方式 A: 使用 find_package
```bash
# 1. 先构建并安装 LumaEngine SDK
cd /path/to/LumaEngine
cmake -B build -DLUMA_INSTALL_SDK=ON
cmake --build build
cmake --install build --prefix /path/to/install

# 2. 构建示例
cd SDK/examples/embedded
cmake -B build -DCMAKE_PREFIX_PATH=/path/to/install
cmake --build build
```

### 方式 B: 直接指定路径
```bash
cd SDK/examples/embedded
cmake -B build -DLUMA_SDK_DIR=/path/to/LumaEngine/build
cmake --build build
```

### 运行
```bash
# 确保 LumaEngine.dll 和依赖库在 PATH 或同目录
./build/embedded_game
```
