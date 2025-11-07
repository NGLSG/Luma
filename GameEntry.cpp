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

#ifndef DLL_SEARCH_PATH
#define DLL_SEARCH_PATH "GameData"
#endif


static int GameEntryImpl(int argc, char* argv[], const char* executablePath)
{
    std::unique_ptr<ApplicationBase> app;
    ApplicationConfig config;

    ProjectSettings::GetInstance().LoadInRuntime();
    auto& settings = ProjectSettings::GetInstance();

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




ENTRY_API int LumaEngine_Game_Entry(int argc, char* argv[], const char* currentExePath)
{
    std::string executablePath;
    if (!currentExePath)
    {
        executablePath = GetExecutablePath();
    }
    else
    {
        executablePath = currentExePath;
    }
    LogInfo("Executable Path: {}", executablePath);
    return GameEntryImpl(argc, argv, executablePath.c_str());
}
