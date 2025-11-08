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

#include "Application/Game.h"
#include "Application/ProjectSettings.h"
#include "Utils/Logger.h"
#include "Utils/PathUtils.h"

#ifndef DLL_SEARCH_PATH
#define DLL_SEARCH_PATH "GameData"
#endif


namespace
{
    void ConfigureWorkingDirectory(const char* executablePath)
    {
        if (!executablePath || executablePath[0] == '\0')
        {
            return;
        }

        std::filesystem::path inputPath(executablePath);
        std::filesystem::path workingDir;
        std::error_code ec;
        if (std::filesystem::is_directory(inputPath, ec))
        {
            workingDir = inputPath;
        }
        else
        {
            workingDir = inputPath.parent_path();
        }

        if (!workingDir.empty())
        {
            std::error_code changeEc;
            std::filesystem::current_path(workingDir, changeEc);
            if (changeEc)
            {
                LogError("Failed to set working directory to '{}': {}", workingDir.string(), changeEc.message());
            }
            else
            {
                LogInfo("Working directory set to '{}'", workingDir.string());
            }
        }
    }
}

static int GameEntryImpl(int argc, char* argv[], const char* executablePath)
{
    std::unique_ptr<ApplicationBase> app;
    ApplicationConfig config;

    ProjectSettings::GetInstance().LoadInRuntime();
    auto& settings = ProjectSettings::GetInstance();
    PathUtils::Initialize(settings.GetAppName());

#if defined(_WIN32) || defined(_WIN64)
    if (!settings.IsConsoleEnabled())
    {
        ::ShowWindow(::GetConsoleWindow(), SW_HIDE);
        ::FreeConsole();
    }
#endif

    config.title = settings.GetAppName();
    config.StartSceneGuid = settings.GetStartScene();
    config.width = settings.GetTargetWidth();
    config.height = settings.GetTargetHeight();

    app = std::make_unique<Game>(config);

    try
    {
        app->Run();
    }
    catch (const std::exception& e)
    {
        LogError("Application encountered a fatal error: {}", e.what());

#if defined(_WIN32) || defined(_WIN64)
        MessageBoxA(NULL, e.what(), "Fatal Error", MB_OK | MB_ICONERROR);
#endif
        return -1;
    }

    return 0;
}




ENTRY_API int LumaEngine_Game_Entry(int argc, char* argv[], const char* currentExePath,
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
    ConfigureWorkingDirectory(executablePath.c_str());
    LogInfo("Executable Path: {}", executablePath);
    return GameEntryImpl(argc, argv, executablePath.c_str());
}
