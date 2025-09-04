#include "Utils.h"
#ifdef _WIN32
#include <windows.h>
#include <shellapi.h>
#else
#include <cstdlib>
#endif
void Utils::trimLeft(std::string& s)
{
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) {
        return !std::isspace(ch);
    }));
}

void Utils::trimRight(std::string& s)
{
    s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) {
        return !std::isspace(ch);
    }).base(), s.end());
}

void Utils::trim(std::string& s)
{
    trimLeft(s);
    trimRight(s);
}

std::string Utils::ExecuteCommandAndGetOutput(const std::string& command)
{
    std::array<char, 256> buffer;
    std::string result;

    
    std::unique_ptr<FILE, decltype(&PCLOSE)> pipe(POPEN(command.c_str(), "r"), PCLOSE);
    if (!pipe)
    {
        
        return "";
    }

    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr)
    {
        result += buffer.data();
    }

    
    
    trim(result);

    return result;
}

void Utils::OpenFileExplorerAt(std::filesystem::path::iterator::reference path)
{
#ifdef _WIN32

    ShellExecuteW(NULL, L"open", path.c_str(), NULL, NULL, SW_SHOWDEFAULT);
#elif defined(__APPLE__)

    std::string command = "open \"" + path.string() + "\"";
    system(command.c_str());
#else

    std::string command = "xdg-open \"" + path.string() + "\"";
    system(command.c_str());
#endif
}

void Utils::OpenBrowserAt(const std::string& url)
{
#ifdef _WIN32

    ShellExecuteW(NULL, L"open", std::wstring(url.begin(), url.end()).c_str(), NULL, NULL, SW_SHOWDEFAULT);
#elif defined(__APPLE__)

    std::string command = "open \"" + url + "\"";
    system(command.c_str());
#else

    std::string command = "xdg-open \"" + url + "\"";
    system(command.c_str());
#endif
}

std::string Utils::GetHashFromFile(const std::string& filePath)
{
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_CTX sha256;
    SHA256_Init(&sha256);

    FILE* file = fopen(filePath.c_str(), "rb");
    if (!file)
    {
        return "";
    }

    const size_t bufferSize = 8192;
    unsigned char buffer[bufferSize];
    size_t bytesRead;

    while ((bytesRead = fread(buffer, 1, bufferSize, file)) > 0)
    {
        SHA256_Update(&sha256, buffer, bytesRead);
    }

    fclose(file);
    SHA256_Final(hash, &sha256);

    std::string hashString;
    for (int i = 0; i < SHA256_DIGEST_LENGTH; ++i)
    {
        char hex[3];
        snprintf(hex, sizeof(hex), "%02x", hash[i]);
        hashString += hex;
    }
    return hashString;
}

std::string Utils::GetHashFromString(const std::string& input)
{
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    SHA256_Update(&sha256, input.data(), input.size());
    SHA256_Final(hash, &sha256);

    std::string hashString;
    for (int i = 0; i < SHA256_DIGEST_LENGTH; ++i)
    {
        char hex[3];
        snprintf(hex, sizeof(hex), "%02x", hash[i]);
        hashString += hex;
    }
    return hashString;
}
