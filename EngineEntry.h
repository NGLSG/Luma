#ifndef LUMAENGINE_ENGINEENTRY_H
#define LUMAENGINE_ENGINEENTRY_H

#include <string>

#if defined(_WIN32) || defined(_WIN64)
#if defined(LUMA_ENGINE_EXPORTS)
#define ENTRY_API extern "C" __declspec(dllexport)
#else
#define ENTRY_API extern "C" __declspec(dllimport)
#endif
#else
#define ENTRY_API extern "C"
#endif
// 获取当前可执行文件的完整路径
inline static std::string GetExecutablePath()
{
#if defined(_WIN32) || defined(_WIN64)
    char buffer[MAX_PATH];
    GetModuleFileNameA(nullptr, buffer, MAX_PATH);
    return std::string(buffer);
#else
    char buffer[PATH_MAX];
    ssize_t count = readlink("/proc/self/exe", buffer, sizeof(buffer) - 1);
    if (count != -1)
    {
        buffer[count] = '\0';
        return std::string(buffer);
    }
    return "";
#endif
}

// 游戏运行时入口
ENTRY_API int LumaEngine_Game_Entry(int argc, char* argv[], const char* currentExePath = nullptr,
                                    const char* androidPackageName = nullptr);

// 编辑器入口
ENTRY_API int LumaEngine_Editor_Entry(int argc, char* argv[], const char* currentExePath = nullptr,
                                      const char* androidPackageName = nullptr);

#endif // LUMAENGINE_ENGINEENTRY_H
