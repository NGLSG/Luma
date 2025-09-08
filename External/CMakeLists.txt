cmake_minimum_required(VERSION 3.21)
project(Luma3rd)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- 0. 引入 CPM.cmake ---
# 使用 CPM.cmake 统一管理第三方依赖
include(cmake/CPM.cmake)

# --- 1. 设置全局编译选项 ---
# 为所有目标启用位置无关代码 (PIC)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
option(USE_CUDA "启用 CUDA 进行 GPU 加速" OFF)

# 为 GNU GCC 和 Clang 启用 -fPIC 和符号可见性控制
if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC")

    # 针对 Linux 平台的符号可见性设置
    if (UNIX AND NOT APPLE)
        # 设置默认符号可见性为默认
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fvisibility=default")
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fvisibility=default")

        # 添加链接器选项来处理符号重复定义问题
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--allow-multiple-definition")
        set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--allow-multiple-definition")

        # 允许未定义符号
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--allow-shlib-undefined")
        set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--allow-shlib-undefined")

        # 强制导出所有符号
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--export-dynamic")

        # 使用链接时优化来删除重复符号
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--gc-sections")
        set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--gc-sections")

        message(STATUS "已为 Linux 平台配置符号可见性和链接器选项")
    endif ()

    message(STATUS "已为 ${CMAKE_CXX_COMPILER_ID} 编译器启用 -fPIC")
endif ()

# --- 2. 根据操作系统设置依赖项路径 ---
if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif ()

# 检测当前平台并设置对应的SDK路径
if (WIN32)
    message(STATUS "Platform: 正在为 Windows 配置...")
    set(PLATFORM_NAME "win")
    set(PLATFORM_FULL_NAME "Windows")
    if (CMAKE_GENERATOR_PLATFORM MATCHES "x64" OR CMAKE_SIZEOF_VOID_P EQUAL 8)
        set(ARCH_NAME "x64")
    elseif (CMAKE_GENERATOR_PLATFORM MATCHES "ARM64" OR CMAKE_SYSTEM_PROCESSOR MATCHES "ARM64")
        set(ARCH_NAME "arm64")
    else ()
        set(ARCH_NAME "x86")
    endif ()
elseif (UNIX AND NOT APPLE)
    message(STATUS "Platform: 正在为 Linux 配置...")
    set(PLATFORM_NAME "linux")
    set(PLATFORM_FULL_NAME "Linux")
    if (CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|AMD64")
        set(ARCH_NAME "x64")
    elseif (CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64|ARM64")
        set(ARCH_NAME "arm64")
    else ()
        set(ARCH_NAME "x86")
    endif ()
elseif (APPLE)
    message(STATUS "Platform: 正在为 macOS 配置...")
    set(PLATFORM_NAME "osx")
    set(PLATFORM_FULL_NAME "macOS")
    if (CMAKE_SYSTEM_PROCESSOR MATCHES "arm64|aarch64")
        set(ARCH_NAME "arm64")
    else ()
        set(ARCH_NAME "x64")
    endif ()
else ()
    message(FATAL_ERROR "检测到不支持的平台。")
endif ()

# 构建平台特定的SDK路径
set(Skia_DIR "${CMAKE_CURRENT_SOURCE_DIR}/skia-${PLATFORM_NAME}")
set(DotNetRuntime_DIR "${CMAKE_CURRENT_SOURCE_DIR}/coreclr-${PLATFORM_NAME}-${ARCH_NAME}")

message(STATUS "检测到平台: ${PLATFORM_NAME}-${ARCH_NAME}")
message(STATUS "使用 Skia SDK 路径: ${Skia_DIR}")
message(STATUS "使用 .NET Runtime 路径: ${DotNetRuntime_DIR}")

# 检查SDK路径是否存在
if (NOT EXISTS "${Skia_DIR}")
    message(WARNING "Skia SDK 路径不存在: ${Skia_DIR}")
endif ()
if (NOT EXISTS "${DotNetRuntime_DIR}")
    message(WARNING ".NET Runtime 路径不存在: ${DotNetRuntime_DIR}")
endif ()

# --- 3. 查找预编译的/系统的依赖包 ---
find_package(Vulkan REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(Skia REQUIRED)
find_package(DotNetRuntime REQUIRED)

# 平台特定的多媒体库查找
if (WIN32)
    find_package(FFMPEG REQUIRED)
    find_package(CURL REQUIRED)
elseif (UNIX AND NOT APPLE)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(FFMPEG REQUIRED
            libavformat>=58.0.0 libavcodec>=58.0.0 libswscale>=5.0.0
            libavutil>=56.0.0 libswresample>=3.0.0)
    if (FFMPEG_FOUND)
        add_library(FFmpeg::FFmpeg INTERFACE IMPORTED)
        target_include_directories(FFmpeg::FFmpeg INTERFACE ${FFMPEG_INCLUDE_DIRS})
        target_link_directories(FFmpeg::FFmpeg INTERFACE ${FFMPEG_LIBRARY_DIRS})
        target_link_libraries(FFmpeg::FFmpeg INTERFACE ${FFMPEG_LIBRARIES})
    else ()
        message(FATAL_ERROR "无法找到 FFmpeg 库。")
    endif ()
    find_package(CURL REQUIRED)
elseif (APPLE)
    find_package(FFMPEG REQUIRED)
    find_package(CURL REQUIRED)
endif ()

# --- 4. 使用 CPM 获取第三方源代码依赖 ---
CPMAddPackage(
        NAME SDL
        GITHUB_REPOSITORY libsdl-org/SDL
        GIT_TAG 2ef79441701c87c801fe0e1456321a791f4b2faf # main
        OPTIONS "SDL_SHARED OFF" "SDL_STATIC ON" "SDL_TEST OFF" "SDL_EXAMPLES OFF"
)
CPMAddPackage(
        NAME EnTT
        GITHUB_REPOSITORY skypjack/entt
        GIT_TAG 5642788046130821fbdf1729ef523d64ce4ff95e # master
)
CPMAddPackage(
        NAME yaml-cpp
        GITHUB_REPOSITORY jbeder/yaml-cpp
        GIT_TAG 2f86d13775d119edbb69af52e5f566fd65c6953b # master
        OPTIONS "YAML_CPP_BUILD_TESTS OFF" "YAML_CPP_BUILD_TOOLS OFF" "YAML_CPP_BUILD_CONTRIB OFF"
)
CPMAddPackage(
        NAME glm
        GITHUB_REPOSITORY g-truc/glm
        GIT_TAG 2d4c4b4dd31fde06cfffad7915c2b3006402322f # master
)
CPMAddPackage(
        NAME box2d
        GITHUB_REPOSITORY erincatto/box2d
        GIT_TAG df9787b59e4480135fbd73d275f007b5d931a83f # main
        OPTIONS "BOX2D_BUILD_UNIT_TESTS OFF" "BOX2D_BUILD_TESTBED OFF" "BOX2D_BUILD_DOCS OFF"
)
CPMAddPackage(
        NAME astc-encoder
        GITHUB_REPOSITORY ARM-software/astc-encoder
        GIT_TAG 27c6ab1bb6622c9998a816c78e8090f8fddd05a5 # main
        OPTIONS "ASTCENC_CLI OFF" "ASTCENC_UNITTEST OFF"
)
CPMAddPackage(
        NAME nlohmann_json
        GITHUB_REPOSITORY nlohmann/json
        GIT_TAG 55abcb5c0f0571b07a1717665859dbc5a9522f1b # develop
        OPTIONS "JSON_BuildTests OFF" "JSON_Install OFF"
)
CPMAddPackage(
        NAME imgui
        GITHUB_REPOSITORY ocornut/imgui
        GIT_TAG 1f7f1f54af38b0350d8c0008b096a9af6de299c7 # docking
)
CPMAddPackage(
        NAME imgui-node-editor
        GITHUB_REPOSITORY NGLSG/imgui-node-editor
        GIT_TAG dd5df3a1f218a25a3f23b0bf13187a03ed228a2b # master
)
CPMAddPackage(
        NAME ImGuizmo
        GITHUB_REPOSITORY CedricGuillemet/ImGuizmo
        GIT_TAG 3e725628fa64986c71ebc7c764621a8e7423fef4 # master
)

# --- 5. GPU 加速与 Llama.cpp 配置 ---
if (Vulkan_FOUND)
    message(STATUS "【GPU加速】使用 Vulkan")
    set(GGML_VULKAN ON CACHE BOOL "启用 Vulkan 支持" FORCE)
else ()
    message(STATUS "【GPU加速】将使用 CPU 模式")
endif ()

CPMAddPackage(
        NAME llama-cpp
        GITHUB_REPOSITORY ggerganov/llama.cpp
        GIT_TAG 492d7f1ff77e116e44962b832cc3db24cfc46d24 # master
        OPTIONS
        "LLAMA_BUILD_TESTS OFF"
        "LLAMA_BUILD_EXAMPLES OFF"
        "GGML_CUDA ${USE_CUDA}"
        "GGML_VULKAN ${GGML_VULKAN}"
)

# 为 llama 目标设置特定的 C++17 标准和链接选项 (安全方式)
if (TARGET llama)
    set_target_properties(llama PROPERTIES
            CXX_STANDARD 17
            CXX_STANDARD_REQUIRED ON
            POSITION_INDEPENDENT_CODE ON
    )
    if (UNIX AND NOT APPLE)
        target_link_options(llama PRIVATE -Wl,--allow-multiple-definition -Wl,--gc-sections)
    endif ()
    message(STATUS "已为 llama 目标设置 C++17 标准和特定链接选项")
endif ()

# --- 6. 配置从源码构建的库 ---
# astc-encoder
#[[set(ASTCENC_STATIC_TARGET astcenc-static)
if (TARGET ${ASTCENC_STATIC_TARGET})
    set_target_properties(${ASTCENC_STATIC_TARGET} PROPERTIES POSITION_INDEPENDENT_CODE ON)
    if (UNIX AND NOT APPLE)
        target_link_options(${ASTCENC_STATIC_TARGET} INTERFACE -Wl,--allow-multiple-definition)
    endif ()
    message(STATUS "已配置 astc-encoder 静态库: ${ASTCENC_STATIC_TARGET}")
else ()
    message(WARNING "未找到 astc-encoder 目标: ${ASTCENC_STATIC_TARGET}")
endif ()]]

# ImGui
add_library(ImGui_lib STATIC
        "${imgui_SOURCE_DIR}/imgui.cpp"
        "${imgui_SOURCE_DIR}/imgui_draw.cpp"
        "${imgui_SOURCE_DIR}/imgui_tables.cpp"
        "${imgui_SOURCE_DIR}/imgui_widgets.cpp"
        "${imgui_SOURCE_DIR}/backends/imgui_impl_sdl3.cpp"
        "${imgui_SOURCE_DIR}/backends/imgui_impl_wgpu.cpp"
        "${imgui_SOURCE_DIR}/misc/cpp/imgui_stdlib.cpp"
)
target_include_directories(ImGui_lib PUBLIC
        "${imgui_SOURCE_DIR}"
        "${imgui_SOURCE_DIR}/backends"
        "${imgui_SOURCE_DIR}/misc/cpp"
)
target_link_libraries(ImGui_lib PUBLIC SDL3::SDL3 Skia::Skia)
target_compile_definitions(ImGui_lib PUBLIC IMGUI_IMPL_WEBGPU_BACKEND_DAWN)
set_target_properties(ImGui_lib PROPERTIES POSITION_INDEPENDENT_CODE ON)
if (UNIX AND NOT APPLE)
    target_link_options(ImGui_lib PRIVATE -Wl,--allow-multiple-definition)
endif ()

# ImGui Node Editor
add_library(imgui_node_editor STATIC
        "${imgui-node-editor_SOURCE_DIR}/crude_json.cpp"
        "${imgui-node-editor_SOURCE_DIR}/imgui_canvas.cpp"
        "${imgui-node-editor_SOURCE_DIR}/imgui_node_editor_api.cpp"
        "${imgui-node-editor_SOURCE_DIR}/imgui_node_editor.cpp"
)
target_include_directories(imgui_node_editor PUBLIC "${imgui-node-editor_SOURCE_DIR}")
target_link_libraries(imgui_node_editor PUBLIC ImGui_lib)
set_target_properties(imgui_node_editor PROPERTIES POSITION_INDEPENDENT_CODE ON)
if (UNIX AND NOT APPLE)
    target_link_options(imgui_node_editor PRIVATE -Wl,--allow-multiple-definition)
endif ()

# ImGuizmo
add_library(ImGuizmo STATIC "${ImGuizmo_SOURCE_DIR}/ImGuizmo.cpp")
target_include_directories(ImGuizmo PUBLIC "${ImGuizmo_SOURCE_DIR}")
target_link_libraries(ImGuizmo PUBLIC ImGui_lib)
set_target_properties(ImGuizmo PROPERTIES POSITION_INDEPENDENT_CODE ON)
if (UNIX AND NOT APPLE)
    target_link_options(ImGuizmo PRIVATE -Wl,--allow-multiple-definition)
endif ()

# --- 7. 核心第三方库聚合器 ---
add_library(Luma3rd INTERFACE)

# 根据平台链接不同的 FFmpeg 目标
if (UNIX AND NOT APPLE)
    set(FFMPEG_TARGET_LIBS FFmpeg::FFmpeg)
else ()
    set(FFMPEG_TARGET_LIBS ${FFMPEG_LIBRARIES})
endif ()

# 对 Linux 平台，将所有需要特殊链接选项的静态库打包管理，避免符号冲突
if (UNIX AND NOT APPLE)
    add_library(StaticLibsGroup STATIC)
    # 创建一个虚拟源文件以满足 add_library 的要求
    file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/static_libs_group_dummy.cpp"
            "// 虚拟源文件，用于组合多个静态库\n"
            "extern \"C\" void static_libs_group_dummy() {}\n"
    )
    target_sources(StaticLibsGroup PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/static_libs_group_dummy.cpp")
    target_link_libraries(StaticLibsGroup PUBLIC
            ${ASTCENC_STATIC_TARGET}
            imgui_node_editor ImGui_lib ImGuizmo
            llama
    )
    target_link_options(StaticLibsGroup PRIVATE -Wl,--allow-multiple-definition -Wl,--gc-sections)
    set_target_properties(StaticLibsGroup PROPERTIES POSITION_INDEPENDENT_CODE ON)

    target_link_libraries(Luma3rd INTERFACE
            StaticLibsGroup # 组合的静态库
            Skia::Skia Vulkan::Vulkan SDL3::SDL3 EnTT::EnTT yaml-cpp glm::glm box2d
            OpenSSL::SSL OpenSSL::Crypto DotNet::Host
            ${FFMPEG_TARGET_LIBS} CURL::libcurl
            nlohmann_json::nlohmann_json
    )
else ()
    # 非 Linux 平台直接链接
    target_link_libraries(Luma3rd INTERFACE
            Skia::Skia Vulkan::Vulkan SDL3::SDL3 EnTT::EnTT yaml-cpp glm::glm box2d
            OpenSSL::SSL OpenSSL::Crypto DotNet::Host #${ASTCENC_STATIC_TARGET}
            imgui_node_editor ImGui_lib ImGuizmo nlohmann_json::nlohmann_json
            ${FFMPEG_TARGET_LIBS} CURL::libcurl
            llama
    )
endif ()

target_link_directories(Luma3rd INTERFACE ${FFMPEG_LIBRARY_DIRS})
target_include_directories(Luma3rd INTERFACE
        ${Vulkan_INCLUDE_DIRS}
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${FFMPEG_INCLUDE_DIRS}
)

# --- 8. 平台特定配置 ---
if (WIN32)
    target_link_libraries(Luma3rd INTERFACE
            d3d12 dxgi d3dcompiler user32 gdi32 winmm ole32 oleaut32
            imm32 version setupapi kernel32 shell32 advapi32
    )
    target_compile_definitions(Luma3rd INTERFACE
            _CRT_SECURE_NO_WARNINGS NOMINMAX WIN32_LEAN_AND_MEAN UNICODE _UNICODE
    )
elseif (UNIX AND NOT APPLE)
    target_link_libraries(Luma3rd INTERFACE
            dl pthread fontconfig X11 Xrandr Xinerama Xcursor Xi EGL OpenGL
    )
    target_compile_definitions(Luma3rd INTERFACE LINUX_PLATFORM)
elseif (APPLE)
    target_link_libraries(Luma3rd INTERFACE
            "-framework Cocoa" "-framework Metal" "-framework IOKit"
            "-framework CoreFoundation" "-framework CoreVideo" "-framework QuartzCore"
    )
    target_compile_definitions(Luma3rd INTERFACE MACOS_PLATFORM)
endif ()

# --- 9. 通用编译定义 ---
if (GGML_CUDA)
    target_compile_definitions(Luma3rd INTERFACE HAVE_CUDA=1)
    if (CMAKE_CUDA_COMPILER_VERSION VERSION_GREATER_EQUAL 12.0)
        target_compile_definitions(Luma3rd INTERFACE HAVE_CUDA12=1)
    endif ()
elseif (GGML_VULKAN)
    target_compile_definitions(Luma3rd INTERFACE HAVE_VULKAN=1)
else ()
    target_compile_definitions(Luma3rd INTERFACE CPU_ONLY=1)
endif ()

target_compile_definitions(Luma3rd INTERFACE
        $<$<CONFIG:Debug>:LDEBUG=1>
        $<$<CONFIG:Release>:LRELEASE=1>
)

# --- 10. 导出变量供主项目使用 ---
set(LUMA_PLATFORM_NAME ${PLATFORM_FULL_NAME} PARENT_SCOPE)
set(LUMA_ARCH_NAME ${ARCH_NAME} PARENT_SCOPE)
set(LUMA_DOTNET_RUNTIME_DIR ${DotNetRuntime_DIR} PARENT_SCOPE)

message(STATUS "=== 配置完成 ===")
