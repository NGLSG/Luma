cmake_minimum_required(VERSION 3.21)
set(CMAKE_SUPPRESS_DEVELOPER_WARNINGS TRUE)
project(Luma3rd)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# =============================================================================
# 引入 CPM.cmake
# =============================================================================
include(cmake/CPM.cmake)

# =============================================================================
# Android 平台全局强制使用 shared libc++
# =============================================================================
if(ANDROID)
    set(ANDROID_STL "c++_shared" CACHE STRING "Android STL 类型" FORCE)
    message(STATUS "Android 平台：已设置全局使用 c++_shared")
endif()

# =============================================================================
# 轻量化构建选项（仅影响符号剥离和体积优化）
# =============================================================================
option(ENABLE_LIGHTWEIGHT_BUILD "启用轻量化构建（剥离符号并优化体积）" OFF)

if(ENABLE_LIGHTWEIGHT_BUILD)
    message(STATUS "========================================")
    message(STATUS "轻量化构建模式: 已启用")
    message(STATUS "========================================")
endif()

# =============================================================================
# 平台与架构检测
# =============================================================================
message(STATUS "========================================")
message(STATUS "开始配置 Luma 第三方依赖")
message(STATUS "========================================")

if (ANDROID)
    set(PLATFORM_NAME "android")
    set(PLATFORM_FULL_NAME "Android")
    message(STATUS "检测到 Android 平台编译")
    message(STATUS "Android ABI: ${CMAKE_ANDROID_ARCH_ABI}")
    message(STATUS "Android Platform: ${ANDROID_PLATFORM}")
elseif (WIN32)
    set(PLATFORM_NAME "win")
    set(PLATFORM_FULL_NAME "Windows")
elseif (UNIX AND NOT APPLE)
    set(PLATFORM_NAME "linux")
    set(PLATFORM_FULL_NAME "Linux")
elseif (APPLE)
    set(PLATFORM_NAME "osx")
    set(PLATFORM_FULL_NAME "macOS")
else ()
    message(FATAL_ERROR "检测到不支持的平台")
endif ()

if (ANDROID)
    if (CMAKE_ANDROID_ARCH_ABI STREQUAL "arm64-v8a")
        set(ARCH_NAME "arm64")
    elseif (CMAKE_ANDROID_ARCH_ABI STREQUAL "x86_64")
        set(ARCH_NAME "x64")
    elseif (CMAKE_ANDROID_ARCH_ABI STREQUAL "armeabi-v7a")
        set(ARCH_NAME "arm")
    elseif (CMAKE_ANDROID_ARCH_ABI STREQUAL "x86")
        set(ARCH_NAME "x86")
    else ()
        message(FATAL_ERROR "不支持的 Android ABI: ${CMAKE_ANDROID_ARCH_ABI}")
    endif ()
else ()
    if (CMAKE_SIZEOF_VOID_P EQUAL 8)
        if (CMAKE_SYSTEM_PROCESSOR MATCHES "(aarch64|arm64|ARM64)")
            set(ARCH_NAME "arm64")
        elseif (CMAKE_SYSTEM_PROCESSOR MATCHES "(x86_64|AMD64|amd64)")
            set(ARCH_NAME "x64")
        else ()
            message(FATAL_ERROR "不支持的 64 位架构: ${CMAKE_SYSTEM_PROCESSOR}")
        endif ()
    else ()
        set(ARCH_NAME "x86")
    endif ()
endif ()

message(STATUS "平台: ${PLATFORM_FULL_NAME}")
message(STATUS "架构: ${ARCH_NAME}")

# =============================================================================
# 设置全局编译选项
# =============================================================================
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
option(USE_CUDA "启用 CUDA 进行 GPU 加速" OFF)

if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC")

    if (UNIX AND NOT APPLE)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fvisibility=default")
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fvisibility=default")
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--allow-multiple-definition")
        set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--allow-multiple-definition")
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--allow-shlib-undefined")
        set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--allow-shlib-undefined")
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--export-dynamic")
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--gc-sections")
        set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--gc-sections")
        message(STATUS "已为 Linux/Android 平台配置符号可见性和链接器选项")
    endif ()

    message(STATUS "已为 ${CMAKE_CXX_COMPILER_ID} 编译器启用 -fPIC")
endif ()

# =============================================================================
# 配置依赖项路径
# =============================================================================
if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif ()

set(Skia_DIR "${CMAKE_CURRENT_SOURCE_DIR}/skia-${PLATFORM_NAME}")

if (PLATFORM_NAME STREQUAL "android")
    set(DotNetRuntime_DIR "${CMAKE_CURRENT_SOURCE_DIR}/mono-${PLATFORM_NAME}-${ARCH_NAME}")
else ()
    set(DotNetRuntime_DIR "${CMAKE_CURRENT_SOURCE_DIR}/coreclr-${PLATFORM_NAME}-${ARCH_NAME}")
endif ()

message(STATUS "Skia SDK 路径: ${Skia_DIR}")
message(STATUS ".NET Runtime 路径: ${DotNetRuntime_DIR}")

if (NOT EXISTS "${Skia_DIR}")
    message(WARNING "Skia SDK 路径不存在: ${Skia_DIR}")
endif ()
if (NOT EXISTS "${DotNetRuntime_DIR}")
    message(WARNING ".NET Runtime 路径不存在: ${DotNetRuntime_DIR}")
endif ()

# =============================================================================
# 查找预编译依赖包
# =============================================================================
find_package(Vulkan REQUIRED)
if (Vulkan_FOUND)
    message(STATUS "✅ Vulkan: ${Vulkan_LIBRARY}")
else ()
    message(WARNING "⚠️  Vulkan 未找到，将禁用 GPU 加速")
endif ()

find_package(Skia REQUIRED)
find_package(DotNetRuntime REQUIRED)

if (PLATFORM_NAME STREQUAL "android")
    set(DOTNET_RUNTIME_TARGET DotNet::Mono)
    message(STATUS "✅ .NET Runtime: Mono")
else ()
    set(DOTNET_RUNTIME_TARGET DotNet::Host)
    message(STATUS "✅ .NET Runtime: CoreCLR")
endif ()

find_package(OpenSSL REQUIRED)
if (OpenSSL_FOUND)
    message(STATUS "✅ OpenSSL: ${OPENSSL_VERSION}")
else ()
    message(FATAL_ERROR "❌ 无法找到 OpenSSL")
endif ()

find_package(CURL REQUIRED)
if (CURL_FOUND)
    message(STATUS "✅ CURL: ${CURL_VERSION_STRING}")
else ()
    message(FATAL_ERROR "❌ 无法找到 CURL")
endif ()

if (WIN32 OR ANDROID OR APPLE)
    find_package(FFMPEG REQUIRED)
    if (FFMPEG_FOUND)
        set(FFMPEG_TARGET_LIBS ${FFMPEG_LIBRARIES})
        message(STATUS "✅ FFmpeg 已找到 (${PLATFORM_FULL_NAME})")
    else ()
        message(FATAL_ERROR "❌ 无法找到 FFmpeg")
    endif ()
elseif (UNIX AND NOT APPLE)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(FFMPEG REQUIRED
            libavformat>=58.0.0
            libavcodec>=58.0.0
            libswscale>=5.0.0
            libavutil>=56.0.0
            libswresample>=3.0.0
    )
    if (FFMPEG_FOUND)
        add_library(FFmpeg::FFmpeg INTERFACE IMPORTED)
        target_include_directories(FFmpeg::FFmpeg INTERFACE ${FFMPEG_INCLUDE_DIRS})
        target_link_directories(FFmpeg::FFmpeg INTERFACE ${FFMPEG_LIBRARY_DIRS})
        target_link_libraries(FFmpeg::FFmpeg INTERFACE ${FFMPEG_LIBRARIES})
        set(FFMPEG_TARGET_LIBS FFmpeg::FFmpeg)
        message(STATUS "✅ FFmpeg 已找到 (Linux 系统)")
    else ()
        message(FATAL_ERROR "❌ 无法找到 FFmpeg 库")
    endif ()
endif ()

# =============================================================================
# 使用 CPM 获取第三方源代码依赖
# =============================================================================
message(STATUS "正在通过 CPM 获取第三方源代码依赖...")

if (ANDROID)
    CPMAddPackage(
            NAME SDL
            GITHUB_REPOSITORY libsdl-org/SDL
            GIT_TAG 2ef79441701c87c801fe0e1456321a791f4b2faf
            OPTIONS
            "SDL_SHARED ON"
            "SDL_TEST OFF"
            "SDL_EXAMPLES OFF"
            "SDL_ANDROID ON"
    )
else ()
    CPMAddPackage(
            NAME SDL
            GITHUB_REPOSITORY libsdl-org/SDL
            GIT_TAG 2ef79441701c87c801fe0e1456321a791f4b2faf
            OPTIONS
            "SDL_SHARED OFF"
            "SDL_STATIC ON"
            "SDL_TEST OFF"
            "SDL_EXAMPLES OFF"
    )
endif ()

CPMAddPackage(
        NAME EnTT
        GITHUB_REPOSITORY skypjack/entt
        GIT_TAG 5642788046130821fbdf1729ef523d64ce4ff95e
)

CPMAddPackage(
        NAME yaml-cpp
        GITHUB_REPOSITORY jbeder/yaml-cpp
        GIT_TAG 2f86d13775d119edbb69af52e5f566fd65c6953b
        OPTIONS
        "YAML_CPP_BUILD_TESTS OFF"
        "YAML_CPP_BUILD_TOOLS OFF"
        "YAML_CPP_BUILD_CONTRIB OFF"
)

CPMAddPackage(
        NAME glm
        GITHUB_REPOSITORY g-truc/glm
        GIT_TAG 2d4c4b4dd31fde06cfffad7915c2b3006402322f
)

CPMAddPackage(
        NAME box2d
        GITHUB_REPOSITORY erincatto/box2d
        GIT_TAG df9787b59e4480135fbd73d275f007b5d931a83f
        OPTIONS
        "BOX2D_BUILD_UNIT_TESTS OFF"
        "BOX2D_BUILD_TESTBED OFF"
        "BOX2D_BUILD_DOCS OFF"
)

if (ANDROID)
    CPMAddPackage(
            NAME astc-encoder
            GITHUB_REPOSITORY ARM-software/astc-encoder
            GIT_TAG 27c6ab1bb6622c9998a816c78e8090f8fddd05a5
            OPTIONS
            "ASTCENC_CLI OFF"
            "ASTCENC_UNITTEST OFF"
            "ASTCENC_ISA_NONE ON"
    )
else ()
    CPMAddPackage(
            NAME astc-encoder
            GITHUB_REPOSITORY ARM-software/astc-encoder
            GIT_TAG 27c6ab1bb6622c9998a816c78e8090f8fddd05a5
            OPTIONS
            "ASTCENC_CLI OFF"
            "ASTCENC_UNITTEST OFF"
    )
endif ()

CPMAddPackage(
        NAME nlohmann_json
        GITHUB_REPOSITORY nlohmann/json
        GIT_TAG 55abcb5c0f0571b07a1717665859dbc5a9522f1b
        OPTIONS
        "JSON_BuildTests OFF"
        "JSON_Install OFF"
)

CPMAddPackage(
        NAME imgui
        GITHUB_REPOSITORY ocornut/imgui
        GIT_TAG 1f7f1f54af38b0350d8c0008b096a9af6de299c7
)

CPMAddPackage(
        NAME imgui-node-editor
        GITHUB_REPOSITORY NGLSG/imgui-node-editor
        GIT_TAG dd5df3a1f218a25a3f23b0bf13187a03ed228a2b
)

CPMAddPackage(
        NAME ImGuizmo
        GITHUB_REPOSITORY CedricGuillemet/ImGuizmo
        GIT_TAG 3e725628fa64986c71ebc7c764621a8e7423fef4
)

# =============================================================================
# GPU 加速与 Llama.cpp 配置
# =============================================================================
if (USE_CUDA)
    message(STATUS "【GPU加速】使用 CUDA")
    set(GGML_CUDA ON CACHE BOOL "启用 CUDA 支持" FORCE)
    set(GGML_VULKAN OFF CACHE BOOL "禁用 Vulkan 支持" FORCE)
elseif (Vulkan_FOUND AND NOT ANDROID)
    message(STATUS "【GPU加速】使用 Vulkan")
    set(GGML_VULKAN ON CACHE BOOL "启用 Vulkan 支持" FORCE)
    set(GGML_CUDA OFF CACHE BOOL "禁用 CUDA 支持" FORCE)
else ()
    message(STATUS "【GPU加速】将使用 CPU 模式")
    set(GGML_VULKAN OFF CACHE BOOL "禁用 Vulkan 支持" FORCE)
    set(GGML_CUDA OFF CACHE BOOL "禁用 CUDA 支持" FORCE)
endif ()

if (ANDROID)
    CPMAddPackage(
            NAME llama-cpp
            GITHUB_REPOSITORY ggerganov/llama.cpp
            GIT_TAG 492d7f1ff77e116e44962b832cc3db24cfc46d24
            OPTIONS
            "LLAMA_BUILD_TESTS OFF"
            "LLAMA_BUILD_EXAMPLES OFF"
            "GGML_CUDA OFF"
            "GGML_VULKAN OFF"
            "GGML_NATIVE OFF"
    )
else ()
    CPMAddPackage(
            NAME llama-cpp
            GITHUB_REPOSITORY ggerganov/llama.cpp
            GIT_TAG 492d7f1ff77e116e44962b832cc3db24cfc46d24
            OPTIONS
            "LLAMA_BUILD_TESTS OFF"
            "LLAMA_BUILD_EXAMPLES OFF"
            "GGML_CUDA ${GGML_CUDA}"
            "GGML_VULKAN ${GGML_VULKAN}"
    )
endif ()

if (TARGET llama)
    set_target_properties(llama PROPERTIES
            CXX_STANDARD 17
            CXX_STANDARD_REQUIRED ON
            POSITION_INDEPENDENT_CODE ON
    )
    if (UNIX AND NOT APPLE)
        target_link_options(llama PRIVATE -Wl,--allow-multiple-definition -Wl,--gc-sections)
    endif ()
    message(STATUS "已为 llama 目标设置 C++17 标准")
endif ()

# =============================================================================
# 配置从源码构建的库
# =============================================================================
set(ASTCENC_STATIC_TARGET "")
if (ANDROID)
    if (TARGET astcenc-none-static)
        set(ASTCENC_STATIC_TARGET astcenc-none-static)
    endif ()
else ()
    if (TARGET astcenc-avx2-static)
        set(ASTCENC_STATIC_TARGET astcenc-avx2-static)
    elseif (TARGET astcenc-native-static)
        set(ASTCENC_STATIC_TARGET astcenc-native-static)
    elseif (TARGET astcenc-none-static)
        set(ASTCENC_STATIC_TARGET astcenc-none-static)
    endif ()
endif ()

if (ASTCENC_STATIC_TARGET)
    set_target_properties(${ASTCENC_STATIC_TARGET} PROPERTIES POSITION_INDEPENDENT_CODE ON)
    if (UNIX AND NOT APPLE)
        target_link_options(${ASTCENC_STATIC_TARGET} INTERFACE -Wl,--allow-multiple-definition)
    endif ()
    message(STATUS "已配置 ASTC Encoder: ${ASTCENC_STATIC_TARGET}")
else ()
    message(WARNING "未找到 ASTC Encoder 目标")
endif ()

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

add_library(ImGuizmo STATIC "${ImGuizmo_SOURCE_DIR}/ImGuizmo.cpp")
target_include_directories(ImGuizmo PUBLIC "${ImGuizmo_SOURCE_DIR}")
target_link_libraries(ImGuizmo PUBLIC ImGui_lib)
set_target_properties(ImGuizmo PROPERTIES POSITION_INDEPENDENT_CODE ON)
if (UNIX AND NOT APPLE)
    target_link_options(ImGuizmo PRIVATE -Wl,--allow-multiple-definition)
endif ()

# =============================================================================
# 核心第三方库聚合器
# =============================================================================
add_library(Luma3rd INTERFACE)

set(STATIC_LIBS_TO_LINK imgui_node_editor ImGui_lib ImGuizmo llama)
if (ASTCENC_STATIC_TARGET)
    list(APPEND STATIC_LIBS_TO_LINK ${ASTCENC_STATIC_TARGET})
endif ()

if (UNIX AND NOT APPLE)
    add_library(StaticLibsGroup STATIC)
    file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/static_libs_group_dummy.cpp"
            "// 虚拟源文件，用于组合多个静态库\n"
            "extern \"C\" void static_libs_group_dummy() {}\n"
    )
    target_sources(StaticLibsGroup PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/static_libs_group_dummy.cpp")
    target_link_libraries(StaticLibsGroup PUBLIC ${STATIC_LIBS_TO_LINK})
    target_link_options(StaticLibsGroup PRIVATE -Wl,--allow-multiple-definition -Wl,--gc-sections)
    set_target_properties(StaticLibsGroup PROPERTIES POSITION_INDEPENDENT_CODE ON)

    target_link_libraries(Luma3rd INTERFACE
            StaticLibsGroup
            Skia::Skia
            $<$<BOOL:${Vulkan_FOUND}>:Vulkan::Vulkan>
            SDL3::SDL3
            EnTT::EnTT
            yaml-cpp
            glm::glm
            box2d
            OpenSSL::SSL
            OpenSSL::Crypto
            ${DOTNET_RUNTIME_TARGET}
            ${FFMPEG_TARGET_LIBS}
            CURL::libcurl
            nlohmann_json::nlohmann_json
    )
else ()
    target_link_libraries(Luma3rd INTERFACE
            ${STATIC_LIBS_TO_LINK}
            Skia::Skia
            $<$<BOOL:${Vulkan_FOUND}>:Vulkan::Vulkan>
            SDL3::SDL3
            EnTT::EnTT
            yaml-cpp
            glm::glm
            box2d
            OpenSSL::SSL
            OpenSSL::Crypto
            ${DOTNET_RUNTIME_TARGET}
            ${FFMPEG_TARGET_LIBS}
            CURL::libcurl
            nlohmann_json::nlohmann_json
    )
endif ()

target_include_directories(Luma3rd INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

if (Vulkan_FOUND)
    target_include_directories(Luma3rd INTERFACE ${Vulkan_INCLUDE_DIRS})
endif ()

if (UNIX AND NOT APPLE AND NOT ANDROID)
    target_link_directories(Luma3rd INTERFACE ${FFMPEG_LIBRARY_DIRS})
    target_include_directories(Luma3rd INTERFACE ${FFMPEG_INCLUDE_DIRS})
endif ()

# =============================================================================
# 平台特定系统库链接
# =============================================================================
if (WIN32)
    target_link_libraries(Luma3rd INTERFACE
            d3d12 dxgi d3dcompiler user32 gdi32 winmm ole32 oleaut32
            imm32 version setupapi kernel32 shell32 advapi32
    )
    target_compile_definitions(Luma3rd INTERFACE
            _CRT_SECURE_NO_WARNINGS
            NOMINMAX
            WIN32_LEAN_AND_MEAN
            UNICODE
            _UNICODE
    )
elseif (ANDROID)
    target_link_libraries(Luma3rd INTERFACE
            log android EGL GLESv3 dl z m
    )
    target_compile_definitions(Luma3rd INTERFACE ANDROID_PLATFORM)
elseif (UNIX AND NOT APPLE)
    target_link_libraries(Luma3rd INTERFACE
            dl pthread fontconfig X11 Xrandr Xinerama Xcursor Xi EGL OpenGL
    )
    target_compile_definitions(Luma3rd INTERFACE LINUX_PLATFORM)
elseif (APPLE)
    target_link_libraries(Luma3rd INTERFACE
            "-framework Cocoa"
            "-framework Metal"
            "-framework IOKit"
            "-framework CoreFoundation"
            "-framework CoreVideo"
            "-framework QuartzCore"
    )
    target_compile_definitions(Luma3rd INTERFACE MACOS_PLATFORM)
endif ()

# =============================================================================
# GPU 加速编译定义
# =============================================================================
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

# =============================================================================
# 构建类型编译定义
# =============================================================================
target_compile_definitions(Luma3rd INTERFACE
        $<$<CONFIG:Debug>:LDEBUG=1>
        $<$<CONFIG:Release>:LRELEASE=1>
)

# =============================================================================
# 导出变量供主项目使用
# =============================================================================
set(LUMA_PLATFORM_NAME ${PLATFORM_FULL_NAME} PARENT_SCOPE)
set(LUMA_ARCH_NAME ${ARCH_NAME} PARENT_SCOPE)
set(LUMA_DOTNET_RUNTIME_DIR ${DotNetRuntime_DIR} PARENT_SCOPE)

message(STATUS "========================================")
message(STATUS "第三方依赖配置完成")
message(STATUS "平台: ${PLATFORM_FULL_NAME}-${ARCH_NAME}")
message(STATUS ".NET Runtime: ${DOTNET_RUNTIME_TARGET}")
message(STATUS "GPU 加速: CUDA:${GGML_CUDA},Vulkan:${GGML_VULKAN}")
if(ANDROID)
    message(STATUS "Android STL: c++_shared (全局启用)")
endif()
if(ENABLE_LIGHTWEIGHT_BUILD)
    message(STATUS "轻量化构建: 已启用")
endif()
message(STATUS "========================================")