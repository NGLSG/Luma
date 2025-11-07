// Android平台基于Mono的托管宿主实现
#ifndef MONOHOST_H
#define MONOHOST_H
#ifdef __ANDROID__
#include <string>
#include <filesystem>
#include <memory>
#include <mutex>
#include <unordered_map>

#include "../Utils/Platform.h"
#include <mono/jit/jit.h>
#include <mono/metadata/assembly.h>
#include <mono/metadata/debug-helpers.h>
#include <mono/metadata/mono-config.h>
#include <mono/metadata/mono-debug.h>
#include <mono/metadata/object.h>
#include <mono/metadata/appdomain.h>
#include <mono/metadata/class.h>
#include <mono/metadata/environment.h>
#include <mono/metadata/threads.h>
#include <jni.h>

#if defined(_WIN32)
#define LUMA_CALLBACK __stdcall
#else
#define LUMA_CALLBACK
#endif

/// 托管垃圾回收句柄的类型别名
using ManagedGCHandle = intptr_t;
/// 创建托管实例函数的类型别名
using CreateInstanceFn = ManagedGCHandle (LUMA_CALLBACK *)(void* scenePtr, uint32_t entityId, const char* typeName,
                                                           const char* assemblyName);
/// 销毁托管实例函数的类型别名
using DestroyInstanceFn = void (LUMA_CALLBACK *)(ManagedGCHandle handle);
/// 更新托管实例函数的类型别名
using UpdateInstanceFn = void (LUMA_CALLBACK *)(ManagedGCHandle handle, float deltaTime);
/// 设置托管实例属性的函数的类型别名
using SetPropertyFn = void (LUMA_CALLBACK *)(ManagedGCHandle handle, const char* propName, const char* valueAsYaml);
/// 调试列表函数的类型别名
using DebugListFn = void (LUMA_CALLBACK *)(const char* assemblyPath);
/// 调用托管实例方法的函数的类型别名
using InvokeMethodFn = void (LUMA_CALLBACK *)(ManagedGCHandle handle, const char* methodName, const char* argsAsYaml);
/// 托管实例创建时回调函数的类型别名
using OnCreateFn = void (LUMA_CALLBACK *)(ManagedGCHandle handle);
/// 托管实例启用时回调函数的类型别名
using CallOnEnableFn = void (LUMA_CALLBACK *)(ManagedGCHandle handle);
/// 托管实例禁用时回调函数的类型别名
using CallOnDisableFn = void (LUMA_CALLBACK *)(ManagedGCHandle handle);
/// 分发碰撞事件回调函数的类型别名
using DispatchCollisionEventFn = void (LUMA_CALLBACK *)(ManagedGCHandle handlePtr, int contactType,
                                                        uint32_t otherEntityId);

/// 初始化托管域函数的类型别名
using InitializeDomainFn = void (LUMA_CALLBACK *)(const char* baseDirUtf8);
/// 卸载托管域函数的类型别名
using UnloadDomainFn = void (LUMA_CALLBACK *)();

/**
 * @brief Mono运行时宿主类，为Android平台提供托管代码执行环境
 */
class LUMA_API MonoHost
{
public:
    static MonoHost* GetInstance();
    static void CreateNewInstance();
    static void DestroyInstance();

    bool Initialize(const std::filesystem::path& mainAssemblyPath, bool isEditorMode);
    void Shutdown();

    CreateInstanceFn GetCreateInstanceFn() const { return m_createInstanceFn; }
    DestroyInstanceFn GetDestroyInstanceFn() const { return m_destroyInstanceFn; }
    UpdateInstanceFn GetUpdateInstanceFn() const { return m_updateInstanceFn; }
    SetPropertyFn GetSetPropertyFn() const { return m_setPropertyFn; }
    DebugListFn GetDebugListFn() const { return m_debugListFn; }
    InvokeMethodFn GetInvokeMethodFn() const { return m_invokeMethodFn; }
    OnCreateFn GetOnCreateFn() const { return m_onCreateFn; }
    DispatchCollisionEventFn GetDispatchCollisionEventFn() const { return m_dispatchCollisionEventFn; }
    CallOnEnableFn GetCallOnEnableFn() const { return m_callOnEnableFn; }
    CallOnDisableFn GetCallOnDisableFn() const { return m_callOnDisableFn; }

    MonoHost() = default;
    ~MonoHost() = default;

private:
    MonoHost(const MonoHost&) = delete;
    MonoHost& operator=(const MonoHost&) = delete;

    bool bootstrapRuntimeIfNeeded(const std::filesystem::path& sdkDir);
    MonoMethod* getManagedMethod(const std::wstring& assemblyName, const std::wstring& typeName,
                                 const std::wstring& methodName) const;
    bool initializeDelegates();
    bool checkAndLogException(MonoObject* exception) const;

    static std::string pathToUtf8(const std::filesystem::path& p);
    static std::string wideToUtf8(const std::wstring& wstr);

    // 静态包装函数
    static void wrapperInitializeDomain(const char* baseDirUtf8);
    static void wrapperUnloadDomain();
    static ManagedGCHandle wrapperCreateInstance(void* scenePtr, uint32_t entityId, const char* typeName, const char* assemblyName);
    static void wrapperOnCreate(ManagedGCHandle handle);
    static void wrapperDestroyInstance(ManagedGCHandle handle);
    static void wrapperUpdateInstance(ManagedGCHandle handle, float deltaTime);
    static void wrapperSetProperty(ManagedGCHandle handle, const char* propName, const char* valueAsYaml);
    static void wrapperDebugList(const char* assemblyPath);
    static void wrapperInvokeMethod(ManagedGCHandle handle, const char* methodName, const char* argsAsYaml);
    static void wrapperDispatchCollisionEvent(ManagedGCHandle handle, int contactType, uint32_t otherEntityId);
    static void wrapperOnEnable(ManagedGCHandle handle);
    static void wrapperOnDisable(ManagedGCHandle handle);

    // 静态成员
    inline static std::unique_ptr<MonoHost> s_instance = nullptr;
    static std::mutex s_mutex;

    // Mono运行时相关句柄
    MonoDomain* m_monoDomain = nullptr;
    MonoAssembly* m_sdkAssembly = nullptr;
    MonoImage* m_sdkImage = nullptr;

    // 托管方法句柄缓存
    mutable std::unordered_map<std::string, MonoMethod*> m_methodCache;

    // 脚本程序集路径
    std::filesystem::path m_scriptAssembliesPath;

    // 委托函数指针
    InitializeDomainFn m_initializeDomainFn = nullptr;
    UnloadDomainFn m_unloadDomainFn = nullptr;
    CreateInstanceFn m_createInstanceFn = nullptr;
    OnCreateFn m_onCreateFn = nullptr;
    DestroyInstanceFn m_destroyInstanceFn = nullptr;
    UpdateInstanceFn m_updateInstanceFn = nullptr;
    SetPropertyFn m_setPropertyFn = nullptr;
    DebugListFn m_debugListFn = nullptr;
    InvokeMethodFn m_invokeMethodFn = nullptr;
    DispatchCollisionEventFn m_dispatchCollisionEventFn = nullptr;
    CallOnEnableFn m_callOnEnableFn = nullptr;
    CallOnDisableFn m_callOnDisableFn = nullptr;

    // 状态标志
    bool m_isInitialized = false;
    bool m_isEditorMode = false;
};

#endif
#endif