cmake_minimum_required(VERSION 3.21)
project(LumaEmbeddedExample LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)

set(LUMA_SDK_DIR "" CACHE PATH "Path to LumaEngine SDK or build directory")

# Method 1: find_package (if LumaEngine SDK is installed)
# find_package(LumaEngine REQUIRED)

add_executable(embedded_game main.cpp)

if(TARGET LumaEngine::LumaEngine)
    target_link_libraries(embedded_game PRIVATE LumaEngine::LumaEngine)
elseif(LUMA_SDK_DIR)
    target_include_directories(embedded_game PRIVATE "${LUMA_SDK_DIR}/include")
    if(WIN32)
        target_link_libraries(embedded_game PRIVATE "${LUMA_SDK_DIR}/lib/LumaEngine.lib")
    else()
        target_link_libraries(embedded_game PRIVATE "${LUMA_SDK_DIR}/lib/libLumaEngine.so")
    endif()
endif()
