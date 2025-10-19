cmake_minimum_required(VERSION 3.21)
project(LumaEngine CXX)

# =============================================================================
# 全局设置
# =============================================================================
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 启用 UTF-8 编码
if (MSVC)
    add_compile_options(/utf-8)
else ()
    add_compile_options(-finput-charset=UTF-8 -fexec-charset=UTF-8)
endif ()

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif ()

set(ENGINE_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR})
message(STATUS "========================================")
message(STATUS "开始配置 Luma Engine")
message(STATUS "引擎根目录: ${ENGINE_ROOT_DIR}")
message(STATUS "========================================")

# =============================================================================
# 添加第三方依赖 (这会设置 LUMA_PLATFORM_NAME 和 LUMA_ARCH_NAME)
# =============================================================================
add_subdirectory(External)

# 从 External 获取平台和架构信息
message(STATUS "检测到平台: ${LUMA_PLATFORM_NAME}-${LUMA_ARCH_NAME}")
message(STATUS "使用 .NET Runtime 路径: ${LUMA_DOTNET_RUNTIME_DIR}")

# =============================================================================
# 平台和架构编译定义
# =============================================================================
if (ANDROID)
    add_compile_definitions(LUMA_PLATFORM_ANDROID)
elseif (WIN32)
    add_compile_definitions(LUMA_PLATFORM_WINDOWS)
elseif (UNIX AND NOT APPLE)
    add_compile_definitions(LUMA_PLATFORM_LINUX)
elseif (APPLE)
    add_compile_definitions(LUMA_PLATFORM_MACOS)
endif ()

if (LUMA_ARCH_NAME STREQUAL "arm64")
    add_compile_definitions(LUMA_ARM64)
elseif (LUMA_ARCH_NAME STREQUAL "x64")
    add_compile_definitions(LUMA_X64)
elseif (LUMA_ARCH_NAME STREQUAL "x86")
    add_compile_definitions(LUMA_X86)
endif ()

# =============================================================================
# SIMD 指令集选项
# =============================================================================
option(ENABLE_AVX512 "启用 AVX512 指令集支持 (需要硬件支持)" OFF)

# =============================================================================
# 检查指令集扩展
# =============================================================================
include(CheckCXXCompilerFlag)

set(LUMA_SUPPORTED_EXTENSIONS "")
set(LUMA_SIMD_COMPILE_OPTIONS "")

# Android 平台的 NEON 支持
if (ANDROID AND CMAKE_ANDROID_ARCH_ABI STREQUAL "arm64-v8a")
    add_compile_definitions(LUMA_NEON)
    list(APPEND LUMA_SUPPORTED_EXTENSIONS "NEON")
    message(STATUS "启用 NEON 支持 (Android arm64-v8a)")
endif ()

# x86/x64 平台的 SIMD 扩展检测
if (NOT ANDROID AND LUMA_ARCH_NAME MATCHES "(x64|x86)")
    if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        # 检测 SSE 系列指令集
        check_cxx_compiler_flag("-msse" HAS_SSE)
        check_cxx_compiler_flag("-msse2" HAS_SSE2)
        check_cxx_compiler_flag("-msse3" HAS_SSE3)
        check_cxx_compiler_flag("-mssse3" HAS_SSSE3)
        check_cxx_compiler_flag("-msse4.1" HAS_SSE41)
        check_cxx_compiler_flag("-msse4.2" HAS_SSE42)

        # 检测 AVX 系列指令集
        check_cxx_compiler_flag("-mavx" HAS_AVX)
        check_cxx_compiler_flag("-mavx2" HAS_AVX2)
        check_cxx_compiler_flag("-mavx512f" HAS_AVX512)

        # 按优先级选择最佳指令集 (AVX512 需要显式启用)
        if (ENABLE_AVX512 AND HAS_AVX512)
            add_compile_definitions(LUMA_AVX512)
            list(APPEND LUMA_SUPPORTED_EXTENSIONS "AVX512")
            list(APPEND LUMA_SIMD_COMPILE_OPTIONS -mavx512f -mavx512cd -mavx512vl -mavx512dq -mavx512bw)
            message(STATUS "启用 AVX512 支持 (显式启用)")
        elseif (HAS_AVX2)
            add_compile_definitions(LUMA_AVX2)
            list(APPEND LUMA_SUPPORTED_EXTENSIONS "AVX2")
            list(APPEND LUMA_SIMD_COMPILE_OPTIONS -mavx2 -mfma)
            message(STATUS "启用 AVX2 支持 (自动选择)")
        elseif (HAS_AVX)
            add_compile_definitions(LUMA_AVX)
            list(APPEND LUMA_SUPPORTED_EXTENSIONS "AVX")
            list(APPEND LUMA_SIMD_COMPILE_OPTIONS -mavx)
            message(STATUS "启用 AVX 支持 (自动选择)")
        elseif (HAS_SSE42)
            add_compile_definitions(LUMA_SSE42)
            list(APPEND LUMA_SUPPORTED_EXTENSIONS "SSE4.2")
            list(APPEND LUMA_SIMD_COMPILE_OPTIONS -msse4.2)
            message(STATUS "启用 SSE4.2 支持 (自动选择)")
        elseif (HAS_SSE41)
            add_compile_definitions(LUMA_SSE41)
            list(APPEND LUMA_SUPPORTED_EXTENSIONS "SSE4.1")
            list(APPEND LUMA_SIMD_COMPILE_OPTIONS -msse4.1)
            message(STATUS "启用 SSE4.1 支持 (自动选择)")
        elseif (HAS_SSSE3)
            add_compile_definitions(LUMA_SSSE3)
            list(APPEND LUMA_SUPPORTED_EXTENSIONS "SSSE3")
            list(APPEND LUMA_SIMD_COMPILE_OPTIONS -mssse3)
            message(STATUS "启用 SSSE3 支持 (自动选择)")
        elseif (HAS_SSE3)
            add_compile_definitions(LUMA_SSE3)
            list(APPEND LUMA_SUPPORTED_EXTENSIONS "SSE3")
            list(APPEND LUMA_SIMD_COMPILE_OPTIONS -msse3)
            message(STATUS "启用 SSE3 支持 (自动选择)")
        elseif (HAS_SSE2)
            add_compile_definitions(LUMA_SSE2)
            list(APPEND LUMA_SUPPORTED_EXTENSIONS "SSE2")
            list(APPEND LUMA_SIMD_COMPILE_OPTIONS -msse2)
            message(STATUS "启用 SSE2 支持 (自动选择)")
        elseif (HAS_SSE)
            add_compile_definitions(LUMA_SSE)
            list(APPEND LUMA_SUPPORTED_EXTENSIONS "SSE")
            list(APPEND LUMA_SIMD_COMPILE_OPTIONS -msse)
            message(STATUS "启用 SSE 支持 (自动选择)")
        endif ()

    elseif (MSVC)
        # MSVC 的指令集检测和选择
        # MSVC 在 x64 下默认支持 SSE2
        if (LUMA_ARCH_NAME STREQUAL "x64")
            add_compile_definitions(LUMA_SSE2)
            list(APPEND LUMA_SUPPORTED_EXTENSIONS "SSE2")
        endif ()

        # 检测 AVX 支持
        set(TEST_AVX_FILE ${CMAKE_BINARY_DIR}/test_avx.cpp)
        file(WRITE ${TEST_AVX_FILE}
                "#include <immintrin.h>\n"
                "int main() { __m256 test = _mm256_setzero_ps(); return 0; }\n"
        )
        try_compile(HAS_AVX ${CMAKE_BINARY_DIR} ${TEST_AVX_FILE}
                CMAKE_FLAGS "/arch:AVX"
        )

        # 检测 AVX2 支持
        set(TEST_AVX2_FILE ${CMAKE_BINARY_DIR}/test_avx2.cpp)
        file(WRITE ${TEST_AVX2_FILE}
                "#include <immintrin.h>\n"
                "int main() { __m256i test = _mm256_abs_epi32(_mm256_setzero_si256()); return 0; }\n"
        )
        try_compile(HAS_AVX2 ${CMAKE_BINARY_DIR} ${TEST_AVX2_FILE}
                CMAKE_FLAGS "/arch:AVX2"
        )

        # 检测 AVX512 支持
        set(TEST_AVX512_FILE ${CMAKE_BINARY_DIR}/test_avx512.cpp)
        file(WRITE ${TEST_AVX512_FILE}
                "#include <immintrin.h>\n"
                "int main() { __m512 test = _mm512_setzero_ps(); return 0; }\n"
        )
        try_compile(HAS_AVX512 ${CMAKE_BINARY_DIR} ${TEST_AVX512_FILE}
                CMAKE_FLAGS "/arch:AVX512"
        )

        # 按优先级选择最佳指令集
        if (ENABLE_AVX512 AND HAS_AVX512)
            add_compile_definitions(LUMA_AVX512)
            list(APPEND LUMA_SUPPORTED_EXTENSIONS "AVX512")
            list(APPEND LUMA_SIMD_COMPILE_OPTIONS /arch:AVX512)
            message(STATUS "启用 AVX512 支持 (显式启用)")
        elseif (HAS_AVX2)
            add_compile_definitions(LUMA_AVX2)
            list(APPEND LUMA_SUPPORTED_EXTENSIONS "AVX2")
            list(APPEND LUMA_SIMD_COMPILE_OPTIONS /arch:AVX2)
            message(STATUS "启用 AVX2 支持 (自动选择)")
        elseif (HAS_AVX)
            add_compile_definitions(LUMA_AVX)
            list(APPEND LUMA_SUPPORTED_EXTENSIONS "AVX")
            list(APPEND LUMA_SIMD_COMPILE_OPTIONS /arch:AVX)
            message(STATUS "启用 AVX 支持 (自动选择)")
        endif ()
    endif ()
endif ()

if (LUMA_SUPPORTED_EXTENSIONS)
    message(STATUS "支持的指令集扩展: ${LUMA_SUPPORTED_EXTENSIONS}")
else ()
    message(STATUS "未检测到 SIMD 指令集扩展")
endif ()

# =============================================================================
# 定义引擎动态库 LumaEngine
# =============================================================================
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
        "Event/*.cpp" "Event/*.h"
        "Input/*.cpp" "Input/*.h"
        "AIServices/include/*.h"
        "AIServices/src/*.cpp"
        "AIServices/src/Impls/*.cpp"
        "AIServices/include/Impls/*.h"
)
list(FILTER LUMA_ENGINE_SOURCES EXCLUDE REGEX ".*/Discarded/.*")

# 添加脚本系统源文件
if (ANDROID)
    # Android 平台排除 CoreCLR 主机
    file(GLOB_RECURSE LUMA_SCRIPTING_SOURCES CONFIGURE_DEPENDS "Scripting/*.cpp" "Scripting/*.h")
    list(FILTER LUMA_SCRIPTING_SOURCES EXCLUDE REGEX ".*/CoreCLRHost\\..*")
    list(APPEND LUMA_ENGINE_SOURCES ${LUMA_SCRIPTING_SOURCES})
    message(STATUS "Android 平台: 使用 Mono 脚本主机")
else ()
    # 桌面平台使用 CoreCLR 主机
    file(GLOB_RECURSE LUMA_SCRIPTING_SOURCES CONFIGURE_DEPENDS "Scripting/*.cpp" "Scripting/*.h")
    list(APPEND LUMA_ENGINE_SOURCES ${LUMA_SCRIPTING_SOURCES})
    message(STATUS "桌面平台: 使用 CoreCLR 脚本主机")
endif ()

# 创建引擎动态库
add_library(LumaEngine SHARED
        ${LUMA_ENGINE_SOURCES}
        Luma_CAPI.cpp
        Luma_CAPI.h
)

# 设置包含目录
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
        ${ENGINE_ROOT_DIR}/Event
        ${ENGINE_ROOT_DIR}/Input
        ${ENGINE_ROOT_DIR}/Scripting
        ${ENGINE_ROOT_DIR}/AIServices/include
)

# 编译定义
target_compile_definitions(LumaEngine PUBLIC
        LUMA_ENGINE_EXPORTS
        GLM_ENABLE_EXPERIMENTAL
)

# 链接第三方库
target_link_libraries(LumaEngine PUBLIC Luma3rd)

# 应用 SIMD 编译选项
if (LUMA_SIMD_COMPILE_OPTIONS)
    target_compile_options(LumaEngine PRIVATE ${LUMA_SIMD_COMPILE_OPTIONS})
endif ()

# Linux/Android 平台的链接选项
if (UNIX AND NOT APPLE)
    target_link_options(LumaEngine PRIVATE
            -Wl,--allow-multiple-definition
            -Wl,--export-dynamic
            -Wl,--allow-shlib-undefined
            -Wl,--gc-sections
    )
endif ()

# 预编译头
target_precompile_headers(LumaEngine PUBLIC "Utils/PCH.h")

# =============================================================================
# 桌面平台可执行文件 (Editor 和 Game)
# =============================================================================
if (NOT ANDROID)
    # Editor 可执行文件
    add_executable(Editor main.cpp)
    target_link_libraries(Editor PRIVATE LumaEngine)
    target_compile_definitions(Editor PUBLIC
            LUMA_EDITOR
            GLM_ENABLE_EXPERIMENTAL
    )

    # Game 可执行文件
    add_executable(Game main.cpp)
    target_compile_definitions(Game PRIVATE GLM_ENABLE_EXPERIMENTAL)

    # 平台特定链接配置
    if (WIN32 AND MSVC)
        # Windows 延迟加载
        target_link_libraries(Game PRIVATE LumaEngine delayimp.lib)
        target_link_options(Game PRIVATE "/DELAYLOAD:LumaEngine.dll")

        # 大对象文件支持
        target_compile_options(LumaEngine PRIVATE /bigobj)
        target_compile_options(Editor PRIVATE /bigobj)
        target_compile_options(Game PRIVATE /bigobj)
    else ()
        target_link_libraries(Game PRIVATE LumaEngine)
    endif ()

    # Linux 平台链接选项
    if (UNIX AND NOT APPLE)
        target_link_options(Editor PRIVATE
                -Wl,--allow-multiple-definition
                -Wl,--export-dynamic
                -Wl,--allow-shlib-undefined
                -Wl,--gc-sections
                -Wl,--no-as-needed
        )
        target_link_options(Game PRIVATE
                -Wl,--allow-multiple-definition
                -Wl,--export-dynamic
                -Wl,--allow-shlib-undefined
                -Wl,--gc-sections
                -Wl,--no-as-needed
        )
    endif ()

    message(STATUS "已配置桌面平台可执行文件: Editor 和 Game")
else ()
    message(STATUS "Android 平台: 跳过 Editor 和 Game 可执行文件")
endif ()

# =============================================================================
# 资源复制函数
# =============================================================================
function(CopyEngineAssets TARGET_NAME OUTPUT_DIR)
    # 复制 .NET Runtime
    if (EXISTS "${LUMA_DOTNET_RUNTIME_DIR}/bin")
        add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_directory
                "${LUMA_DOTNET_RUNTIME_DIR}/bin" "${OUTPUT_DIR}"
                COMMENT "正在为 ${TARGET_NAME} 复制 .NET Runtime..."
        )
    else ()
        message(WARNING "找不到 .NET Runtime bin 目录: ${LUMA_DOTNET_RUNTIME_DIR}/bin")
    endif ()

    # 复制 LumaEngine 动态库
    add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE:LumaEngine>" "${OUTPUT_DIR}"
            COMMENT "正在为 ${TARGET_NAME} 复制 LumaEngine 动态库..."
    )

    # 复制资源文件夹
    set(RESOURCE_FOLDERS Icons Fonts Tools template)
    foreach (FOLDER ${RESOURCE_FOLDERS})
        if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${FOLDER}")
            add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E copy_directory
                    "${CMAKE_CURRENT_SOURCE_DIR}/${FOLDER}" "${OUTPUT_DIR}/${FOLDER}"
                    COMMENT "正在为 ${TARGET_NAME} 复制 ${FOLDER} 文件夹..."
            )
        else ()
            message(WARNING "资源文件夹不存在: ${CMAKE_CURRENT_SOURCE_DIR}/${FOLDER}")
        endif ()
    endforeach ()
endfunction()

# =============================================================================
# 直接编译时的资源复制
# =============================================================================
if (NOT ANDROID AND TARGET Editor)
    CopyEngineAssets(Editor "$<TARGET_FILE_DIR:Editor>")
endif ()

# =============================================================================
# 统一的打包与发布目标 (publish)
# =============================================================================

# 定义目录结构
set(PACKAGE_ROOT_DIR "${CMAKE_SOURCE_DIR}/Build")
set(PACKAGE_PLATFORM_DIR "${PACKAGE_ROOT_DIR}/${LUMA_PLATFORM_NAME}")
set(PUBLISH_ROOT_DIR "${CMAKE_SOURCE_DIR}/Publish")
set(PUBLISH_PLATFORM_DIR "${PUBLISH_ROOT_DIR}/${LUMA_PLATFORM_NAME}")
set(PUBLISH_GAMEDATA_DIR "${PUBLISH_PLATFORM_DIR}/GameData")

# 创建 publish 目标
add_custom_target(publish
        COMMENT "正在统一执行打包和发布..."
)

if (NOT ANDROID)
    add_dependencies(publish Editor Game LumaEngine)
else ()
    add_dependencies(publish LumaEngine)
endif ()

# 清理旧目录
add_custom_command(TARGET publish PRE_BUILD
        COMMAND ${CMAKE_COMMAND} -E remove_directory "${PACKAGE_PLATFORM_DIR}"
        COMMAND ${CMAKE_COMMAND} -E remove_directory "${PUBLISH_PLATFORM_DIR}"
        COMMENT "正在清理旧的打包和发布目录..."
)

# 创建目录结构
add_custom_command(TARGET publish POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "${PACKAGE_PLATFORM_DIR}"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${PUBLISH_PLATFORM_DIR}"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${PUBLISH_GAMEDATA_DIR}"
        COMMENT "正在创建打包和发布目录结构..."
)

# =============================================================================
# 步骤 A: 创建 Build/Platform 完整包
# =============================================================================

# A.1: 复制所有编译产物
if (WIN32)
    file(GLOB BUILD_EXECUTABLES CONFIGURE_DEPENDS "${CMAKE_BINARY_DIR}/*.exe")
    file(GLOB BUILD_LIBRARIES CONFIGURE_DEPENDS "${CMAKE_BINARY_DIR}/*.dll")
    set(ALL_BUILD_ARTIFACTS ${BUILD_EXECUTABLES} ${BUILD_LIBRARIES})
elseif (UNIX AND NOT APPLE AND NOT ANDROID)
    file(GLOB BUILD_LIBRARIES CONFIGURE_DEPENDS "${CMAKE_BINARY_DIR}/lib*.so*")
    set(ALL_BUILD_ARTIFACTS ${BUILD_LIBRARIES})

    # 明确添加可执行文件
    if (TARGET Game)
        list(APPEND ALL_BUILD_ARTIFACTS "$<TARGET_FILE:Game>")
    endif ()
    if (TARGET Editor)
        list(APPEND ALL_BUILD_ARTIFACTS "$<TARGET_FILE:Editor>")
    endif ()

    # 添加其他无后缀文件 (如 apphost)
    file(GLOB OTHER_FILES CONFIGURE_DEPENDS LIST_DIRECTORIES false "${CMAKE_BINARY_DIR}/*")
    foreach (FILE_PATH ${OTHER_FILES})
        get_filename_component(FILE_NAME ${FILE_PATH} NAME)
        get_filename_component(FILE_EXT ${FILE_PATH} EXT)
        if (NOT IS_DIRECTORY "${FILE_PATH}" AND
                FILE_EXT STREQUAL "" AND
                NOT FILE_NAME MATCHES "^(Makefile|CMake.*|Game|Editor)$")
            list(APPEND ALL_BUILD_ARTIFACTS "${FILE_PATH}")
        endif ()
    endforeach ()
elseif (ANDROID)
    file(GLOB BUILD_LIBRARIES CONFIGURE_DEPENDS "${CMAKE_BINARY_DIR}/lib*.so")
    set(ALL_BUILD_ARTIFACTS ${BUILD_LIBRARIES})
endif ()

foreach (ARTIFACT ${ALL_BUILD_ARTIFACTS})
    add_custom_command(TARGET publish POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different "${ARTIFACT}" "${PACKAGE_PLATFORM_DIR}/"
    )
endforeach ()
add_custom_command(TARGET publish POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E echo "步骤 A.1 完成: 已复制编译产物"
        COMMENT "正在复制编译产物到 Build/${LUMA_PLATFORM_NAME}..."
)

# A.2: 复制资源文件夹
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
        COMMAND ${CMAKE_COMMAND} -E echo "步骤 A.2 完成: 已复制资源文件夹"
        COMMENT "正在复制资源文件夹到 Build/${LUMA_PLATFORM_NAME}..."
)

# A.3: 复制配置文件
if (EXISTS "${CMAKE_BINARY_DIR}/imgui.ini")
    add_custom_command(TARGET publish POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_BINARY_DIR}/imgui.ini" "${PACKAGE_PLATFORM_DIR}/"
    )
endif ()
add_custom_command(TARGET publish POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E echo "步骤 A.3 完成: 已复制配置文件"
        COMMENT "正在复制配置文件到 Build/${LUMA_PLATFORM_NAME}..."
)

# =============================================================================
# 步骤 B: 创建 Publish/Platform 分发包
# =============================================================================

if (NOT ANDROID)
    # B.1: 复制 Game 可执行文件
    add_custom_command(TARGET publish POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy "$<TARGET_FILE:Game>" "${PUBLISH_PLATFORM_DIR}/"
            COMMENT "正在复制 Game 可执行文件到 Publish/${LUMA_PLATFORM_NAME}..."
    )
endif ()

# B.2: 复制核心依赖到 GameData
set(FILES_TO_COPY_TO_GAMEDATA "")
if (WIN32)
    file(GLOB PUBLISH_EXECUTABLES CONFIGURE_DEPENDS "${CMAKE_BINARY_DIR}/*.exe")
    file(GLOB PUBLISH_LIBRARIES CONFIGURE_DEPENDS "${CMAKE_BINARY_DIR}/*.dll")
    list(APPEND FILES_TO_COPY_TO_GAMEDATA ${PUBLISH_EXECUTABLES} ${PUBLISH_LIBRARIES})
    list(REMOVE_ITEM FILES_TO_COPY_TO_GAMEDATA "${CMAKE_BINARY_DIR}/Game.exe")
    list(REMOVE_ITEM FILES_TO_COPY_TO_GAMEDATA "${CMAKE_BINARY_DIR}/Editor.exe")
elseif (UNIX AND NOT APPLE AND NOT ANDROID)
    file(GLOB PUBLISH_LIBRARIES CONFIGURE_DEPENDS "${CMAKE_BINARY_DIR}/lib*.so*")
    list(APPEND FILES_TO_COPY_TO_GAMEDATA ${PUBLISH_LIBRARIES})
    if (EXISTS "${CMAKE_BINARY_DIR}/apphost")
        list(APPEND FILES_TO_COPY_TO_GAMEDATA "${CMAKE_BINARY_DIR}/apphost")
    endif ()
elseif (ANDROID)
    file(GLOB PUBLISH_LIBRARIES CONFIGURE_DEPENDS "${CMAKE_BINARY_DIR}/lib*.so")
    list(APPEND FILES_TO_COPY_TO_GAMEDATA ${PUBLISH_LIBRARIES})
endif ()

foreach (FILE_PATH ${FILES_TO_COPY_TO_GAMEDATA})
    add_custom_command(TARGET publish POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different "${FILE_PATH}" "${PUBLISH_GAMEDATA_DIR}/"
    )
endforeach ()
add_custom_command(TARGET publish POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E echo "步骤 B.2 完成: 已复制核心依赖"
        COMMENT "正在复制核心依赖到 Publish/${LUMA_PLATFORM_NAME}/GameData..."
)

# B.3: 复制 .NET SDK 文件
set(SDK_SOURCE_DIR "${CMAKE_BINARY_DIR}/Tools/${LUMA_PLATFORM_NAME}")
set(SDK_FILES
        "Luma.SDK.deps.json"
        "Luma.SDK.dll"
        "Luma.SDK.runtimeconfig.json"
        "YamlDotNet.dll"
        "GameScripts.deps.json"
        "GameScripts.dll"
        "GameScripts.runtimeconfig.json"
        "ScriptMetadata.yaml"
        "Luma.SourceGenerator.deps.json"
        "Luma.SourceGenerator.dll"
)
foreach (SDK_FILE ${SDK_FILES})
    if (EXISTS "${SDK_SOURCE_DIR}/${SDK_FILE}")
        add_custom_command(TARGET publish POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different "${SDK_SOURCE_DIR}/${SDK_FILE}" "${PUBLISH_GAMEDATA_DIR}/"
        )
    endif ()
endforeach ()
add_custom_command(TARGET publish POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E echo "步骤 B.3 完成: 已复制 .NET SDK 文件"
        COMMENT "正在复制 .NET SDK 文件到 Publish/${LUMA_PLATFORM_NAME}/GameData..."
)

# B.4: 清理调试文件 (仅 Windows)
if (WIN32)
    add_custom_command(TARGET publish POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E remove -f
            "${PUBLISH_PLATFORM_DIR}/Game.pdb"
            "${PUBLISH_GAMEDATA_DIR}/LumaEngine.pdb"
            "${PUBLISH_GAMEDATA_DIR}/LumaEngine.exp"
            "${PUBLISH_GAMEDATA_DIR}/LumaEngine.ilk"
            "${PUBLISH_GAMEDATA_DIR}/LumaEngine.lib"
            COMMENT "正在清理 Publish 目录中的调试文件..."
    )
endif ()

message(STATUS "========================================")
message(STATUS "Luma Engine 配置完成")
if (ENABLE_AVX512)
    message(STATUS "AVX512 指令集: 已显式启用")
else ()
    message(STATUS "AVX512 指令集: 未启用 (使用 -DENABLE_AVX512=ON 启用)")
endif ()
message(STATUS "========================================")