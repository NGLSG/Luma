cmake_minimum_required(VERSION 3.21)
project(LumaEngine CXX)

# --- 全局设置 ---
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- 全局启用 UTF-8 ---
if (MSVC)
    add_compile_options(/utf-8)
else ()
    add_compile_options(-finput-charset=UTF-8 -fexec-charset=UTF-8)
endif ()

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif ()

set(ENGINE_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR})
message(STATUS "引擎根目录: ${ENGINE_ROOT_DIR}")

# --- 检查目标架构 ---
if (CMAKE_SIZEOF_VOID_P EQUAL 8)
    if (CMAKE_SYSTEM_PROCESSOR MATCHES "(aarch64|arm64)")
        set(LUMA_ARCH_NAME "arm64")
        add_compile_definitions(LUMA_ARM64)
    else ()
        set(LUMA_ARCH_NAME "x64")
        add_compile_definitions(LUMA_X64)
    endif ()
else ()
    message(FATAL_ERROR "Unsupported architecture. Only x64 and arm64 are supported.")
endif ()
message(STATUS "目标架构: ${LUMA_ARCH_NAME}")

# --- 检查系统 ---
if (WIN32)
    set(LUMA_PLATFORM_NAME "Windows")
    add_compile_definitions(LUMA_PLATFORM_WINDOWS)
elseif (UNIX AND NOT APPLE)
    set(LUMA_PLATFORM_NAME "Linux")
    add_compile_definitions(LUMA_PLATFORM_LINUX)
elseif (APPLE)
    set(LUMA_PLATFORM_NAME "MacOS")
    add_compile_definitions(LUMA_PLATFORM_MACOS)
else ()
    message(FATAL_ERROR "Unsupported platform.")
endif ()
message(STATUS "目标平台: ${LUMA_PLATFORM_NAME}")

# --- 检查指令集扩展 ---
include(CheckCXXCompilerFlag)

set(LUMA_SUPPORTED_EXTENSIONS "")

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    check_cxx_compiler_flag("-mavx" HAS_AVX)
    if (HAS_AVX)
        add_compile_definitions(LUMA_AVX)
        list(APPEND LUMA_SUPPORTED_EXTENSIONS "AVX")
        message(STATUS "启用 AVX 支持")
    endif ()

    check_cxx_compiler_flag("-mavx2" HAS_AVX2)
    if (HAS_AVX2)
        add_compile_definitions(LUMA_AVX2)
        list(APPEND LUMA_SUPPORTED_EXTENSIONS "AVX2")
        message(STATUS "启用 AVX2 支持")
    endif ()

    check_cxx_compiler_flag("-mavx512f" HAS_AVX512)
    if (HAS_AVX512)
        add_compile_definitions(LUMA_AVX512)
        list(APPEND LUMA_SUPPORTED_EXTENSIONS "AVX512")
        message(STATUS "启用 AVX512 支持")
    endif ()

    if (CMAKE_SYSTEM_PROCESSOR MATCHES "(aarch64|arm64)")
        check_cxx_compiler_flag("-march=armv8-a+simd" HAS_NENO)
        if (HAS_NENO)
            add_compile_options(-march=armv8-a+simd)
            add_compile_definitions(LUMA_NENO)
            list(APPEND LUMA_SUPPORTED_EXTENSIONS "NENO")
            message(STATUS "启用 NENO 支持")
        endif ()
    endif ()
elseif (MSVC)
    # MSVC 编译器的指令集检测
    set(MSVC_ARCH_FLAGS "")
    foreach (FLAG SSE2 SSE4.2 AVX AVX2 AVX512)
        string(TOLOWER ${FLAG} LOWERCASE_FLAG)
        string(REPLACE "." "" SAFE_FLAG ${LOWERCASE_FLAG})
        set(TEST_FILE ${CMAKE_BINARY_DIR}/test_${SAFE_FLAG}.cpp)
        file(WRITE ${TEST_FILE} "
        #include <immintrin.h>
        int main() {
            __m256 test = _mm256_setzero_ps();
            return 0;
        }")
        try_compile(HAS_${FLAG} ${CMAKE_BINARY_DIR} ${TEST_FILE} CMAKE_FLAGS "/D_USE_MATH_DEFINES /arch:${FLAG}")
        if (HAS_${FLAG})
            add_compile_definitions(LUMA_${FLAG})
            list(APPEND LUMA_SUPPORTED_EXTENSIONS ${FLAG})
            message(STATUS "启用 ${FLAG} 支持")
        endif ()
    endforeach ()
endif ()

message(STATUS "支持的指令集扩展: ${LUMA_SUPPORTED_EXTENSIONS}")

# --- 添加包含所有第三方库的子目录 ---
add_subdirectory(External)

# 从 External 获取平台信息
message(STATUS "检测到平台: ${LUMA_PLATFORM_NAME}-${LUMA_ARCH_NAME}")
message(STATUS "使用 .NET Runtime 路径: ${LUMA_DOTNET_RUNTIME_DIR}")

# --- 定义统一的引擎动态库 LumaEngine ---
file(GLOB_RECURSE LUMA_ENGINE_SOURCES CONFIGURE_DEPENDS
        "Application/*.cpp" "Application/*.h"
        "Components/*.cpp" "Components/*.h"
        "Data/*.cpp" "Data/*.h"
        "Renderer/*.cpp" "Renderer/*.h"
        "Resources/*.cpp" "Resources/*.h"
        "Runtime/*.cpp" "Runtime/*.h"
        "Scene/*.cpp" "Scene/*.h"
        "Systems/*.cpp" "Systems/*.h"
        "Utils/*.cpp" "Utils/*.h"
        "Scripting/*.cpp" "Scripting/*.h"
        "Event/*.cpp" "Event/*.h"
        "Input/*.cpp" "Input/*.h"
        "AIServices/include/*.h" "AIServices/src/*.cpp" "AIServices/src/Impls/*.cpp" "AIServices/include/Impls/*.h"
)
list(FILTER LUMA_ENGINE_SOURCES EXCLUDE REGEX ".*/Discarded/.*")

add_library(LumaEngine SHARED
        ${LUMA_ENGINE_SOURCES}
        Luma_CAPI.cpp
        Luma_CAPI.h
)

target_include_directories(LumaEngine PUBLIC
        ${ENGINE_ROOT_DIR}
        ${ENGINE_ROOT_DIR}/Application
        ${ENGINE_ROOT_DIR}/Components
        ${ENGINE_ROOT_DIR}/Data
        ${ENGINE_ROOT_DIR}/Renderer
        ${ENGINE_ROOT_DIR}/Resources
        ${ENGINE_ROOT_DIR}/Runtime
        ${ENGINE_ROOT_DIR}/Scene
        ${ENGINE_ROOT_DIR}/Systems
        ${ENGINE_ROOT_DIR}/Utils
        ${ENGINE_ROOT_DIR}/Scripting
        ${ENGINE_ROOT_DIR}/Event
        ${ENGINE_ROOT_DIR}/AIServices/include
)

target_compile_definitions(LumaEngine PUBLIC
        LUMA_ENGINE_EXPORTS
        GLM_ENABLE_EXPERIMENTAL
)

# 为引擎核心库链接所有第三方依赖
target_link_libraries(LumaEngine PUBLIC Luma3rd)

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    if (HAS_AVX)
        target_compile_options(LumaEngine PRIVATE -mavx)
    endif ()
    if (HAS_AVX2)
        target_compile_options(LumaEngine PRIVATE -mavx2 -mfma)
    endif ()
    if (HAS_AVX512)
        target_compile_options(LumaEngine PRIVATE -mavx512f -mavx512cd -mavx512vl -mavx512dq -mavx512bw)
    endif ()
endif ()

# 为 LumaEngine 添加特殊的链接选项来处理符号冲突
if (UNIX AND NOT APPLE)
    target_link_options(LumaEngine PRIVATE
            -Wl,--allow-multiple-definition
            -Wl,--export-dynamic
            -Wl,--allow-shlib-undefined
            -Wl,--gc-sections
    )
endif ()

target_precompile_headers(LumaEngine PUBLIC "Utils/PCH.h")

# --- 编辑器可执行文件 ---
add_executable(Editor main.cpp)
target_link_libraries(Editor PRIVATE LumaEngine)

# 为 Editor 定义 LUMA_EDITOR 宏，使其总是以编辑器模式编译
target_compile_definitions(Editor PUBLIC
        LUMA_EDITOR
        GLM_ENABLE_EXPERIMENTAL
)

# Linux 平台的特殊链接选项修复符号可见性问题
if (UNIX AND NOT APPLE)
    target_link_options(Editor PRIVATE
            -Wl,--allow-multiple-definition
            -Wl,--export-dynamic
            -Wl,--allow-shlib-undefined
            -Wl,--gc-sections
            -Wl,--no-as-needed
    )
endif ()

# --- 游戏可执行文件 ---
add_executable(Game main.cpp)

# 平台特定的链接配置
if (WIN32 AND MSVC)
    # Windows 平台使用延迟加载
    target_link_libraries(Game PRIVATE LumaEngine delayimp.lib)
    target_link_options(Game PRIVATE "/DELAYLOAD:LumaEngine.dll")
    # 启用大对象文件支持
    target_compile_options(LumaEngine PRIVATE /bigobj)
    target_compile_options(Editor PRIVATE /bigobj)
    target_compile_options(Game PRIVATE /bigobj)
else ()
    # 其他平台标准链接
    target_link_libraries(Game PRIVATE LumaEngine)
    # Linux 平台的特殊链接选项
    if (UNIX AND NOT APPLE)
        target_link_options(Game PRIVATE
                -Wl,--allow-multiple-definition
                -Wl,--export-dynamic
                -Wl,--allow-shlib-undefined
                -Wl,--gc-sections
                -Wl,--no-as-needed
        )
    endif ()
endif ()

target_compile_definitions(Game PRIVATE
        GLM_ENABLE_EXPERIMENTAL
)

# ===================================================================
# --- 统一资源复制函数 ---
# ===================================================================
function(CopyEngineAssets TARGET_NAME OUTPUT_DIR)
    # 复制 .NET CoreCLR 运行时（如果路径存在）
    if (EXISTS "${LUMA_DOTNET_RUNTIME_DIR}/bin")
        add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_directory
                "${LUMA_DOTNET_RUNTIME_DIR}/bin"
                "${OUTPUT_DIR}"
                COMMENT "正在为 ${TARGET_NAME} 复制 .NET CoreCLR 运行时..."
        )
    else ()
        message(WARNING "找不到 .NET Runtime bin 目录: ${LUMA_DOTNET_RUNTIME_DIR}/bin")
    endif ()

    # 复制 LumaEngine 动态库
    add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE:LumaEngine>"
            "${OUTPUT_DIR}"
            COMMENT "正在为 ${TARGET_NAME} 复制 LumaEngine 动态库..."
    )

    # 定义需要复制的资源文件夹列表
    set(RESOURCE_FOLDERS "Icons" "Fonts" "Tools" "template")

    foreach (FOLDER ${RESOURCE_FOLDERS})
        if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${FOLDER}")
            add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E copy_directory
                    "${CMAKE_CURRENT_SOURCE_DIR}/${FOLDER}"
                    "${OUTPUT_DIR}/${FOLDER}"
                    COMMENT "正在为 ${TARGET_NAME} 复制${FOLDER}文件..."
            )
        else ()
            message(WARNING "资源文件夹不存在: ${CMAKE_CURRENT_SOURCE_DIR}/${FOLDER}")
        endif ()
    endforeach ()
endfunction()

# ===================================================================
# --- 直接编译时的资源复制 ---
# ===================================================================
# Editor 直接编译时只复制资源到编译目录
set(EDITOR_OUTPUT_DIR "$<TARGET_FILE_DIR:Editor>")
CopyEngineAssets(Editor "${EDITOR_OUTPUT_DIR}")

# ===================================================================
# --- 统一的打包与发布目标 (publish) ---
# 该目标会同时创建:
# 1. Build/Arch:   完整的开发运行时包
# 2. Publish/Arch: 精简的游戏分发包
# ===================================================================

# --- 1. 定义所有需要的目录 ---
# 完整包目录 (原 package)
set(PACKAGE_ROOT_DIR "${CMAKE_SOURCE_DIR}/Build")
set(PACKAGE_PLATFORM_DIR "${PACKAGE_ROOT_DIR}/${LUMA_PLATFORM_NAME}")

# 分发包目录 (原 publish)
set(PUBLISH_ROOT_DIR "${CMAKE_SOURCE_DIR}/Publish")
set(PUBLISH_PLATFORM_DIR "${PUBLISH_ROOT_DIR}/${LUMA_PLATFORM_NAME}")
set(PUBLISH_GAMEDATA_DIR "${PUBLISH_PLATFORM_DIR}/GameData")

# --- 2. 创建统一的 publish 目标 ---
add_custom_target(publish
        COMMENT "正在统一执行打包和发布..."
)
add_dependencies(publish Editor Game LumaEngine)

# --- 3. 清理和创建所有目标目录 ---
add_custom_command(TARGET publish PRE_BUILD
        COMMAND ${CMAKE_COMMAND} -E remove_directory "${PACKAGE_PLATFORM_DIR}"
        COMMAND ${CMAKE_COMMAND} -E remove_directory "${PUBLISH_PLATFORM_DIR}"
        COMMENT "正在清理旧的打包和发布目录..."
)
add_custom_command(TARGET publish POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "${PACKAGE_PLATFORM_DIR}"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${PUBLISH_PLATFORM_DIR}"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${PUBLISH_GAMEDATA_DIR}"
        COMMENT "正在创建打包和发布目录结构..."
)


# ===================================================================
# --- 步骤 4A: 创建 Build/Arch 完整包 ---
# ===================================================================

# --- 4A.1: 复制所有编译产物到 Build/Arch ---
set(ALL_BUILD_ARTIFACTS "")
if (WIN32)
    file(GLOB EXECUTABLES CONFIGURE_DEPENDS "${CMAKE_BINARY_DIR}/*.exe")
    file(GLOB LIBRARIES CONFIGURE_DEPENDS "${CMAKE_BINARY_DIR}/*.dll")
    list(APPEND ALL_BUILD_ARTIFACTS ${EXECUTABLES} ${LIBRARIES})
elseif (UNIX AND NOT APPLE)
    # 首先，复制所有的 .so 文件
    file(GLOB LIBRARIES CONFIGURE_DEPENDS "${CMAKE_BINARY_DIR}/lib*.so*")
    list(APPEND ALL_BUILD_ARTIFACTS ${LIBRARIES})

    # 然后，明确地添加 Editor 和 Game 这两个可执行文件目标
    # $<TARGET_FILE:Game> 和 $<TARGET_FILE:Editor> 是生成器表达式，
    # 它们会在构建时被解析为目标的实际文件路径，这是最可靠的方式。
    list(APPEND ALL_BUILD_ARTIFACTS "$<TARGET_FILE:Game>")
    list(APPEND ALL_BUILD_ARTIFACTS "$<TARGET_FILE:Editor>")

    # 你依然可以保留查找无后缀文件的逻辑，以复制 apphost 等其他文件
    file(GLOB OTHER_FILES CONFIGURE_DEPENDS LIST_DIRECTORIES false "${CMAKE_BINARY_DIR}/*")
    list(REMOVE_DUPLICATES OTHER_FILES) # 避免重复

    foreach (FILE_PATH ${OTHER_FILES})
        get_filename_component(FILE_NAME ${FILE_PATH} NAME)
        get_filename_component(FILE_EXT ${FILE_PATH} EXT)
        # 确保是无后缀文件，并且不是目录或我们已经添加过的目标
        if (NOT IS_DIRECTORY "${FILE_PATH}" AND FILE_EXT STREQUAL "" AND NOT FILE_NAME STREQUAL "Makefile" AND NOT FILE_NAME STREQUAL "Game" AND NOT FILE_NAME STREQUAL "Editor")
            list(APPEND ALL_BUILD_ARTIFACTS "${FILE_PATH}")
        endif ()
    endforeach ()
endif ()

foreach (ARTIFACT ${ALL_BUILD_ARTIFACTS})
    add_custom_command(TARGET publish POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different "${ARTIFACT}" "${PACKAGE_PLATFORM_DIR}/"
    )
endforeach ()
add_custom_command(TARGET publish POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E echo "Step A.1 Complete: Copied build artifacts."
        COMMENT "步骤 A: 正在复制所有编译产物到 Build/${LUMA_PLATFORM_NAME}..."
)

# --- 4A.2: 复制资源文件夹到 Build/Arch ---
set(FOLDERS_TO_COPY Tools template Fonts Icons)
foreach (FOLDER ${FOLDERS_TO_COPY})
    if (EXISTS "${CMAKE_SOURCE_DIR}/${FOLDER}")
        add_custom_command(TARGET publish POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_directory
                "${CMAKE_SOURCE_DIR}/${FOLDER}" "${PACKAGE_PLATFORM_DIR}/${FOLDER}"
        )
    endif ()
endforeach ()
add_custom_command(TARGET publish POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E echo "Step A.2 Complete: Copied resource folders."
        COMMENT "步骤 A: 正在复制资源文件夹到 Build/${LUMA_PLATFORM_NAME}..."
)

# --- 4A.3: 复制配置文件到 Build/Arch ---
if (EXISTS "${CMAKE_BINARY_DIR}/imgui.ini")
    add_custom_command(TARGET publish POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_BINARY_DIR}/imgui.ini" "${PACKAGE_PLATFORM_DIR}/"
    )
endif ()
add_custom_command(TARGET publish POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E echo "Step A.3 Complete: Copied config files."
        COMMENT "步骤 A: 正在复制配置文件到 Build/${LUMA_PLATFORM_NAME}..."
)

# ===================================================================
# --- 步骤 4B: 创建 Publish/Arch 分发包 ---
# ===================================================================

# --- 4B.1: 复制 Game 可执行文件到 Publish/Arch ---
add_custom_command(TARGET publish POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy "$<TARGET_FILE:Game>" "${PUBLISH_PLATFORM_DIR}/"
        COMMENT "步骤 B: 正在复制 Game 可执行文件到 Publish/${LUMA_PLATFORM_NAME}..."
)

# --- 4B.2: 复制核心依赖到 Publish/Arch/GameData ---
set(FILES_TO_COPY_TO_GAMEDATA "")
if (WIN32)
    file(GLOB EXECUTABLES CONFIGURE_DEPENDS "${CMAKE_BINARY_DIR}/*.exe")
    file(GLOB LIBRARIES CONFIGURE_DEPENDS "${CMAKE_BINARY_DIR}/*.dll")
    list(APPEND FILES_TO_COPY_TO_GAMEDATA ${EXECUTABLES} ${LIBRARIES})
    list(REMOVE_ITEM FILES_TO_COPY_TO_GAMEDATA "${CMAKE_BINARY_DIR}/Game.exe")
    list(REMOVE_ITEM FILES_TO_COPY_TO_GAMEDATA "${CMAKE_BINARY_DIR}/Editor.exe") # Editor 也不需要
elseif (UNIX AND NOT APPLE)
    file(GLOB LIBRARIES CONFIGURE_DEPENDS "${CMAKE_BINARY_DIR}/lib*.so*")
    list(APPEND FILES_TO_COPY_TO_GAMEDATA ${LIBRARIES})
    # For the distribution package, we are intentionally selective. We only need apphost.
    if (EXISTS "${CMAKE_BINARY_DIR}/apphost")
        list(APPEND FILES_TO_COPY_TO_GAMEDATA "${CMAKE_BINARY_DIR}/apphost")
    endif ()
endif ()

foreach (FILE_PATH ${FILES_TO_COPY_TO_GAMEDATA})
    add_custom_command(TARGET publish POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different "${FILE_PATH}" "${PUBLISH_GAMEDATA_DIR}/"
    )
endforeach ()
add_custom_command(TARGET publish POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E echo "Step B.2 Complete: Copied core dependencies."
        COMMENT "步骤 B: 正在复制核心依赖到 Publish/${LUMA_PLATFORM_NAME}/GameData..."
)

# --- 4B.3: 复制 .NET SDK 文件到 Publish/Arch/GameData ---
set(SDK_SOURCE_DIR "${CMAKE_BINARY_DIR}/Tools/${LUMA_PLATFORM_NAME}")
set(SDK_FILES
        "Luma.SDK.deps.json" "Luma.SDK.dll" "Luma.SDK.runtimeconfig.json" "YamlDotNet.dll"
        "GameScripts.deps.json" "GameScripts.dll" "GameScripts.runtimeconfig.json"
        "ScriptMetadata.yaml" "Luma.SourceGenerator.deps.json" "Luma.SourceGenerator.dll"
)
foreach (SDK_FILE ${SDK_FILES})
    if (EXISTS "${SDK_SOURCE_DIR}/${SDK_FILE}")
        add_custom_command(TARGET publish POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different "${SDK_SOURCE_DIR}/${SDK_FILE}" "${PUBLISH_GAMEDATA_DIR}/"
        )
    endif ()
endforeach ()
add_custom_command(TARGET publish POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E echo "Step B.3 Complete: Copied .NET SDK files."
        COMMENT "步骤 B: 正在复制 .NET SDK 文件到 Publish/${LUMA_PLATFORM_NAME}/GameData..."
)

# --- 4B.4: 清理 Publish/Arch 中的调试文件 (仅 Windows) ---
if (WIN32)
    add_custom_command(TARGET publish POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E remove -f
            "${PUBLISH_PLATFORM_DIR}/Game.pdb"
            "${PUBLISH_GAMEDATA_DIR}/LumaEngine.pdb"
            "${PUBLISH_GAMEDATA_DIR}/LumaEngine.exp"
            "${PUBLISH_GAMEDATA_DIR}/LumaEngine.ilk"
            "${PUBLISH_GAMEDATA_DIR}/LumaEngine.lib"
            COMMENT "步骤 B: 正在清理 Publish 目录中的调试文件..."
    )
endif ()