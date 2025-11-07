#include "MonoHost.h"
#ifdef __ANDROID__
#include <iostream>
#include <chrono>
#include <iomanip>
#include <sstream>
#include "../Utils/Directory.h"
#include "../Utils/PCH.h"
#include "../Application/ProjectSettings.h"
#include <jni.h>
#include <android/log.h>
#include <pthread.h>

#include <mono/jit/jit.h>
#include <mono/metadata/assembly.h>
#include <mono/metadata/object.h>
#include <mono/metadata/environment.h>
#include <mono/metadata/mono-config.h>
#include <mono/metadata/threads.h>
#include <mono/utils/mono-logger.h>
#include <mono/metadata/mono-debug.h>

std::mutex MonoHost::s_mutex;

MonoHost* MonoHost::GetInstance()
{
    return s_instance.get();
}

void MonoHost::CreateNewInstance()
{
    std::lock_guard<std::mutex> lock(s_mutex);

    if (s_instance)
    {
        LogInfo("MonoHost: 销毁旧的单例实例");
        s_instance->Shutdown();
        s_instance.reset();
    }

    s_instance = std::unique_ptr<MonoHost>(new MonoHost());
}

void MonoHost::DestroyInstance()
{
    std::lock_guard<std::mutex> lock(s_mutex);

    if (s_instance)
    {
        s_instance->Shutdown();
        s_instance.reset();
        s_instance = nullptr;
    }
}

bool MonoHost::Initialize(const std::filesystem::path& mainAssemblyPath, bool isEditorMode)
{
    if (m_isInitialized)
    {
        LogInfo("MonoHost: 检测到重复初始化请求，执行完全关闭以便重新加载托管域");
        Shutdown();
    }

    m_isEditorMode = isEditorMode;
    const std::filesystem::path sourceDir = mainAssemblyPath.parent_path();
    std::filesystem::path effectiveAssemblyDir = sourceDir;

    if (!bootstrapRuntimeIfNeeded(sourceDir))
    {
        LogError("MonoHost: 引导 Mono 运行时失败");
        return false;
    }

    m_scriptAssembliesPath = Directory::GetAbsolutePath(effectiveAssemblyDir.string());
    LogInfo("MonoHost: 脚本程序集路径: {}", m_scriptAssembliesPath.string());

    
    if (!initializeDelegates())
    {
        LogError("MonoHost: 初始化委托失败");
        return false;
    }

    
    if (m_initializeDomainFn)
    {
        std::string pathUtf8 = pathToUtf8(m_scriptAssembliesPath);
        LogInfo("MonoHost: 调用 InitializeDomain，路径: {}", pathUtf8);

        try
        {
            m_initializeDomainFn(pathUtf8.c_str());
            LogInfo("MonoHost: InitializeDomain 调用成功");
        }
        catch (...)
        {
            LogError("MonoHost: InitializeDomain 调用时发生异常");
            return false;
        }
    }
    else
    {
        LogError("MonoHost: InitializeDomainFn 未初始化");
        return false;
    }

    LogInfo("MonoHost: 宿主初始化成功");
    m_isInitialized = true;
    return true;
}

bool MonoHost::bootstrapRuntimeIfNeeded(const std::filesystem::path& sdkDir)
{
    if (m_monoDomain)
    {
        LogInfo("MonoHost: Mono 域已初始化，跳过引导");
        return true;
    }

    
    std::string appDataDir = "/data/data/com.lumaengine.lumaandroid/files";
    std::filesystem::path bclPath = std::filesystem::path(appDataDir) / "mono-bcl" / "net9.0";

    
    std::error_code ec;
    if (!std::filesystem::exists(bclPath, ec))
    {
        LogError("MonoHost: BCL 目录不存在: {}", bclPath.string());
        return false;
    }

    
    std::filesystem::path corlibPath = bclPath / "System.Private.CoreLib.dll";
    if (!std::filesystem::exists(corlibPath, ec))
    {
        LogError("MonoHost: 核心库不存在: {}", corlibPath.string());
        return false;
    }

    LogInfo("MonoHost: 找到 BCL 路径: {}", bclPath.string());
    LogInfo("MonoHost: 找到核心库: {}", corlibPath.string());

    
    if (ProjectSettings::GetInstance().GetScriptDebugEnabled())
    {
        const char* monoOptions[] = {
            "--soft-breakpoints",
            "--debugger-agent=transport=dt_socket,address=127.0.0.1:55555,server=y,suspend=n"
        };
        mono_jit_parse_options(2, const_cast<char**>(monoOptions));
        mono_debug_init(MONO_DEBUG_FORMAT_MONO);
        LogInfo("MonoHost: Mono 调试模式已启用，监听端口 55555");
    }

    
    std::string bclPathUtf8 = pathToUtf8(bclPath);
    setenv("MONO_PATH", bclPathUtf8.c_str(), 1);
    LogInfo("MonoHost: 已设置 MONO_PATH: {}", bclPathUtf8);

    
    mono_set_assemblies_path(bclPathUtf8.c_str());
    LogInfo("MonoHost: 已设置程序集搜索路径: {}", bclPathUtf8);

    
    m_monoDomain = mono_jit_init("LumaMonoDomain");
    if (!m_monoDomain)
    {
        LogError("MonoHost: 初始化 Mono JIT 失败");
        return false;
    }

    LogInfo("MonoHost: Mono 域初始化成功");

    
    mono_thread_attach(m_monoDomain);

    
    std::filesystem::path sdkAssemblyPath = sdkDir / "Luma.SDK.dll";
    if (!std::filesystem::exists(sdkAssemblyPath, ec))
    {
        LogError("MonoHost: SDK 程序集不存在: {}", sdkAssemblyPath.string());
        return false;
    }

    std::string sdkAssemblyPathUtf8 = pathToUtf8(sdkAssemblyPath);
    LogInfo("MonoHost: 尝试加载 SDK 程序集: {}", sdkAssemblyPathUtf8);

    m_sdkAssembly = mono_domain_assembly_open(m_monoDomain, sdkAssemblyPathUtf8.c_str());
    if (!m_sdkAssembly)
    {
        LogError("MonoHost: 加载 SDK 程序集失败: {}", sdkAssemblyPathUtf8);
        return false;
    }

    m_sdkImage = mono_assembly_get_image(m_sdkAssembly);
    if (!m_sdkImage)
    {
        LogError("MonoHost: 无法获取 SDK 程序集镜像");
        return false;
    }

    LogInfo("MonoHost: SDK 程序集加载成功: {}", sdkAssemblyPathUtf8);
    return true;
}

void MonoHost::Shutdown()
{
    
    if (m_unloadDomainFn)
    {
        try
        {
            m_unloadDomainFn();
            LogInfo("MonoHost: UnloadDomain 调用成功");
        }
        catch (...)
        {
            LogWarn("MonoHost: UnloadDomain 调用时发生异常");
        }
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

    
    m_sdkAssembly = nullptr;
    m_sdkImage = nullptr;
    m_methodCache.clear();

    
    if (m_monoDomain)
    {
        m_monoDomain = nullptr;
        LogInfo("MonoHost: Mono 域已清理");
    }

    m_scriptAssembliesPath.clear();
    m_isInitialized = false;

    LogInfo("MonoHost: 已完全关闭");
}

MonoMethod* MonoHost::getManagedMethod(
    const std::wstring& assemblyName,
    const std::wstring& typeName,
    const std::wstring& methodName
) const
{
    if (!m_sdkImage)
    {
        LogError("MonoHost: SDK 程序集镜像未初始化");
        return nullptr;
    }

    
    std::string namespaceUtf8 = "Luma.SDK";
    std::string classUtf8 = "Interop";
    std::string methodUtf8 = wideToUtf8(methodName);

    
    std::string cacheKey = namespaceUtf8 + "::" + classUtf8 + "::" + methodUtf8;

    
    auto it = m_methodCache.find(cacheKey);
    if (it != m_methodCache.end())
    {
        return it->second;
    }

    
    MonoClass* klass = mono_class_from_name(m_sdkImage, namespaceUtf8.c_str(), classUtf8.c_str());
    if (!klass)
    {
        LogError("MonoHost: 无法找到类 {}.{}", namespaceUtf8, classUtf8);
        return nullptr;
    }

    
    MonoMethod* method = mono_class_get_method_from_name(klass, methodUtf8.c_str(), -1);
    if (!method)
    {
        LogError("MonoHost: 无法找到方法 {}.{}.{}", namespaceUtf8, classUtf8, methodUtf8);
        return nullptr;
    }

    
    m_methodCache[cacheKey] = method;
    return method;
}

bool MonoHost::checkAndLogException(MonoObject* exception) const
{
    if (!exception) return false;

    MonoClass* exceptionClass = mono_object_get_class(exception);
    const char* exceptionName = mono_class_get_name(exceptionClass);

    
    MonoProperty* messageProp = mono_class_get_property_from_name(exceptionClass, "Message");
    if (messageProp)
    {
        MonoMethod* getter = mono_property_get_get_method(messageProp);
        MonoObject* messageObj = mono_runtime_invoke(getter, exception, nullptr, nullptr);
        if (messageObj)
        {
            char* message = mono_string_to_utf8((MonoString*)messageObj);
            LogError("MonoHost: 托管异常 [{}]: {}", exceptionName, message);
            mono_free(message);
        }
    }
    else
    {
        LogError("MonoHost: 托管异常 [{}]", exceptionName);
    }

    
    MonoProperty* stackTraceProp = mono_class_get_property_from_name(exceptionClass, "StackTrace");
    if (stackTraceProp)
    {
        MonoMethod* getter = mono_property_get_get_method(stackTraceProp);
        MonoObject* stackTraceObj = mono_runtime_invoke(getter, exception, nullptr, nullptr);
        if (stackTraceObj)
        {
            char* stackTrace = mono_string_to_utf8((MonoString*)stackTraceObj);
            LogError("MonoHost: 堆栈跟踪:\n{}", stackTrace);
            mono_free(stackTrace);
        }
    }

    return true;
}



void MonoHost::wrapperInitializeDomain(const char* baseDirUtf8)
{
    auto* instance = GetInstance();
    if (!instance || !instance->m_sdkImage || !instance->m_monoDomain) return;

    MonoMethod* method = instance->getManagedMethod(L"Luma.SDK", L"Luma.SDK.Interop", L"InitializeDomain");
    if (!method) return;

    
    void* pathPtr = (void*)baseDirUtf8;
    void* args[1];
    args[0] = &pathPtr;

    MonoObject* exception = nullptr;
    mono_runtime_invoke(method, nullptr, args, &exception);
    instance->checkAndLogException(exception);
}

void MonoHost::wrapperUnloadDomain()
{
    auto* instance = GetInstance();
    if (!instance || !instance->m_sdkImage) return;

    MonoMethod* method = instance->getManagedMethod(L"Luma.SDK", L"Luma.SDK.Interop", L"UnloadDomain");
    if (!method) return;

    MonoObject* exception = nullptr;
    mono_runtime_invoke(method, nullptr, nullptr, &exception);
    instance->checkAndLogException(exception);
}

ManagedGCHandle MonoHost::wrapperCreateInstance(void* scenePtr, uint32_t entityId, const char* typeName,
                                                const char* assemblyName)
{
    auto* instance = GetInstance();
    if (!instance || !instance->m_sdkImage || !instance->m_monoDomain) return 0;

    MonoMethod* method = instance->getManagedMethod(L"Luma.SDK", L"Luma.SDK.Interop", L"CreateScriptInstance");
    if (!method) return 0;

    
    void* typeNamePtr = (void*)typeName;
    void* assemblyNamePtr = (void*)assemblyName;

    void* args[4];
    args[0] = &scenePtr;           
    args[1] = &entityId;           
    args[2] = &typeNamePtr;        
    args[3] = &assemblyNamePtr;    

    MonoObject* exception = nullptr;
    MonoObject* result = mono_runtime_invoke(method, nullptr, args, &exception);

    if (instance->checkAndLogException(exception))
    {
        return 0;
    }

    if (!result) return 0;

    
    return *(intptr_t*)mono_object_unbox(result);
}

void MonoHost::wrapperOnCreate(ManagedGCHandle handle)
{
    auto* instance = GetInstance();
    if (!instance || !instance->m_sdkImage) return;

    MonoMethod* method = instance->getManagedMethod(L"Luma.SDK", L"Luma.SDK.Interop", L"OnCreate");
    if (!method) return;

    void* handlePtr = (void*)handle;
    void* args[1];
    args[0] = &handlePtr;

    MonoObject* exception = nullptr;
    mono_runtime_invoke(method, nullptr, args, &exception);
    instance->checkAndLogException(exception);
}

void MonoHost::wrapperDestroyInstance(ManagedGCHandle handle)
{
    auto* instance = GetInstance();
    if (!instance || !instance->m_sdkImage) return;

    MonoMethod* method = instance->getManagedMethod(L"Luma.SDK", L"Luma.SDK.Interop", L"DestroyScriptInstance");
    if (!method) return;

    void* handlePtr = (void*)handle;
    void* args[1];
    args[0] = &handlePtr;

    MonoObject* exception = nullptr;
    mono_runtime_invoke(method, nullptr, args, &exception);
    instance->checkAndLogException(exception);
}

void MonoHost::wrapperUpdateInstance(ManagedGCHandle handle, float deltaTime)
{
    auto* instance = GetInstance();
    if (!instance || !instance->m_sdkImage) return;

    MonoMethod* method = instance->getManagedMethod(L"Luma.SDK", L"Luma.SDK.Interop", L"InvokeUpdate");
    if (!method) return;

    void* handlePtr = (void*)handle;
    void* args[2];
    args[0] = &handlePtr;
    args[1] = &deltaTime;

    MonoObject* exception = nullptr;
    mono_runtime_invoke(method, nullptr, args, &exception);
    instance->checkAndLogException(exception);
}

void MonoHost::wrapperSetProperty(ManagedGCHandle handle, const char* propName, const char* valueAsYaml)
{
    auto* instance = GetInstance();
    if (!instance || !instance->m_sdkImage || !instance->m_monoDomain) return;

    MonoMethod* method = instance->getManagedMethod(L"Luma.SDK", L"Luma.SDK.Interop", L"SetExportedProperty");
    if (!method) return;

    
    void* handlePtr = (void*)handle;
    void* propNamePtr = (void*)propName;
    void* valuePtr = (void*)valueAsYaml;

    void* args[3];
    args[0] = &handlePtr;
    args[1] = &propNamePtr;
    args[2] = &valuePtr;

    MonoObject* exception = nullptr;
    mono_runtime_invoke(method, nullptr, args, &exception);
    instance->checkAndLogException(exception);
}

void MonoHost::wrapperDebugList(const char* assemblyPath)
{
    auto* instance = GetInstance();
    if (!instance || !instance->m_sdkImage || !instance->m_monoDomain) return;

    MonoMethod* method = instance->getManagedMethod(L"Luma.SDK", L"Luma.SDK.Interop", L"Debug_ListAllTypesAndMethods");
    if (!method) return;

    
    void* pathPtr = (void*)assemblyPath;
    void* args[1];
    args[0] = &pathPtr;

    MonoObject* exception = nullptr;
    mono_runtime_invoke(method, nullptr, args, &exception);
    instance->checkAndLogException(exception);
}

void MonoHost::wrapperInvokeMethod(ManagedGCHandle handle, const char* methodName, const char* argsAsYaml)
{
    auto* instance = GetInstance();
    if (!instance || !instance->m_sdkImage || !instance->m_monoDomain) return;

    MonoMethod* method = instance->getManagedMethod(L"Luma.SDK", L"Luma.SDK.Interop", L"InvokeMethod");
    if (!method) return;

    
    void* handlePtr = (void*)handle;
    void* methodNamePtr = (void*)methodName;
    void* argsPtr = (void*)argsAsYaml;

    void* args[3];
    args[0] = &handlePtr;
    args[1] = &methodNamePtr;
    args[2] = &argsPtr;

    MonoObject* exception = nullptr;
    mono_runtime_invoke(method, nullptr, args, &exception);
    instance->checkAndLogException(exception);
}

void MonoHost::wrapperDispatchCollisionEvent(ManagedGCHandle handle, int contactType, uint32_t otherEntityId)
{
    auto* instance = GetInstance();
    if (!instance || !instance->m_sdkImage) return;

    MonoMethod* method = instance->getManagedMethod(L"Luma.SDK", L"Luma.SDK.Interop", L"DispatchCollisionEvent");
    if (!method) return;

    void* handlePtr = (void*)handle;
    void* args[3];
    args[0] = &handlePtr;
    args[1] = &contactType;
    args[2] = &otherEntityId;

    MonoObject* exception = nullptr;
    mono_runtime_invoke(method, nullptr, args, &exception);
    instance->checkAndLogException(exception);
}

void MonoHost::wrapperOnEnable(ManagedGCHandle handle)
{
    auto* instance = GetInstance();
    if (!instance || !instance->m_sdkImage) return;

    MonoMethod* method = instance->getManagedMethod(L"Luma.SDK", L"Luma.SDK.Interop", L"OnEnable");
    if (!method) return;

    void* handlePtr = (void*)handle;
    void* args[1];
    args[0] = &handlePtr;

    MonoObject* exception = nullptr;
    mono_runtime_invoke(method, nullptr, args, &exception);
    instance->checkAndLogException(exception);
}

void MonoHost::wrapperOnDisable(ManagedGCHandle handle)
{
    auto* instance = GetInstance();
    if (!instance || !instance->m_sdkImage) return;

    MonoMethod* method = instance->getManagedMethod(L"Luma.SDK", L"Luma.SDK.Interop", L"OnDisable");
    if (!method) return;

    void* handlePtr = (void*)handle;
    void* args[1];
    args[0] = &handlePtr;

    MonoObject* exception = nullptr;
    mono_runtime_invoke(method, nullptr, args, &exception);
    instance->checkAndLogException(exception);
}

bool MonoHost::initializeDelegates()
{
    LogInfo("MonoHost: 开始初始化委托函数指针");

    
    m_initializeDomainFn = &MonoHost::wrapperInitializeDomain;
    m_unloadDomainFn = &MonoHost::wrapperUnloadDomain;
    m_createInstanceFn = &MonoHost::wrapperCreateInstance;
    m_onCreateFn = &MonoHost::wrapperOnCreate;
    m_destroyInstanceFn = &MonoHost::wrapperDestroyInstance;
    m_updateInstanceFn = &MonoHost::wrapperUpdateInstance;
    m_setPropertyFn = &MonoHost::wrapperSetProperty;
    m_debugListFn = &MonoHost::wrapperDebugList;
    m_invokeMethodFn = &MonoHost::wrapperInvokeMethod;
    m_dispatchCollisionEventFn = &MonoHost::wrapperDispatchCollisionEvent;
    m_callOnEnableFn = &MonoHost::wrapperOnEnable;
    m_callOnDisableFn = &MonoHost::wrapperOnDisable;

    LogInfo("MonoHost: 所有委托函数指针初始化成功");
    return true;
}

std::string MonoHost::pathToUtf8(const std::filesystem::path& p)
{
    return p.string();
}

std::string MonoHost::wideToUtf8(const std::wstring& wstr)
{
    if (wstr.empty()) return std::string();

    std::string result;
    result.reserve(wstr.size() * 3);

    for (wchar_t wc : wstr)
    {
        if (wc < 0x80)
        {
            result.push_back(static_cast<char>(wc));
        }
        else if (wc < 0x800)
        {
            result.push_back(static_cast<char>(0xC0 | ((wc >> 6) & 0x1F)));
            result.push_back(static_cast<char>(0x80 | (wc & 0x3F)));
        }
        else if (wc < 0x10000)
        {
            result.push_back(static_cast<char>(0xE0 | ((wc >> 12) & 0x0F)));
            result.push_back(static_cast<char>(0x80 | ((wc >> 6) & 0x3F)));
            result.push_back(static_cast<char>(0x80 | (wc & 0x3F)));
        }
        else
        {
            result.push_back(static_cast<char>(0xF0 | ((wc >> 18) & 0x07)));
            result.push_back(static_cast<char>(0x80 | ((wc >> 12) & 0x3F)));
            result.push_back(static_cast<char>(0x80 | ((wc >> 6) & 0x3F)));
            result.push_back(static_cast<char>(0x80 | (wc & 0x3F)));
        }
    }

    return result;
}
#endif