// Android平台基于Mono的托管宿主实现
#ifndef MONOHOST_H
#define MONOHOST_H

#include <string>
#include <filesystem>
#include <memory>
#include <mutex>
#include <unordered_map>

#include "../Utils/Platform.h"

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
 *
 * 该类封装了Mono运行时的初始化、程序集加载和方法调用功能。
 * 提供与CoreCLRHost相同的API接口，便于在不同平台间切换。
 */
class LUMA_API MonoHost
{
public:
    /**
     * @brief 获取MonoHost的单例实例
     * @return MonoHost* 单例指针
     */
    static MonoHost* GetInstance();

    /**
     * @brief 创建新的单例实例
     *
     * 如果已存在实例，将先销毁旧实例再创建新实例。
     */
    static void CreateNewInstance();

    /**
     * @brief 销毁单例实例
     */
    static void DestroyInstance();

    /**
     * @brief 初始化Mono宿主
     * @param mainAssemblyPath 主程序集路径
     * @param isEditorMode 是否为编辑器模式
     * @return bool 初始化成功返回true，否则返回false
     */
    bool Initialize(const std::filesystem::path& mainAssemblyPath, bool isEditorMode);

    /**
     * @brief 关闭Mono宿主，释放所有资源
     */
    void Shutdown();

    /**
     * @brief 获取创建脚本实例的函数指针
     * @return CreateInstanceFn 函数指针
     */
    CreateInstanceFn GetCreateInstanceFn() const { return m_createInstanceFn; }

    /**
     * @brief 获取销毁脚本实例的函数指针
     * @return DestroyInstanceFn 函数指针
     */
    DestroyInstanceFn GetDestroyInstanceFn() const { return m_destroyInstanceFn; }

    /**
     * @brief 获取更新脚本实例的函数指针
     * @return UpdateInstanceFn 函数指针
     */
    UpdateInstanceFn GetUpdateInstanceFn() const { return m_updateInstanceFn; }

    /**
     * @brief 获取设置属性的函数指针
     * @return SetPropertyFn 函数指针
     */
    SetPropertyFn GetSetPropertyFn() const { return m_setPropertyFn; }

    /**
     * @brief 获取调试列表的函数指针
     * @return DebugListFn 函数指针
     */
    DebugListFn GetDebugListFn() const { return m_debugListFn; }

    /**
     * @brief 获取调用方法的函数指针
     * @return InvokeMethodFn 函数指针
     */
    InvokeMethodFn GetInvokeMethodFn() const { return m_invokeMethodFn; }

    /**
     * @brief 获取OnCreate回调的函数指针
     * @return OnCreateFn 函数指针
     */
    OnCreateFn GetOnCreateFn() const { return m_onCreateFn; }

    /**
     * @brief 获取分发碰撞事件的函数指针
     * @return DispatchCollisionEventFn 函数指针
     */
    DispatchCollisionEventFn GetDispatchCollisionEventFn() const { return m_dispatchCollisionEventFn; }

    /**
     * @brief 获取OnEnable回调的函数指针
     * @return CallOnEnableFn 函数指针
     */
    CallOnEnableFn GetCallOnEnableFn() const { return m_callOnEnableFn; }

    /**
     * @brief 获取OnDisable回调的函数指针
     * @return CallOnDisableFn 函数指针
     */
    CallOnDisableFn GetCallOnDisableFn() const { return m_callOnDisableFn; }

    /**
     * @brief 默认构造函数
     */
    MonoHost() = default;

    /**
     * @brief 析构函数
     */
    ~MonoHost() = default;

private:
    // 禁用拷贝构造和赋值
    MonoHost(const MonoHost&) = delete;
    MonoHost& operator=(const MonoHost&) = delete;

    /**
     * @brief 加载Mono运行时库并初始化函数指针
     * @return bool 成功返回true，失败返回false
     */
    bool loadMonoRuntime();

    /**
     * @brief 初始化Mono JIT编译器
     * @param domainName 域名称
     * @return bool 成功返回true，失败返回false
     */
    bool initializeMonoDomain(const std::string& domainName);

    /**
     * @brief 加载程序集
     * @param assemblyPath 程序集路径
     * @return bool 成功返回true，失败返回false
     */
    bool loadAssembly(const std::filesystem::path& assemblyPath);

    /**
     * @brief 获取托管方法句柄
     * @param namespaceName 命名空间
     * @param className 类名
     * @param methodName 方法名
     * @param paramCount 参数数量（-1表示任意参数）
     * @return void* 方法句柄，失败返回nullptr
     */
    void* getManagedMethod(const std::string& namespaceName, const std::string& className,
                           const std::string& methodName, int paramCount = -1);

    /**
     * @brief 初始化所有委托包装函数
     * @return bool 成功返回true，失败返回false
     */
    bool initializeDelegates();

    /**
     * @brief 将文件系统路径转换为UTF-8字符串
     * @param p 文件系统路径
     * @return std::string UTF-8字符串
     */
    static std::string pathToUtf8(const std::filesystem::path& p);

    /**
     * @brief 将宽字符串转换为UTF-8字符串
     * @param wstr 宽字符串
     * @return std::string UTF-8字符串
     */
    static std::string wideToUtf8(const std::wstring& wstr);

    // 静态成员
    inline static std::unique_ptr<MonoHost> s_instance = nullptr;
    static std::mutex s_mutex;

    // Mono运行时相关句柄
    void* m_monoDomain = nullptr;
    void* m_sdkAssembly = nullptr;
    void* m_sdkImage = nullptr;

    // 托管方法句柄缓存
    std::unordered_map<std::string, void*> m_methodCache;

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
