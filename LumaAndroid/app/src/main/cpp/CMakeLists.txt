cmake_minimum_required(VERSION 3.22.1)

project("lumaandroid")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# =====================================================================
# 选项: 选择使用预编译引擎库还是从源码编译引擎
# =====================================================================
option(USE_PREBUILT_ENGINE "使用预编译的引擎库而非从源码编译" OFF)
option(ENABLE_LIGHTWEIGHT_BUILD "启用轻量化构建（剥离符号并使用 shared libc++）" OFF)

# =====================================================================
# 轻量化构建配置
# =====================================================================
if(ENABLE_LIGHTWEIGHT_BUILD)
    message(STATUS "========================================")
    message(STATUS "Android 轻量化构建模式: 已启用")
    message(STATUS "========================================")

    # 强制使用 shared libc++
    set(ANDROID_STL "c++_shared" CACHE STRING "Android STL 类型" FORCE)
    message(STATUS "已设置 Android STL 为 c++_shared")

    # 添加优化编译选项
    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        # Release 模式的额外优化
        add_compile_options(
                -Oz                    # 优化体积
                -ffunction-sections    # 函数独立段
                -fdata-sections        # 数据独立段
        )
        add_link_options(
                -Wl,--gc-sections      # 链接时删除未使用的段
                -Wl,--as-needed        # 只链接需要的库
        )
        message(STATUS "已启用 Release 模式体积优化选项")
    else()
        # Debug 模式保留基本调试能力
        add_compile_options(
                -Os                    # 优化体积（保留调试信息）
                -ffunction-sections
                -fdata-sections
        )
        add_link_options(
                -Wl,--gc-sections
        )
        message(STATUS "已启用 Debug 模式体积优化选项")
    endif()
endif()

if(USE_PREBUILT_ENGINE)
    message(STATUS "========================================")
    message(STATUS "模式: 使用预编译引擎库")
    message(STATUS "========================================")

    # 预编译库路径
    set(PREBUILT_ENGINE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../jniLibs/${CMAKE_ANDROID_ARCH_ABI}")

    # 定义所有需要导入的预编译库（除了 mono 相关库）
    set(PREBUILT_LIBS
            "LumaEngine"
            "SDL3"
            "omp"
            "ggml-base"
            "ggml-cpu"
            "ggml"
            "llama"
    )

    # 检查并导入所有预编译库
    foreach(LIB_NAME ${PREBUILT_LIBS})
        set(LIB_PATH "${PREBUILT_ENGINE_DIR}/lib${LIB_NAME}.so")

        if(NOT EXISTS "${LIB_PATH}")
            message(FATAL_ERROR "预编译库不存在: ${LIB_PATH}\n"
                    "请确保已将所有预编译库放置在正确位置，或使用 -DUSE_PREBUILT_ENGINE=OFF 从源码编译")
        endif()

        # 导入预编译库
        add_library(${LIB_NAME} SHARED IMPORTED)
        set_target_properties(${LIB_NAME} PROPERTIES
                IMPORTED_LOCATION "${LIB_PATH}"
        )

        message(STATUS "已导入预编译库: ${LIB_NAME} -> ${LIB_PATH}")
    endforeach()

    # 如果是轻量化构建，对预编译库进行符号剥离
    if(ENABLE_LIGHTWEIGHT_BUILD)
        message(STATUS "正在为预编译库配置符号剥离...")
        foreach(LIB_NAME ${PREBUILT_LIBS})
            set(LIB_PATH "${PREBUILT_ENGINE_DIR}/lib${LIB_NAME}.so")
            if(EXISTS "${LIB_PATH}")
                if(CMAKE_BUILD_TYPE STREQUAL "Release")
                    # Release 模式完全剥离
                    add_custom_command(
                            OUTPUT "${LIB_PATH}.stripped"
                            COMMAND ${CMAKE_STRIP} --strip-all "${LIB_PATH}"
                            COMMAND ${CMAKE_COMMAND} -E touch "${LIB_PATH}.stripped"
                            DEPENDS "${LIB_PATH}"
                            COMMENT "正在剥离 ${LIB_NAME} 的所有符号..."
                    )
                else()
                    # Debug 模式仅剥离调试符号
                    add_custom_command(
                            OUTPUT "${LIB_PATH}.stripped"
                            COMMAND ${CMAKE_STRIP} --strip-debug "${LIB_PATH}"
                            COMMAND ${CMAKE_COMMAND} -E touch "${LIB_PATH}.stripped"
                            DEPENDS "${LIB_PATH}"
                            COMMENT "正在剥离 ${LIB_NAME} 的调试符号..."
                    )
                endif()
                list(APPEND STRIPPED_LIBS "${LIB_PATH}.stripped")
            endif()
        endforeach()
    endif()

    # 头文件目录（只需要 EngineEntry.h）
    set(LUMA_ENGINE_ROOT "${CMAKE_CURRENT_SOURCE_DIR}")

    message(STATUS "所有预编译库已成功导入")

else()
    message(STATUS "========================================")
    message(STATUS "模式: 从源码编译引擎")
    message(STATUS "========================================")

    # 从源码编译引擎
    set(LUMA_ENGINE_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../../../..")

    if(NOT EXISTS "${LUMA_ENGINE_ROOT}/CMakeLists.txt")
        message(FATAL_ERROR "引擎源码路径无效: ${LUMA_ENGINE_ROOT}\n"
                "请确保引擎源码存在，或使用 -DUSE_PREBUILT_ENGINE=ON 使用预编译库")
    endif()

    message(STATUS "正在从以下路径添加 LumaEngine: ${LUMA_ENGINE_ROOT}")
    add_subdirectory(${LUMA_ENGINE_ROOT} ${CMAKE_CURRENT_BINARY_DIR}/_engine_build)

endif()

# =====================================================================
# 主模块库
# =====================================================================
add_library(main SHARED
        sdl_main_wrapper.cpp
)

if(USE_PREBUILT_ENGINE)
    # 预编译模式：需要显式链接所有预编译库
    target_link_libraries(main
            android
            log
            LumaEngine
            SDL3
            omp
            ggml-base
            ggml-cpu
            ggml
            llama
    )

    # 轻量化构建：添加依赖关系
    if(ENABLE_LIGHTWEIGHT_BUILD AND STRIPPED_LIBS)
        add_custom_target(strip_prebuilt_libs ALL DEPENDS ${STRIPPED_LIBS})
        add_dependencies(main strip_prebuilt_libs)
    endif()
else()
    # 源码编译模式：引擎会自动处理依赖
    target_link_libraries(main
            android
            log
            LumaEngine
    )
endif()

target_include_directories(main PRIVATE ${LUMA_ENGINE_ROOT})

target_compile_features(main PRIVATE cxx_std_20)

# =====================================================================
# 轻量化构建：主模块符号剥离
# =====================================================================
if(ENABLE_LIGHTWEIGHT_BUILD)
    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        # Release 模式使用 --strip-all
        add_custom_command(TARGET main POST_BUILD
                COMMAND ${CMAKE_STRIP} --strip-all $<TARGET_FILE:main>
                COMMENT "正在剥离 main 模块的所有符号（Release 模式）..."
        )
    else()
        # Debug 模式使用 --strip-debug
        add_custom_command(TARGET main POST_BUILD
                COMMAND ${CMAKE_STRIP} --strip-debug $<TARGET_FILE:main>
                COMMENT "正在剥离 main 模块的调试符号（Debug 模式）..."
        )
    endif()

    message(STATUS "已为 main 模块启用符号剥离")
endif()

message(STATUS "========================================")
message(STATUS "LumaAndroid 项目配置完成")
if(ENABLE_LIGHTWEIGHT_BUILD)
    message(STATUS "轻量化构建: 已启用")
    message(STATUS "  - STL: ${ANDROID_STL}")
    message(STATUS "  - 符号剥离: ${CMAKE_BUILD_TYPE} 模式")
    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        message(STATUS "  - 优化级别: -Oz (最小体积)")
    else()
        message(STATUS "  - 优化级别: -Os (体积优化)")
    endif()
else()
    message(STATUS "轻量化构建: 未启用")
endif()
message(STATUS "========================================")