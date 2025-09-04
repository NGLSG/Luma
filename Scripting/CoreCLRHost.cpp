#include "CoreCLRHost.h"
#include <iostream>
#include <chrono>

#include <iomanip>
#include <sstream>
#include <mutex>
#include "../Utils/Directory.h"
#include "../Utils/PCH.h"
#ifdef __linux__
#include <unistd.h>
#include <limits.h>
#elif _WIN32
#include <windows.h>
#include <codecvt>
#endif

namespace
{
    const std::wstring SdkAssemblyName = L"Luma.SDK";
    const std::wstring InteropTypeName = L"Luma.SDK.Interop";


    const std::wstring InitializeDomainMethodName = L"InitializeDomain";
    const std::wstring UnloadDomainMethodName = L"UnloadDomain";


    const std::wstring CreateMethodName = L"CreateScriptInstance";
    const std::wstring OnCreateMethodName = L"OnCreate";
    const std::wstring DestroyMethodName = L"DestroyScriptInstance";
    const std::wstring UpdateMethodName = L"InvokeUpdate";
    const std::wstring SetPropertyMethodName = L"SetExportedProperty";
    const std::wstring DebugListMethodName = L"Debug_ListAllTypesAndMethods";
    const std::wstring InvokeMethodName = L"InvokeMethod";
    const std::wstring DispatchCollisionEventMethodName = L"DispatchCollisionEvent";
    const std::wstring CallOnEnableMethodName = L"OnEnable";
    const std::wstring CallOnDisableMethodName = L"OnDisable";
}

std::filesystem::path get_executable_directory()
{
#ifdef _WIN32
    wchar_t path[MAX_PATH] = {0};
    GetModuleFileNameW(NULL, path, MAX_PATH);
    return std::filesystem::path(path).parent_path();
#elif __linux__
    char result[PATH_MAX];
    ssize_t count = readlink("/proc/self/exe", result, PATH_MAX);
    if (count > 0)
    {
        return std::filesystem::path(result).parent_path();
    }
#endif
    
    return std::filesystem::current_path();
}

std::mutex CoreCLRHost::s_mutex;

CoreCLRHost* CoreCLRHost::GetInstance()
{
    return s_instance.get();
}

void CoreCLRHost::CreateNewInstance()
{
    std::lock_guard<std::mutex> lock(s_mutex);

    if (s_instance)
    {
        LogInfo("CoreCLRHost: 销毁旧的单例实例");
        s_instance->Shutdown();
        s_instance.reset();
    }

    s_instance = std::unique_ptr<CoreCLRHost>(new CoreCLRHost());
}

void CoreCLRHost::DestroyInstance()
{
    std::lock_guard<std::mutex> lock(s_mutex);

    if (s_instance)
    {
        s_instance->Shutdown();
        s_instance.reset();
        s_instance = nullptr;
    }
}

CoreCLRHost::~CoreCLRHost()
{
    Shutdown();
}


bool CoreCLRHost::Initialize(const std::filesystem::path& mainAssemblyPath, bool isEditorMode)
{
    if (m_isInitialized)
    {
        LogInfo("CoreCLRHost: 检测到重复初始化请求，执行完全关闭以便重新加载托管域。");
        Shutdown();
    }

    m_isEditorMode = isEditorMode;
    const std::filesystem::path sourceDir = mainAssemblyPath.parent_path();
    std::filesystem::path effectiveAssemblyDir;
    std::error_code fsEc;

#ifdef _WIN32
    std::filesystem::path engineDir = std::filesystem::current_path();
    std::wstring engineDirW = engineDir.wstring();
    if (!SetDllDirectoryW(engineDirW.c_str()))
    {
        LogWarn("CoreCLRHost: 设置DLL搜索目录失败: {}", engineDir.string());
    }

    std::wstring currentPath;
    DWORD pathSize = GetEnvironmentVariableW(L"PATH", nullptr, 0);
    if (pathSize > 0)
    {
        currentPath.resize(pathSize);
        GetEnvironmentVariableW(L"PATH", currentPath.data(), pathSize);
        currentPath.resize(pathSize - 1);
        std::wstring newPath = engineDirW + L";" + currentPath;
        if (!SetEnvironmentVariableW(L"PATH", newPath.c_str()))
        {
            LogWarn("CoreCLRHost: 更新PATH环境变量失败");
        }
    }
#else 
    std::filesystem::path engineDir = get_executable_directory(); 
    const char* ld_path = getenv("LD_LIBRARY_PATH");
    std::string new_ld_path = engineDir.string(); 
    if (ld_path && ld_path[0] != '\0')
    {
        new_ld_path += ":";
        new_ld_path += ld_path;
    }
    LogInfo("CoreCLRHost: 设置LD_LIBRARY_PATH为: {}", new_ld_path);
    if (setenv("LD_LIBRARY_PATH", new_ld_path.c_str(), 1) != 0)
    {
        LogWarn("CoreCLRHost: Failed to update LD_LIBRARY_PATH environment variable.");
    }
#endif

    if (isEditorMode)
    {
        m_shadowCopyBasePath = ".LumaEditorTemp";
        if (m_isFirstInitialization)
        {
            cleanupOldShadowCopies();
            m_isFirstInitialization = false;
        }

        m_activeShadowCopyPath = createNewShadowCopyDirectory();
        if (m_activeShadowCopyPath.empty() || !copyAssembliesToShadowDirectory(sourceDir, m_activeShadowCopyPath))
        {
            LogError("CoreCLRHost: 初始化影子副本失败。");
            return false;
        }
        effectiveAssemblyDir = m_activeShadowCopyPath;
    }
    else
    {
        effectiveAssemblyDir = sourceDir;
    }

    if (!bootstrapRuntimeIfNeeded(sourceDir))
    {
        LogError("CoreCLRHost: 引导 CLR 失败。");
        return false;
    }

    m_scriptAssembliesPath = Directory::GetAbsolutePath(effectiveAssemblyDir.string());

    m_initializeDomainFn = (InitializeDomainFn)getManagedFunction(SdkAssemblyName, InteropTypeName,
                                                                  InitializeDomainMethodName);
    m_unloadDomainFn = (UnloadDomainFn)getManagedFunction(SdkAssemblyName, InteropTypeName, UnloadDomainMethodName);
    if (!m_initializeDomainFn || !m_unloadDomainFn)
    {
        LogError("CoreCLRHost: 获取域管理委托失败。");
        Shutdown();
        return false;
    }

    m_initializeDomainFn(pathToUtf8(m_scriptAssembliesPath).c_str());

    m_createInstanceFn = (CreateInstanceFn)getManagedFunction(SdkAssemblyName, InteropTypeName, CreateMethodName);
    m_onCreateFn = (OnCreateFn)getManagedFunction(SdkAssemblyName, InteropTypeName, OnCreateMethodName);
    m_destroyInstanceFn = (DestroyInstanceFn)getManagedFunction(SdkAssemblyName, InteropTypeName, DestroyMethodName);
    m_updateInstanceFn = (UpdateInstanceFn)getManagedFunction(SdkAssemblyName, InteropTypeName, UpdateMethodName);
    m_setPropertyFn = (SetPropertyFn)getManagedFunction(SdkAssemblyName, InteropTypeName, SetPropertyMethodName);
    m_debugListFn = (DebugListFn)getManagedFunction(SdkAssemblyName, InteropTypeName, DebugListMethodName);
    m_invokeMethodFn = (InvokeMethodFn)getManagedFunction(SdkAssemblyName, InteropTypeName, InvokeMethodName);
    m_dispatchCollisionEventFn = (DispatchCollisionEventFn)getManagedFunction(SdkAssemblyName, InteropTypeName,
                                                                              DispatchCollisionEventMethodName);
    m_callOnDisableFn = (CallOnDisableFn)getManagedFunction(SdkAssemblyName, InteropTypeName,
                                                            CallOnDisableMethodName);
    m_callOnEnableFn = (CallOnEnableFn)getManagedFunction(SdkAssemblyName, InteropTypeName,
                                                          CallOnEnableMethodName);
    if (!m_createInstanceFn || !m_destroyInstanceFn || !m_updateInstanceFn || !m_setPropertyFn ||
        !m_debugListFn || !m_invokeMethodFn || !m_onCreateFn || !m_dispatchCollisionEventFn || !
        m_callOnDisableFn || !m_callOnEnableFn)
    {
        LogError("CoreCLRHost: 缓存一个或多个 C# 互操作方法失败。");
        Shutdown();
        return false;
    }

    LogInfo("CoreCLRHost: 宿主初始化成功。");
    m_isInitialized = true;
    return true;
}

bool CoreCLRHost::bootstrapRuntimeIfNeeded(const std::filesystem::path& sdkDir)
{
    if (s_runtimeBootstrapped) return true;

    char_t buffer[1024];
    size_t buffer_size = sizeof(buffer) / sizeof(char_t);
    if (get_hostfxr_path(buffer, &buffer_size, nullptr) != 0)
    {
        LogError("CoreCLRHost: 获取 hostfxr 路径失败。");
        return false;
    }
    s_hostfxrLibrary = LOAD_LIBRARY(buffer);
    if (!s_hostfxrLibrary)
    {
        LogError("CoreCLRHost: 加载 hostfxr 库失败。");
        return false;
    }

    auto init_fptr = (hostfxr_initialize_for_runtime_config_fn)GET_PROC_ADDRESS(
        s_hostfxrLibrary, "hostfxr_initialize_for_runtime_config");
    auto get_delegate_fptr = (hostfxr_get_runtime_delegate_fn)GET_PROC_ADDRESS(
        s_hostfxrLibrary, "hostfxr_get_runtime_delegate");
    auto close_fptr = (hostfxr_close_fn)GET_PROC_ADDRESS(s_hostfxrLibrary, "hostfxr_close");
    if (!init_fptr || !get_delegate_fptr || !close_fptr)
    {
        LogError("CoreCLRHost: 加载必需的 hostfxr 函数失败。");
        return false;
    }

    const auto sdkConfigPath = sdkDir / (std::wstring(L"Luma.SDK") + L".runtimeconfig.json");
    if (!std::filesystem::exists(sdkConfigPath))
    {
        LogError("CoreCLRHost: 运行时配置文件未找到：{}", sdkConfigPath.string());
        return false;
    }

    hostfxr_handle runtimeContext = nullptr;
    if (init_fptr(sdkConfigPath.c_str(), nullptr, &runtimeContext) != 0 || runtimeContext == nullptr)
    {
        LogError("CoreCLRHost: 初始化 CoreCLR 运行时上下文失败。配置：{}", sdkConfigPath.string());
        close_fptr(runtimeContext);
        return false;
    }

    if (get_delegate_fptr(runtimeContext, hdt_get_function_pointer, (void**)&s_sharedGetFunctionPtrFn) != 0 || !
        s_sharedGetFunctionPtrFn)
    {
        LogError("CoreCLRHost: 获取 get_function_pointer 委托失败。");
        close_fptr(runtimeContext);
        return false;
    }
    if (get_delegate_fptr(runtimeContext, hdt_load_assembly, (void**)&s_sharedLoadAssemblyFn) != 0 || !
        s_sharedLoadAssemblyFn)
    {
        LogError("CoreCLRHost: 获取 load_assembly 委托失败。");
        close_fptr(runtimeContext);
        return false;
    }

    close_fptr(runtimeContext);

    std::filesystem::path sdkAssemblyPath = sdkDir / (std::wstring(L"Luma.SDK") + L".dll");
    if (s_sharedLoadAssemblyFn(sdkAssemblyPath.c_str(), nullptr, nullptr) != 0)
    {
        LogError("CoreCLRHost: 加载 SDK 程序集失败：{}", sdkAssemblyPath.string());
        return false;
    }

    s_runtimeBootstrapped = true;
    return true;
}


std::filesystem::path CoreCLRHost::createNewShadowCopyDirectory()
{
    std::error_code fsEc;

    if (!std::filesystem::exists(m_shadowCopyBasePath, fsEc))
    {
        std::filesystem::create_directories(m_shadowCopyBasePath, fsEc);
        if (fsEc)
        {
            LogError("CoreCLRHost: 创建影子副本基础目录失败：{}", fsEc.message());
            return {};
        }
    }


    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);
    std::tm buf{};
#ifdef _WIN32
    localtime_s(&buf, &in_time_t);
#else
    localtime_r(&in_time_t, &buf);
#endif
    auto since_epoch = now.time_since_epoch();
    auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(since_epoch).count() % 1000;

    std::wstringstream ss;
    ss << std::put_time(&buf, L"%Y%m%d_%H%M%S") << L"_" << std::setw(3) << std::setfill(L'0') << millis;

    std::filesystem::path newShadowPath = m_shadowCopyBasePath / ss.str();

    std::filesystem::create_directories(newShadowPath, fsEc);
    if (fsEc)
    {
        LogError("CoreCLRHost: 创建影子副本目录失败：{}", fsEc.message());
        return {};
    }

    return newShadowPath;
}

bool CoreCLRHost::copyAssembliesToShadowDirectory(const std::filesystem::path& sourceDir,
                                                  const std::filesystem::path& targetDir)
{
    std::error_code fsEc;
    for (auto it = std::filesystem::recursive_directory_iterator(sourceDir, fsEc);
         it != std::filesystem::recursive_directory_iterator(); it.increment(fsEc))
    {
        if (fsEc)
        {
            LogError("CoreCLRHost: 遍历源目录失败：{}", fsEc.message());
            return false;
        }

        const auto& srcPath = it->path();
        auto rel = std::filesystem::relative(srcPath, sourceDir, fsEc);
        if (fsEc) rel = srcPath.filename();

        auto dstPath = targetDir / rel;

        if (it->is_directory())
        {
            std::filesystem::create_directories(dstPath, fsEc);
            if (fsEc)
            {
                LogError("CoreCLRHost: 创建目录失败：{} -> {}", dstPath.string(), fsEc.message());
                return false;
            }
            continue;
        }

        const std::wstring fileName = srcPath.filename().wstring();
        const bool isLumaSdk =
            fileName == L"Luma.SDK.dll" ||
            fileName == L"Luma.SDK.pdb" ||
            fileName == L"Luma.SDK.runtimeconfig.json" ||
            fileName == L"Luma.SDK.deps.json";

        if (isLumaSdk)
            continue;

        std::filesystem::create_directories(dstPath.parent_path(), fsEc);
        fsEc.clear();
        std::filesystem::copy_file(srcPath, dstPath,
                                   std::filesystem::copy_options::overwrite_existing,
                                   fsEc);
        if (fsEc)
        {
            LogError("CoreCLRHost: 复制文件失败：{} -> {}. 错误: {}",
                     srcPath.string(), dstPath.string(), fsEc.message());
            return false;
        }
    }

    return true;
}

void CoreCLRHost::cleanupOldShadowCopies()
{
    std::error_code fsEc;
    if (std::filesystem::exists(m_shadowCopyBasePath, fsEc))
    {
        std::filesystem::remove_all(m_shadowCopyBasePath, fsEc);
        if (fsEc)
            LogWarn("CoreCLRHost: 清理临时目录失败：{}", fsEc.message());
    }
}

void CoreCLRHost::Shutdown()
{
    if (m_unloadDomainFn)
    {
        m_unloadDomainFn();
    }

    m_initializeDomainFn = nullptr;
    m_unloadDomainFn = nullptr;
    m_createInstanceFn = nullptr;
    m_destroyInstanceFn = nullptr;
    m_updateInstanceFn = nullptr;
    m_setPropertyFn = nullptr;
    m_debugListFn = nullptr;
    m_invokeMethodFn = nullptr;
    m_onCreateFn = nullptr;
    m_dispatchCollisionEventFn = nullptr;
    m_callOnDisableFn = nullptr;
    m_callOnEnableFn = nullptr;
    if (!m_activeShadowCopyPath.empty())
    {
        std::error_code fsEc;
        std::filesystem::remove_all(m_activeShadowCopyPath, fsEc);
        if (fsEc)
        {
            LogWarn("CoreCLRHost: 删除影子副本目录失败：{} -> {}", m_activeShadowCopyPath.string(), fsEc.message());
        }
    }

    m_activeShadowCopyPath.clear();
    m_scriptAssembliesPath.clear();
    m_isInitialized = false;

    LogInfo("CoreCLRHost: 已完全关闭。");
}

void* CoreCLRHost::getManagedFunction(
    const std::wstring& assemblyName,
    const std::wstring& typeName,
    const std::wstring& methodName
) const
{
    if (!s_sharedGetFunctionPtrFn)
    {
        LogError("CoreCLRHost: s_sharedGetFunctionPtrFn is null");
        return nullptr;
    }

    void* delegate_ptr = nullptr;
    const std::wstring qualifiedTypeNameW = typeName + L", " + assemblyName;

#if defined(_WIN32)
    const wchar_t* typeNameNative = qualifiedTypeNameW.c_str();
    const wchar_t* methodNameNative = methodName.c_str();
#else
    std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> converter;
    std::string qualifiedTypeNameUtf8 = converter.to_bytes(qualifiedTypeNameW);
    std::string methodNameUtf8 = converter.to_bytes(methodName);

    const char* typeNameNative = qualifiedTypeNameUtf8.c_str();
    const char* methodNameNative = methodNameUtf8.c_str();
#endif

    const int rc = s_sharedGetFunctionPtrFn(
        typeNameNative,
        methodNameNative,
        UNMANAGEDCALLERSONLY_METHOD,
        nullptr,
        nullptr,
        &delegate_ptr
    );

    if (rc != 0)
    {
        std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> converter;
        LogError("CoreCLRHost: Failed to get managed function '{}' from type '{}'. Error code: {:#0x}",
                 converter.to_bytes(methodName),
                 converter.to_bytes(qualifiedTypeNameW),
                 rc);
        return nullptr;
    }

    if (delegate_ptr == nullptr)
    {
        std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> converter;
        LogError("CoreCLRHost: The function pointer retrieved is null for function '{}'",
                 converter.to_bytes(methodName));
        return nullptr;
    }

    return delegate_ptr;
}

std::string CoreCLRHost::pathToUtf8(const std::filesystem::path& p)
{
#ifdef _WIN32
    const std::wstring w = p.wstring();
    if (w.empty()) return {};
    int len = WideCharToMultiByte(CP_UTF8, 0, w.c_str(), (int)w.size(), nullptr, 0, nullptr, nullptr);
    std::string out;
    out.resize(len);
    WideCharToMultiByte(CP_UTF8, 0, w.c_str(), (int)w.size(), out.data(), len, nullptr, nullptr);
    return out;
#else
    return p.string();
#endif
}
