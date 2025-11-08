#include "EngineEntry.h"

#include <memory>
#include <iostream>
#include <clocale>
#include <filesystem>
#if defined(_WIN32) || defined(_WIN64)
#include <windows.h>
#else
#include <unistd.h>
#endif
#include <openssl/crypto.h>
#include <openssl/opensslv.h>

#ifndef _MSC_VER
#include "Resources/RuntimeAsset/RuntimeScene.h"
#endif

#include "Application/Editor.h"
#include "Application/ProjectSettings.h"
#include "Utils/Logger.h"
#include "Utils/PathUtils.h"

namespace
{
    void ConfigureWorkingDirectory(const char* executablePath)
    {
        if (!executablePath || executablePath[0] == '\0')
        {
            return;
        }

        std::filesystem::path path(executablePath);
        std::filesystem::path workDir;
        std::error_code ec;
        if (std::filesystem::is_directory(path, ec))
        {
            workDir = path;
        }
        else
        {
            workDir = path.parent_path();
        }

        if (!workDir.empty())
        {
            std::error_code changeEc;
            std::filesystem::current_path(workDir, changeEc);
            if (changeEc)
            {
                LogError("设置工作目录失败: {}", changeEc.message());
            }
            else
            {
                LogInfo("工作目录已设置为: {}", workDir.string());
            }
        }
    }
}


static int EditorEntryImpl(int argc, char* argv[], const char* executablePath)
{
    
#if defined(OPENSSL_VERSION_NUMBER) && (OPENSSL_VERSION_NUMBER >= 0x10100000L)
    OPENSSL_init_crypto(OPENSSL_INIT_NO_ATEXIT, nullptr);
#endif

    
    std::setlocale(LC_ALL, ".UTF8");

    
#if defined(_WIN32) || defined(_WIN64)
    SetConsoleOutputCP(CP_UTF8);
#endif

    ConfigureWorkingDirectory(executablePath);
    PathUtils::Initialize("Luma Editor");

    
    LogInfo("正在以编辑器模式启动...");
    ApplicationConfig config;
    config.title = "Luma Editor";
    config.width = 1600;
    config.height = 900;

    
    std::unique_ptr<ApplicationBase> app = std::make_unique<Editor>(config);

    
    try
    {
        app->Run();
    }
    catch (const std::exception& e)
    {
        LogError("编辑器遇到致命错误: {}", e.what());
#if defined(_WIN32) || defined(_WIN64)
        MessageBoxA(NULL, e.what(), "Fatal Error", MB_OK | MB_ICONERROR);
#endif
        return -1;
    }

    return 0;
}




ENTRY_API int LumaEngine_Editor_Entry(int argc, char* argv[], const char* currentExePath,
                                      const char* androidPackageName)
{
#if defined(__ANDROID__)
    PathUtils::InjectAndroidPackageName(androidPackageName ? androidPackageName : "");
#else
    (void)androidPackageName;
#endif

    std::string executablePath;
    if (!currentExePath)
    {
        executablePath = GetExecutablePath();
    }
    else
    {
        executablePath = currentExePath;
    }
    return EditorEntryImpl(argc, argv, executablePath.c_str());
}
