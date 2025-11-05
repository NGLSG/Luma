# Android NDK 工具链文件（兼容 NDK r27.0）
# 使用方式: cmake -DCMAKE_TOOLCHAIN_FILE=./android-ndk.toolchain.cmake

# 必需的 Android 变量设置
if (NOT ANDROID_PLATFORM)
    set(ANDROID_PLATFORM android-26)
endif ()

if (NOT ANDROID_ABI)
    set(ANDROID_ABI arm64-v8a)
endif ()

if (NOT ANDROID_NDK)
    # 尝试从环境变量获取
    if (DEFINED ENV{ANDROID_NDK_HOME})
        set(ANDROID_NDK $ENV{ANDROID_NDK_HOME})
    elseif (DEFINED ENV{ANDROID_NDK})
        set(ANDROID_NDK $ENV{ANDROID_NDK})
    else ()
        message(FATAL_ERROR "必须设置 ANDROID_NDK 路径或环境变量 ANDROID_NDK_HOME/ANDROID_NDK")
    endif ()
endif ()

# 检查 NDK 路径有效性
if (NOT EXISTS "${ANDROID_NDK}")
    message(FATAL_ERROR "ANDROID_NDK 路径不存在: ${ANDROID_NDK}")
endif ()

# 解析 ABI 到编译器目标三元组映射
if (ANDROID_ABI STREQUAL "arm64-v8a")
    set(ANDROID_ARCH arm64)
    set(ANDROID_LLVM_TRIPLE aarch64-linux-android)
    set(CMAKE_SYSTEM_PROCESSOR aarch64)
elseif (ANDROID_ABI STREQUAL "armeabi-v7a")
    set(ANDROID_ARCH armv7)
    set(ANDROID_LLVM_TRIPLE armv7a-linux-androideabi)
    set(CMAKE_SYSTEM_PROCESSOR armv7-a)
elseif (ANDROID_ABI STREQUAL "x86_64")
    set(ANDROID_ARCH x86_64)
    set(ANDROID_LLVM_TRIPLE x86_64-linux-android)
    set(CMAKE_SYSTEM_PROCESSOR x86_64)
elseif (ANDROID_ABI STREQUAL "x86")
    set(ANDROID_ARCH x86)
    set(ANDROID_LLVM_TRIPLE i686-linux-android)
    set(CMAKE_SYSTEM_PROCESSOR i686)
else ()
    message(FATAL_ERROR "不支持的 Android ABI: ${ANDROID_ABI}")
endif ()

# 提取 Android API 级别
string(REGEX MATCH "([0-9]+)$" ANDROID_API_LEVEL "${ANDROID_PLATFORM}")
set(ANDROID_API_LEVEL ${CMAKE_MATCH_1})

# CMake 系统设置
set(CMAKE_SYSTEM_NAME Android)
set(CMAKE_SYSTEM_VERSION ${ANDROID_API_LEVEL})

# 工具链路径
set(ANDROID_TOOLCHAIN_PREFIX "${ANDROID_NDK}/toolchains/llvm/prebuilt")

# 检测预构建工具链主机
if (WIN32)
    set(ANDROID_TOOLCHAIN_HOST "windows-x86_64")
elseif (APPLE)
    if (CMAKE_HOST_SYSTEM_PROCESSOR STREQUAL "arm64")
        set(ANDROID_TOOLCHAIN_HOST "darwin-arm64")
    else ()
        set(ANDROID_TOOLCHAIN_HOST "darwin-x86_64")
    endif ()
elseif (UNIX)
    if (CMAKE_HOST_SYSTEM_PROCESSOR STREQUAL "aarch64")
        set(ANDROID_TOOLCHAIN_HOST "linux-aarch64")
    else ()
        set(ANDROID_TOOLCHAIN_HOST "linux-x86_64")
    endif ()
else ()
    message(FATAL_ERROR "不支持的主机系统")
endif ()

set(ANDROID_TOOLCHAIN_ROOT "${ANDROID_TOOLCHAIN_PREFIX}/${ANDROID_TOOLCHAIN_HOST}")

if (NOT EXISTS "${ANDROID_TOOLCHAIN_ROOT}")
    message(FATAL_ERROR "NDK 工具链不存在: ${ANDROID_TOOLCHAIN_ROOT}")
endif ()

# 设置编译器
set(CMAKE_C_COMPILER "${ANDROID_TOOLCHAIN_ROOT}/bin/clang")
set(CMAKE_CXX_COMPILER "${ANDROID_TOOLCHAIN_ROOT}/bin/clang++")
set(CMAKE_ASM_COMPILER "${ANDROID_TOOLCHAIN_ROOT}/bin/clang")

# 关键修复：设置正确的系统根路径
set(CMAKE_SYSROOT "${ANDROID_TOOLCHAIN_ROOT}/sysroot")
set(CMAKE_FIND_ROOT_PATH "${CMAKE_SYSROOT}")
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

# NDK 源文件（某些库需要）
set(ANDROID_CPP_FEATURES_INCLUDE "${ANDROID_NDK}/sources/android/cpufeatures")

# 编译标志
set(ANDROID_COMPILER_FLAGS "")

# API 级别标志
list(APPEND ANDROID_COMPILER_FLAGS "-target ${ANDROID_LLVM_TRIPLE}${ANDROID_API_LEVEL}")

# 添加 NDK 特定的包含路径（修复 sys/types.h 问题）
set(ANDROID_SYSROOT_INCLUDE_FLAGS
        "-isysroot ${CMAKE_SYSROOT}"
)

# ABI 特定的标志
if (ANDROID_ABI STREQUAL "armeabi-v7a")
    list(APPEND ANDROID_COMPILER_FLAGS "-march=armv7-a" "-mfpu=neon" "-mfloat-abi=softfp")
elseif (ANDROID_ABI STREQUAL "x86")
    list(APPEND ANDROID_COMPILER_FLAGS "-march=i686")
endif ()

# 位置无关代码（所有 Android 库都必须是 PIC）
list(APPEND ANDROID_COMPILER_FLAGS "-fPIC")

# 设置编译标志
string(REPLACE ";" " " ANDROID_COMPILER_FLAGS_STR "${ANDROID_COMPILER_FLAGS}")
set(CMAKE_C_FLAGS "${ANDROID_COMPILER_FLAGS_STR} ${ANDROID_SYSROOT_INCLUDE_FLAGS} ${CMAKE_C_FLAGS}")
set(CMAKE_CXX_FLAGS "${ANDROID_COMPILER_FLAGS_STR} ${ANDROID_SYSROOT_INCLUDE_FLAGS} ${CMAKE_CXX_FLAGS}")
set(CMAKE_ASM_FLAGS "${ANDROID_COMPILER_FLAGS_STR} ${ANDROID_SYSROOT_INCLUDE_FLAGS} ${CMAKE_ASM_FLAGS}")

# 链接标志
set(CMAKE_SHARED_LINKER_FLAGS "-Wl,--exclude-libs,libunwind.a -Wl,--build-id=sha1 ${CMAKE_SHARED_LINKER_FLAGS}")
set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS}")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS}")

# 禁用 Android 特定的编译器警告和特性
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -U_LIBCPP_HAS_NO_EXPERIMENTAL_STOP_TOKEN")

# 符号可见性
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fvisibility=default -fvisibility-inlines-hidden")

# 禁用某些 GNU 扩展
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-gnu-unique")

# 不支持异常
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-exceptions")

# 输出诊断信息
message(STATUS "Android NDK 工具链配置:")
message(STATUS "  NDK 路径: ${ANDROID_NDK}")
message(STATUS "  目标 ABI: ${ANDROID_ABI}")
message(STATUS "  目标 API: ${ANDROID_API_LEVEL}")
message(STATUS "  主机平台: ${ANDROID_TOOLCHAIN_HOST}")
message(STATUS "  C 编译器: ${CMAKE_C_COMPILER}")
message(STATUS "  CXX 编译器: ${CMAKE_CXX_COMPILER}")
message(STATUS "  系统根路径: ${CMAKE_SYSROOT}")