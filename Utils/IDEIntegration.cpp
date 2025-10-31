#include "IDEIntegration.h"
#include "Logger.h"

#ifdef _WIN32
#include <windows.h>
#include <shellapi.h>
#endif

IDE IDEIntegration::DetectInstalledIDE()
{
#ifdef _WIN32

    if (!FindRiderPath().empty()) return IDE::Rider;
    if (!FindVisualStudioPath().empty()) return IDE::VisualStudio;
    if (!FindVSCodePath().empty()) return IDE::VSCode;
#endif

    return IDE::Unknown;
}

bool IDEIntegration::Open(IDE ide, const std::filesystem::path& solutionPath, const std::filesystem::path& filePath)
{
#ifdef _WIN32
    std::wstring idePath;
    std::wstring args;

    switch (ide)
    {
    case IDE::Rider:
        idePath = L"Rider.cmd";
        args = L"\"" + solutionPath.wstring() + L"\" \"" + filePath.wstring() + L"\"";
        break;

    case IDE::VisualStudio:
        idePath = FindVisualStudioPath();
        if (idePath.empty()) return false;
        args = L"/edit \"" + filePath.wstring() + L"\" \"" + solutionPath.wstring() + L"\"";
        break;

    case IDE::VSCode:
        idePath = L"code.exe";
        args = L"--goto \"" + filePath.wstring() + L"\" \"" + solutionPath.parent_path().wstring() + L"\"";
        break;

    default:
        LogError("Unsupported or unknown IDE specified.");
        return false;
    }

    LogInfo("Opening IDE: {} with args: {}", idePath, idePath,
            args);
    ShellExecuteW(NULL, L"open", idePath.c_str(), args.c_str(), NULL, SW_SHOWNORMAL);
    return true;
#else
    LogWarn("Opening scripts in an IDE is not yet implemented on this platform.");
    return false;
#endif
}

#ifdef _WIN32
std::wstring IDEIntegration::GetPathFromRegistry(const std::wstring& keyPath, const std::wstring& valueName)
{
    HKEY hKey;
    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, keyPath.c_str(), 0, KEY_READ, &hKey) == ERROR_SUCCESS)
    {
        wchar_t buffer[MAX_PATH];
        DWORD bufferSize = sizeof(buffer);
        if (RegQueryValueExW(hKey, valueName.c_str(), nullptr, nullptr, (LPBYTE)buffer, &bufferSize) == ERROR_SUCCESS)
        {
            RegCloseKey(hKey);
            return std::wstring(buffer);
        }
        RegCloseKey(hKey);
    }
    return L"";
}

std::wstring IDEIntegration::FindVisualStudioPath()
{
    return GetPathFromRegistry(L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\App Paths\\devenv.exe", L"");
}

std::wstring IDEIntegration::FindVSCodePath()
{
    return GetPathFromRegistry(L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\App Paths\\Code.exe", L"");
}

std::wstring IDEIntegration::FindRiderPath()
{
    wchar_t buffer[MAX_PATH];
    if (SearchPathW(NULL, L"Rider.cmd", NULL, MAX_PATH, buffer, NULL) > 0)
    {
        return L"Rider.cmd";
    }

    std::wstring path = GetPathFromRegistry(L"SOFTWARE\\JetBrains\\Rider", L"InstallLocation");
    if (!path.empty())
    {
        std::wstring exePath = path + L"\\bin\\rider64.exe";
        if (std::filesystem::exists(exePath))
        {
            return exePath;
        }
    }

    return L"";
}
#endif
