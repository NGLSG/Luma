#include "MonoHost.h"
#include <iostream>
#include <chrono>
#include <iomanip>
#include <sstream>
#include "../Utils/Directory.h"
#include "../Utils/PCH.h"
#include "../Application/ProjectSettings.h"

#ifdef __ANDROID__
#include <dlfcn.h>
#include <jni.h>
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

    
    typedef void* (*mono_jit_init_fn)(const char* file);
    typedef void* (*mono_jit_init_version_fn)(const char* root_domain_name, const char* runtime_version);
    typedef void (*mono_jit_cleanup_fn)(void* domain);
    typedef void* (*mono_domain_assembly_open_fn)(void* domain, const char* name);
    typedef void* (*mono_assembly_get_image_fn)(void* assembly);
    typedef void* (*mono_class_from_name_fn)(void* image, const char* name_space, const char* name);
    typedef void* (*mono_class_get_method_from_name_fn)(void* klass, const char* name, int param_count);
    typedef void* (*mono_runtime_invoke_fn)(void* method, void* obj, void** params, void** exc);
    typedef void (*mono_add_internal_call_fn)(const char* name, const void* method);
    typedef void* (*mono_thread_attach_fn)(void* domain);
    typedef void (*mono_thread_detach_fn)(void* thread);
    typedef void* (*mono_get_root_domain_fn)();
    typedef void (*mono_domain_set_config_fn)(void* domain, const char* base_dir, const char* config_file_name);
    typedef void* (*mono_string_new_fn)(void* domain, const char* text);
    typedef char* (*mono_string_to_utf8_fn)(void* string_obj);
    typedef void (*mono_free_fn)(void* ptr);
    typedef void* (*mono_object_new_fn)(void* domain, void* klass);
    typedef void (*mono_runtime_object_init_fn)(void* this_obj);
    typedef uint32_t (*mono_gchandle_new_fn)(void* obj, int pinned);
    typedef void (*mono_gchandle_free_fn)(uint32_t gchandle);
    typedef void* (*mono_gchandle_get_target_fn)(uint32_t gchandle);
    typedef void* (*mono_domain_get_fn)();
    typedef void* (*mono_object_unbox_fn)(void* obj);
    typedef void* (*mono_value_box_fn)(void* domain, void* klass, void* val);
    typedef void (*mono_jit_parse_options_fn)(int argc, char* argv[]);
    typedef void (*mono_debug_init_fn)(int format);

    
    void* s_monoLibHandle = nullptr;

    
    mono_jit_init_fn mono_jit_init = nullptr;
    mono_jit_init_version_fn mono_jit_init_version = nullptr;
    mono_jit_cleanup_fn mono_jit_cleanup = nullptr;
    mono_domain_assembly_open_fn mono_domain_assembly_open = nullptr;
    mono_assembly_get_image_fn mono_assembly_get_image = nullptr;
    mono_class_from_name_fn mono_class_from_name = nullptr;
    mono_class_get_method_from_name_fn mono_class_get_method_from_name = nullptr;
    mono_runtime_invoke_fn mono_runtime_invoke = nullptr;
    mono_add_internal_call_fn mono_add_internal_call = nullptr;
    mono_thread_attach_fn mono_thread_attach = nullptr;
    mono_thread_detach_fn mono_thread_detach = nullptr;
    mono_get_root_domain_fn mono_get_root_domain = nullptr;
    mono_domain_set_config_fn mono_domain_set_config = nullptr;
    mono_string_new_fn mono_string_new = nullptr;
    mono_string_to_utf8_fn mono_string_to_utf8 = nullptr;
    mono_free_fn mono_free = nullptr;
    mono_object_new_fn mono_object_new = nullptr;
    mono_runtime_object_init_fn mono_runtime_object_init = nullptr;
    mono_gchandle_new_fn mono_gchandle_new = nullptr;
    mono_gchandle_free_fn mono_gchandle_free = nullptr;
    mono_gchandle_get_target_fn mono_gchandle_get_target = nullptr;
    mono_domain_get_fn mono_domain_get = nullptr;
    mono_object_unbox_fn mono_object_unbox = nullptr;
    mono_value_box_fn mono_value_box = nullptr;
    mono_jit_parse_options_fn mono_jit_parse_options = nullptr;
    mono_debug_init_fn mono_debug_init = nullptr;

    
    struct MonoMethodContext
    {
        void* method = nullptr;
        void* domain = nullptr;

        MonoMethodContext() = default;
        MonoMethodContext(void* m, void* d) : method(m), domain(d) {}
    };

    
    std::unordered_map<std::string, MonoMethodContext> s_methodContextCache;

    
    void* CreateMonoString(const char* text)
    {
        void* domain = mono_domain_get ? mono_domain_get() : nullptr;
        if (!domain || !mono_string_new)
        {
            LogError("MonoHost: 无法创建MonoString，域或函数指针为空");
            return nullptr;
        }
        return mono_string_new(domain, text ? text : "");
    }

    
    intptr_t UnboxIntPtr(void* obj)
    {
        if (!obj || !mono_object_unbox)
        {
            return 0;
        }
        void* unboxed = mono_object_unbox(obj);
        if (!unboxed)
        {
            return 0;
        }
        return *static_cast<intptr_t*>(unboxed);
    }

    
    void* BoxIntPtr(intptr_t value)
    {
        void* domain = mono_domain_get ? mono_domain_get() : nullptr;
        if (!domain || !mono_value_box)
        {
            return nullptr;
        }

        
        void* mscorlib = mono_domain_assembly_open(domain, "mscorlib");
        if (!mscorlib)
        {
            return nullptr;
        }
        void* image = mono_assembly_get_image(mscorlib);
        if (!image)
        {
            return nullptr;
        }
        void* intPtrClass = mono_class_from_name(image, "System", "IntPtr");
        if (!intPtrClass)
        {
            return nullptr;
        }

        return mono_value_box(domain, intPtrClass, &value);
    }

    
    void* BoxUInt32(uint32_t value)
    {
        void* domain = mono_domain_get ? mono_domain_get() : nullptr;
        if (!domain || !mono_value_box)
        {
            return nullptr;
        }

        void* mscorlib = mono_domain_assembly_open(domain, "mscorlib");
        if (!mscorlib)
        {
            return nullptr;
        }
        void* image = mono_assembly_get_image(mscorlib);
        if (!image)
        {
            return nullptr;
        }
        void* uint32Class = mono_class_from_name(image, "System", "UInt32");
        if (!uint32Class)
        {
            return nullptr;
        }

        return mono_value_box(domain, uint32Class, &value);
    }

    
    void* BoxInt32(int value)
    {
        void* domain = mono_domain_get ? mono_domain_get() : nullptr;
        if (!domain || !mono_value_box)
        {
            return nullptr;
        }

        void* mscorlib = mono_domain_assembly_open(domain, "mscorlib");
        if (!mscorlib)
        {
            return nullptr;
        }
        void* image = mono_assembly_get_image(mscorlib);
        if (!image)
        {
            return nullptr;
        }
        void* int32Class = mono_class_from_name(image, "System", "Int32");
        if (!int32Class)
        {
            return nullptr;
        }

        return mono_value_box(domain, int32Class, &value);
    }

    
    void* BoxFloat(float value)
    {
        void* domain = mono_domain_get ? mono_domain_get() : nullptr;
        if (!domain || !mono_value_box)
        {
            return nullptr;
        }

        void* mscorlib = mono_domain_assembly_open(domain, "mscorlib");
        if (!mscorlib)
        {
            return nullptr;
        }
        void* image = mono_assembly_get_image(mscorlib);
        if (!image)
        {
            return nullptr;
        }
        void* floatClass = mono_class_from_name(image, "System", "Single");
        if (!floatClass)
        {
            return nullptr;
        }

        return mono_value_box(domain, floatClass, &value);
    }

    

    
    void LUMA_CALLBACK Wrapper_InitializeDomain(const char* baseDirUtf8)
    {
        auto it = s_methodContextCache.find("InitializeDomain");
        if (it == s_methodContextCache.end())
        {
            LogError("MonoHost: 未找到InitializeDomain方法上下文");
            return;
        }

        void* params[1];
        params[0] = CreateMonoString(baseDirUtf8);

        void* exception = nullptr;
        mono_runtime_invoke(it->second.method, nullptr, params, &exception);

        if (exception)
        {
            LogError("MonoHost: 调用InitializeDomain时发生异常");
        }
    }

    
    void LUMA_CALLBACK Wrapper_UnloadDomain()
    {
        auto it = s_methodContextCache.find("UnloadDomain");
        if (it == s_methodContextCache.end())
        {
            LogError("MonoHost: 未找到UnloadDomain方法上下文");
            return;
        }

        void* exception = nullptr;
        mono_runtime_invoke(it->second.method, nullptr, nullptr, &exception);

        if (exception)
        {
            LogError("MonoHost: 调用UnloadDomain时发生异常");
        }
    }

    
    ManagedGCHandle LUMA_CALLBACK Wrapper_CreateInstance(void* scenePtr, uint32_t entityId,
                                                         const char* typeName, const char* assemblyName)
    {
        auto it = s_methodContextCache.find("CreateScriptInstance");
        if (it == s_methodContextCache.end())
        {
            LogError("MonoHost: 未找到CreateScriptInstance方法上下文");
            return 0;
        }

        void* params[4];
        params[0] = BoxIntPtr(reinterpret_cast<intptr_t>(scenePtr));
        params[1] = BoxUInt32(entityId);
        params[2] = CreateMonoString(typeName);
        params[3] = CreateMonoString(assemblyName);

        void* exception = nullptr;
        void* result = mono_runtime_invoke(it->second.method, nullptr, params, &exception);

        if (exception)
        {
            LogError("MonoHost: 调用CreateScriptInstance时发生异常");
            return 0;
        }

        if (!result)
        {
            return 0;
        }

        return UnboxIntPtr(result);
    }

    
    void LUMA_CALLBACK Wrapper_DestroyInstance(ManagedGCHandle handle)
    {
        auto it = s_methodContextCache.find("DestroyScriptInstance");
        if (it == s_methodContextCache.end())
        {
            LogError("MonoHost: 未找到DestroyScriptInstance方法上下文");
            return;
        }

        void* params[1];
        params[0] = BoxIntPtr(handle);

        void* exception = nullptr;
        mono_runtime_invoke(it->second.method, nullptr, params, &exception);

        if (exception)
        {
            LogError("MonoHost: 调用DestroyScriptInstance时发生异常");
        }
    }

    
    void LUMA_CALLBACK Wrapper_UpdateInstance(ManagedGCHandle handle, float deltaTime)
    {
        auto it = s_methodContextCache.find("InvokeUpdate");
        if (it == s_methodContextCache.end())
        {
            LogError("MonoHost: 未找到InvokeUpdate方法上下文");
            return;
        }

        void* params[2];
        params[0] = BoxIntPtr(handle);
        params[1] = BoxFloat(deltaTime);

        void* exception = nullptr;
        mono_runtime_invoke(it->second.method, nullptr, params, &exception);

        if (exception)
        {
            LogError("MonoHost: 调用InvokeUpdate时发生异常");
        }
    }

    
    void LUMA_CALLBACK Wrapper_SetProperty(ManagedGCHandle handle, const char* propName, const char* valueAsYaml)
    {
        auto it = s_methodContextCache.find("SetExportedProperty");
        if (it == s_methodContextCache.end())
        {
            LogError("MonoHost: 未找到SetExportedProperty方法上下文");
            return;
        }

        void* params[3];
        params[0] = BoxIntPtr(handle);
        params[1] = CreateMonoString(propName);
        params[2] = CreateMonoString(valueAsYaml);

        void* exception = nullptr;
        mono_runtime_invoke(it->second.method, nullptr, params, &exception);

        if (exception)
        {
            LogError("MonoHost: 调用SetExportedProperty时发生异常");
        }
    }

    
    void LUMA_CALLBACK Wrapper_DebugList(const char* assemblyPath)
    {
        auto it = s_methodContextCache.find("Debug_ListAllTypesAndMethods");
        if (it == s_methodContextCache.end())
        {
            LogError("MonoHost: 未找到Debug_ListAllTypesAndMethods方法上下文");
            return;
        }

        void* params[1];
        params[0] = CreateMonoString(assemblyPath);

        void* exception = nullptr;
        mono_runtime_invoke(it->second.method, nullptr, params, &exception);

        if (exception)
        {
            LogError("MonoHost: 调用Debug_ListAllTypesAndMethods时发生异常");
        }
    }

    
    void LUMA_CALLBACK Wrapper_InvokeMethod(ManagedGCHandle handle, const char* methodName, const char* argsAsYaml)
    {
        auto it = s_methodContextCache.find("InvokeMethod");
        if (it == s_methodContextCache.end())
        {
            LogError("MonoHost: 未找到InvokeMethod方法上下文");
            return;
        }

        void* params[3];
        params[0] = BoxIntPtr(handle);
        params[1] = CreateMonoString(methodName);
        params[2] = CreateMonoString(argsAsYaml);

        void* exception = nullptr;
        mono_runtime_invoke(it->second.method, nullptr, params, &exception);

        if (exception)
        {
            LogError("MonoHost: 调用InvokeMethod时发生异常");
        }
    }

    
    void LUMA_CALLBACK Wrapper_OnCreate(ManagedGCHandle handle)
    {
        auto it = s_methodContextCache.find("OnCreate");
        if (it == s_methodContextCache.end())
        {
            LogError("MonoHost: 未找到OnCreate方法上下文");
            return;
        }

        void* params[1];
        params[0] = BoxIntPtr(handle);

        void* exception = nullptr;
        mono_runtime_invoke(it->second.method, nullptr, params, &exception);

        if (exception)
        {
            LogError("MonoHost: 调用OnCreate时发生异常");
        }
    }

    
    void LUMA_CALLBACK Wrapper_DispatchCollisionEvent(ManagedGCHandle handlePtr, int contactType, uint32_t otherEntityId)
    {
        auto it = s_methodContextCache.find("DispatchCollisionEvent");
        if (it == s_methodContextCache.end())
        {
            LogError("MonoHost: 未找到DispatchCollisionEvent方法上下文");
            return;
        }

        void* params[3];
        params[0] = BoxIntPtr(handlePtr);
        params[1] = BoxInt32(contactType);
        params[2] = BoxUInt32(otherEntityId);

        void* exception = nullptr;
        mono_runtime_invoke(it->second.method, nullptr, params, &exception);

        if (exception)
        {
            LogError("MonoHost: 调用DispatchCollisionEvent时发生异常");
        }
    }

    
    void LUMA_CALLBACK Wrapper_CallOnEnable(ManagedGCHandle handle)
    {
        auto it = s_methodContextCache.find("OnEnable");
        if (it == s_methodContextCache.end())
        {
            LogError("MonoHost: 未找到OnEnable方法上下文");
            return;
        }

        void* params[1];
        params[0] = BoxIntPtr(handle);

        void* exception = nullptr;
        mono_runtime_invoke(it->second.method, nullptr, params, &exception);

        if (exception)
        {
            LogError("MonoHost: 调用OnEnable时发生异常");
        }
    }

    
    void LUMA_CALLBACK Wrapper_CallOnDisable(ManagedGCHandle handle)
    {
        auto it = s_methodContextCache.find("OnDisable");
        if (it == s_methodContextCache.end())
        {
            LogError("MonoHost: 未找到OnDisable方法上下文");
            return;
        }

        void* params[1];
        params[0] = BoxIntPtr(handle);

        void* exception = nullptr;
        mono_runtime_invoke(it->second.method, nullptr, params, &exception);

        if (exception)
        {
            LogError("MonoHost: 调用OnDisable时发生异常");
        }
    }
}



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
    m_scriptAssembliesPath = Directory::GetAbsolutePath(mainAssemblyPath.parent_path().string());

    
    if (!loadMonoRuntime())
    {
        LogError("MonoHost: 加载Mono运行时库失败");
        return false;
    }

    
    if (!initializeMonoDomain("LumaMonoDomain"))
    {
        LogError("MonoHost: 初始化Mono域失败");
        return false;
    }

    
    if (!loadAssembly(mainAssemblyPath))
    {
        LogError("MonoHost: 加载SDK程序集失败");
        Shutdown();
        return false;
    }

    
    if (!initializeDelegates())
    {
        LogError("MonoHost: 初始化委托失败");
        Shutdown();
        return false;
    }

    
    if (m_initializeDomainFn)
    {
        std::string assemblyPathUtf8 = pathToUtf8(m_scriptAssembliesPath);
        m_initializeDomainFn(assemblyPathUtf8.c_str());
    }

    LogInfo("MonoHost: 宿主初始化成功");
    m_isInitialized = true;
    return true;
}

void MonoHost::Shutdown()
{
    
    if (m_unloadDomainFn)
    {
        m_unloadDomainFn();
    }

    
    if (m_monoDomain && mono_jit_cleanup)
    {
        mono_jit_cleanup(m_monoDomain);
        m_monoDomain = nullptr;
    }

    
    m_sdkAssembly = nullptr;
    m_sdkImage = nullptr;

    
    m_methodCache.clear();
    s_methodContextCache.clear();

    
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

    m_scriptAssembliesPath.clear();
    m_isInitialized = false;

    LogInfo("MonoHost: 已完全关闭");
}

bool MonoHost::loadMonoRuntime()
{
    if (s_monoLibHandle)
    {
        LogInfo("MonoHost: Mono运行时已加载");
        return true;
    }

#ifdef __ANDROID__
    
    s_monoLibHandle = dlopen("libmonosgen-2.0.so", RTLD_LAZY | RTLD_GLOBAL);
    if (!s_monoLibHandle)
    {
        LogError("MonoHost: 无法加载libmonosgen-2.0.so: {}", dlerror());
        return false;
    }

    
    #define LOAD_MONO_FUNC(name) \
        name = (name##_fn)dlsym(s_monoLibHandle, #name); \
        if (!name) { \
            LogError("MonoHost: 无法加载函数: {}", #name); \
            dlclose(s_monoLibHandle); \
            s_monoLibHandle = nullptr; \
            return false; \
        }

    LOAD_MONO_FUNC(mono_jit_init);
    LOAD_MONO_FUNC(mono_jit_init_version);
    LOAD_MONO_FUNC(mono_jit_cleanup);
    LOAD_MONO_FUNC(mono_domain_assembly_open);
    LOAD_MONO_FUNC(mono_assembly_get_image);
    LOAD_MONO_FUNC(mono_class_from_name);
    LOAD_MONO_FUNC(mono_class_get_method_from_name);
    LOAD_MONO_FUNC(mono_runtime_invoke);
    LOAD_MONO_FUNC(mono_add_internal_call);
    LOAD_MONO_FUNC(mono_thread_attach);
    LOAD_MONO_FUNC(mono_thread_detach);
    LOAD_MONO_FUNC(mono_get_root_domain);
    LOAD_MONO_FUNC(mono_domain_set_config);
    LOAD_MONO_FUNC(mono_string_new);
    LOAD_MONO_FUNC(mono_string_to_utf8);
    LOAD_MONO_FUNC(mono_free);
    LOAD_MONO_FUNC(mono_object_new);
    LOAD_MONO_FUNC(mono_runtime_object_init);
    LOAD_MONO_FUNC(mono_gchandle_new);
    LOAD_MONO_FUNC(mono_gchandle_free);
    LOAD_MONO_FUNC(mono_gchandle_get_target);
    LOAD_MONO_FUNC(mono_domain_get);
    LOAD_MONO_FUNC(mono_object_unbox);
    LOAD_MONO_FUNC(mono_value_box);
    LOAD_MONO_FUNC(mono_jit_parse_options);
    LOAD_MONO_FUNC(mono_debug_init);

    #undef LOAD_MONO_FUNC

    LogInfo("MonoHost: Mono运行时库加载成功");
    return true;
#else
    LogError("MonoHost: 当前平台不支持Mono运行时");
    return false;
#endif
}

bool MonoHost::initializeMonoDomain(const std::string& domainName)
{
    if (m_monoDomain)
    {
        LogWarn("MonoHost: Mono域已初始化");
        return true;
    }

    
    if (mono_jit_parse_options && mono_debug_init)
    {
        const bool dbgEnabled = ProjectSettings::GetInstance().GetScriptDebugEnabled();
        if (dbgEnabled)
        {
            std::string address = ProjectSettings::GetInstance().GetScriptDebugAddress();
#ifdef __ANDROID__
            
            if (address == "127.0.0.1") address = "0.0.0.0";
#endif
            const int port = ProjectSettings::GetInstance().GetScriptDebugPort();
            const bool suspend = ProjectSettings::GetInstance().GetScriptDebugWaitForAttach();

            std::string agentOpt = std::format("--debugger-agent=transport=dt_socket,address={}:{}{},server=y{}",
                                               address,
                                               port,
                                               "",
                                               suspend ? ",suspend=y" : ",suspend=n");

            std::string debugOpt = "--debug";
            std::string softBpOpt = "--soft-breakpoints";

            char* argv[] = { debugOpt.data(), softBpOpt.data(), agentOpt.data() };
            mono_jit_parse_options(3, argv);

            
            mono_debug_init(2);
            LogInfo("MonoHost: 启用Mono调试代理: {}", agentOpt);
        }
    }

    m_monoDomain = mono_jit_init(domainName.c_str());
    if (!m_monoDomain)
    {
        LogError("MonoHost: 初始化Mono JIT失败");
        return false;
    }

    
    std::string assemblyPath = pathToUtf8(m_scriptAssembliesPath);
    mono_domain_set_config(m_monoDomain, assemblyPath.c_str(), nullptr);

    LogInfo("MonoHost: Mono域初始化成功: {}", domainName);
    return true;
}

bool MonoHost::loadAssembly(const std::filesystem::path& assemblyPath)
{
    if (!m_monoDomain)
    {
        LogError("MonoHost: Mono域未初始化，无法加载程序集");
        return false;
    }

    std::string assemblyPathUtf8 = pathToUtf8(assemblyPath);
    m_sdkAssembly = mono_domain_assembly_open(m_monoDomain, assemblyPathUtf8.c_str());
    if (!m_sdkAssembly)
    {
        LogError("MonoHost: 无法加载SDK程序集: {}", assemblyPathUtf8);
        return false;
    }

    m_sdkImage = mono_assembly_get_image(m_sdkAssembly);
    if (!m_sdkImage)
    {
        LogError("MonoHost: 无法获取SDK程序集镜像");
        return false;
    }

    LogInfo("MonoHost: SDK程序集加载成功: {}", assemblyPathUtf8);
    return true;
}

void* MonoHost::getManagedMethod(const std::string& namespaceName, const std::string& className,
                                const std::string& methodName, int paramCount)
{
    
    std::string cacheKey = namespaceName + "::" + className + "::" + methodName;

    
    auto it = m_methodCache.find(cacheKey);
    if (it != m_methodCache.end())
    {
        return it->second;
    }

    if (!m_sdkImage)
    {
        LogError("MonoHost: SDK程序集镜像未初始化");
        return nullptr;
    }

    void* klass = mono_class_from_name(m_sdkImage, namespaceName.c_str(), className.c_str());
    if (!klass)
    {
        LogError("MonoHost: 无法找到类 {}.{}", namespaceName, className);
        return nullptr;
    }

    void* method = mono_class_get_method_from_name(klass, methodName.c_str(), paramCount);
    if (!method)
    {
        LogError("MonoHost: 无法找到方法 {}.{}.{}", namespaceName, className, methodName);
        return nullptr;
    }

    
    m_methodCache[cacheKey] = method;

    LogInfo("MonoHost: 成功获取方法: {}", cacheKey);
    return method;
}

bool MonoHost::initializeDelegates()
{
    std::string namespaceName = "Luma.SDK";
    std::string className = "Interop";

    
    void* initMethod = getManagedMethod(namespaceName, className, wideToUtf8(InitializeDomainMethodName), 1);
    void* unloadMethod = getManagedMethod(namespaceName, className, wideToUtf8(UnloadDomainMethodName), 0);
    void* createMethod = getManagedMethod(namespaceName, className, wideToUtf8(CreateMethodName), 4);
    void* destroyMethod = getManagedMethod(namespaceName, className, wideToUtf8(DestroyMethodName), 1);
    void* updateMethod = getManagedMethod(namespaceName, className, wideToUtf8(UpdateMethodName), 2);
    void* setPropMethod = getManagedMethod(namespaceName, className, wideToUtf8(SetPropertyMethodName), 3);
    void* debugListMethod = getManagedMethod(namespaceName, className, wideToUtf8(DebugListMethodName), 1);
    void* invokeMethod = getManagedMethod(namespaceName, className, wideToUtf8(InvokeMethodName), 3);
    void* onCreateMethod = getManagedMethod(namespaceName, className, wideToUtf8(OnCreateMethodName), 1);
    void* dispatchCollisionMethod = getManagedMethod(namespaceName, className, wideToUtf8(DispatchCollisionEventMethodName), 3);
    void* onEnableMethod = getManagedMethod(namespaceName, className, wideToUtf8(CallOnEnableMethodName), 1);
    void* onDisableMethod = getManagedMethod(namespaceName, className, wideToUtf8(CallOnDisableMethodName), 1);

    
    if (!initMethod || !unloadMethod || !createMethod || !destroyMethod ||
        !updateMethod || !setPropMethod || !debugListMethod || !invokeMethod ||
        !onCreateMethod || !dispatchCollisionMethod || !onEnableMethod || !onDisableMethod)
    {
        LogError("MonoHost: 无法获取所有必需的托管方法");
        return false;
    }

    
    s_methodContextCache["InitializeDomain"] = MonoMethodContext(initMethod, m_monoDomain);
    s_methodContextCache["UnloadDomain"] = MonoMethodContext(unloadMethod, m_monoDomain);
    s_methodContextCache["CreateScriptInstance"] = MonoMethodContext(createMethod, m_monoDomain);
    s_methodContextCache["DestroyScriptInstance"] = MonoMethodContext(destroyMethod, m_monoDomain);
    s_methodContextCache["InvokeUpdate"] = MonoMethodContext(updateMethod, m_monoDomain);
    s_methodContextCache["SetExportedProperty"] = MonoMethodContext(setPropMethod, m_monoDomain);
    s_methodContextCache["Debug_ListAllTypesAndMethods"] = MonoMethodContext(debugListMethod, m_monoDomain);
    s_methodContextCache["InvokeMethod"] = MonoMethodContext(invokeMethod, m_monoDomain);
    s_methodContextCache["OnCreate"] = MonoMethodContext(onCreateMethod, m_monoDomain);
    s_methodContextCache["DispatchCollisionEvent"] = MonoMethodContext(dispatchCollisionMethod, m_monoDomain);
    s_methodContextCache["OnEnable"] = MonoMethodContext(onEnableMethod, m_monoDomain);
    s_methodContextCache["OnDisable"] = MonoMethodContext(onDisableMethod, m_monoDomain);

    
    m_initializeDomainFn = Wrapper_InitializeDomain;
    m_unloadDomainFn = Wrapper_UnloadDomain;
    m_createInstanceFn = Wrapper_CreateInstance;
    m_destroyInstanceFn = Wrapper_DestroyInstance;
    m_updateInstanceFn = Wrapper_UpdateInstance;
    m_setPropertyFn = Wrapper_SetProperty;
    m_debugListFn = Wrapper_DebugList;
    m_invokeMethodFn = Wrapper_InvokeMethod;
    m_onCreateFn = Wrapper_OnCreate;
    m_dispatchCollisionEventFn = Wrapper_DispatchCollisionEvent;
    m_callOnEnableFn = Wrapper_CallOnEnable;
    m_callOnDisableFn = Wrapper_CallOnDisable;

    LogInfo("MonoHost: 所有委托初始化成功");
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
    result.reserve(wstr.size());

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
        else
        {
            
            result.push_back(static_cast<char>(0xE0 | ((wc >> 12) & 0x0F)));
            result.push_back(static_cast<char>(0x80 | ((wc >> 6) & 0x3F)));
            result.push_back(static_cast<char>(0x80 | (wc & 0x3F)));
        }
    }

    return result;
}
