#ifndef LUMAENGINE_ENGINEENTRY_H
#define LUMAENGINE_ENGINEENTRY_H

#if defined(_WIN32) || defined(_WIN64)
#if defined(LUMA_ENGINE_EXPORTS)
#define ENTRY_API extern "C" __declspec(dllexport)
#else
#define ENTRY_API extern "C" __declspec(dllimport)
#endif
#else
#define ENTRY_API extern "C"
#endif

// 游戏运行时入口
ENTRY_API int LumaEngine_Game_Entry(int argc, char* argv[], const char* currentExePath = nullptr,
                                    const char* androidPackageName = nullptr);

// 编辑器入口
ENTRY_API int LumaEngine_Editor_Entry(int argc, char* argv[], const char* currentExePath = nullptr,
                                      const char* androidPackageName = nullptr);

#endif // LUMAENGINE_ENGINEENTRY_H
