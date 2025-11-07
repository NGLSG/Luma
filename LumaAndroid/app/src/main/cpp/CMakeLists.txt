cmake_minimum_required(VERSION 3.22.1)

project("lumaandroid")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# =====================================================================
# 选项: 选择使用预编译引擎库还是从源码编译引擎
# =====================================================================
option(USE_PREBUILT_ENGINE "使用预编译的引擎库而非从源码编译" OFF)

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

message(STATUS "========================================")
message(STATUS "LumaAndroid 项目配置完成")
message(STATUS "========================================")