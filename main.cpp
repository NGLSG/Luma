#include <memory>
#include <iostream>
#include <clocale>
#if defined(_WIN32) || defined(_WIN64)
#include <windows.h>
#endif
#ifndef _MSC_VER
#include "Resources/RuntimeAsset/RuntimeScene.h"
#endif

#if defined(LUMA_EDITOR)
#include "Application/Editor.h"
#include "Application/ProjectSettings.h"
#else

#include "Application/Game.h"
#include "Application/ProjectSettings.h"
#endif

#include "Utils/Logger.h"

#ifndef DLL_SEARCH_PATH
#define DLL_SEARCH_PATH "GameData"
#endif


int main(int argc, char* argv[])
{
    std::setlocale(LC_ALL, ".UTF8");

#if defined(LUMA_EDITOR) && (defined(_WIN32) || defined(_WIN64))

    SetConsoleOutputCP(CP_UTF8);
#endif

    std::unique_ptr<ApplicationBase> app;
#if !defined(LUMA_EDITOR) && (defined(_WIN32) || defined(_WIN64))

    if (!SetDllDirectoryW(L"GameData"))
    {
        std::cerr << "Fatal Error: Could not set DLL search directory to 'GameData'.\n";
        return -1;
    }
#endif

    ApplicationConfig config;

#if defined(LUMA_EDITOR)

    LogInfo("Starting in Editor mode...");
    config.title = "Luma Editor";
    config.width = 1600;
    config.height = 900;

    app = std::make_unique<Editor>(config);
#else

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
#endif

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
