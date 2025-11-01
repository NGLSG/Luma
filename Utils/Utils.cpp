#include "Utils.h"

#include <openssl/sha.h> 
#include <openssl/evp.h> 
#include <memory>        
#include <array>         
#include <algorithm>     

#ifdef _WIN32
#include <windows.h>
#include <shellapi.h>
#else
#include <cstdlib>

#endif

void Utils::trimLeft(std::string& s)
{
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch)
    {
        return !std::isspace(ch);
    }));
}

void Utils::trimRight(std::string& s)
{
    s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch)
    {
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

    
    std::unique_ptr<FILE, int(*)(FILE*)> pipe(POPEN((command + " 2>&1").c_str(), "r"), PCLOSE);
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

void Utils::OpenFileExplorerAt(const std::filesystem::path& path)
{
#ifdef _WIN32
    ShellExecuteW(NULL, L"open", path.c_str(), NULL, NULL, SW_SHOWDEFAULT);
#elif defined(__APPLE__)
    std::string command = "open \"" + path.string() + "\"";
    
    int res=system(command.c_str());
#else
    std::string command = "xdg-open \"" + path.string() + "\"";
    
    int res=system(command.c_str());
#endif
}

void Utils::OpenBrowserAt(const std::string& url)
{
#ifdef _WIN32
    ShellExecuteW(NULL, L"open", std::wstring(url.begin(), url.end()).c_str(), NULL, NULL, SW_SHOWDEFAULT);
#elif defined(__APPLE__)
    std::string command = "open \"" + url + "\"";
    
    int res = system(command.c_str());
#else
    std::string command = "xdg-open \"" + url + "\"";
    
    int res = system(command.c_str());
#endif
}

std::string Utils::GetHashFromFile(const std::string& filePath)
{
    
    unsigned char hash[SHA256_DIGEST_LENGTH];
    unsigned int hashLen = 0;

    
    std::unique_ptr<EVP_MD_CTX, void(*)(EVP_MD_CTX*)> mdCtx(EVP_MD_CTX_new(), EVP_MD_CTX_free);
    if (!mdCtx)
    {
        return ""; 
    }

    if (EVP_DigestInit_ex(mdCtx.get(), EVP_sha256(), NULL) != 1)
    {
        return ""; 
    }

    std::unique_ptr<FILE, int(*)(FILE*)> file(fopen(filePath.c_str(), "rb"), fclose);
    if (!file)
    {
        return ""; 
    }

    const size_t bufferSize = 8192;
    unsigned char buffer[bufferSize];
    size_t bytesRead;

    while ((bytesRead = fread(buffer, 1, bufferSize, file.get())) > 0)
    {
        if (EVP_DigestUpdate(mdCtx.get(), buffer, bytesRead) != 1)
        {
            return ""; 
        }
    }

    if (EVP_DigestFinal_ex(mdCtx.get(), hash, &hashLen) != 1)
    {
        return ""; 
    }

    
    if (hashLen != SHA256_DIGEST_LENGTH)
    {
        return ""; 
    }

    std::string hashString;
    hashString.reserve(SHA256_DIGEST_LENGTH * 2); 
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
    unsigned int hashLen = 0;

    std::unique_ptr<EVP_MD_CTX, void(*)(EVP_MD_CTX*)> mdCtx(EVP_MD_CTX_new(), EVP_MD_CTX_free);
    if (!mdCtx)
    {
        return "";
    }

    if (EVP_DigestInit_ex(mdCtx.get(), EVP_sha256(), NULL) != 1)
    {
        return "";
    }

    if (EVP_DigestUpdate(mdCtx.get(), input.data(), input.size()) != 1)
    {
        return "";
    }

    if (EVP_DigestFinal_ex(mdCtx.get(), hash, &hashLen) != 1)
    {
        return "";
    }

    if (hashLen != SHA256_DIGEST_LENGTH)
    {
        return "";
    }

    std::string hashString;
    hashString.reserve(SHA256_DIGEST_LENGTH * 2);
    for (int i = 0; i < SHA256_DIGEST_LENGTH; ++i)
    {
        char hex[3];
        snprintf(hex, sizeof(hex), "%02x", hash[i]);
        hashString += hex;
    }
    return hashString;
}