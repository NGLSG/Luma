#ifndef CORECLRHOST_H
#define CORECLRHOST_H

#include <string>
#include <filesystem>
#include <memory>
#include <mutex>


#include <nethost.h>
#include <coreclr_delegates.h>
#include <hostfxr.h>

#ifdef _WIN32
/// Windows平台下的动态库句柄类型。
#define NATIVE_LIB_HANDLE HMODULE
/// Windows平台下加载动态库的宏。
#define LOAD_LIBRARY(path) LoadLibraryW(path)
/// Windows平台下释放动态库的宏。
#define FREE_LIBRARY(handle) FreeLibrary(handle)
/// Windows平台下获取函数地址的宏。
#define GET_PROC_ADDRESS(handle, name) GetProcAddress(handle, name)
#else
#include <dlfcn.h>
/// Linux/macOS平台下的动态库句柄类型。
#define NATIVE_LIB_HANDLE void*
/// Linux/macOS平台下加载动态库的宏。
#define LOAD_LIBRARY(path) dlopen(path, RTLD_LAZY)
/// Linux/macOS平台下释放动态库的宏。
#define FREE_LIBRARY(handle) dlclose(handle)
/// Linux/macOS平台下获取函数地址的宏。
#define GET_PROC_ADDRESS(handle, name) dlsym(handle, name)
#endif

#if defined(_WIN32)
/// Windows平台下的回调函数调用约定。
#define LUMA_CALLBACK __stdcall
#else
/// 非Windows平台下的回调函数调用约定。
#define LUMA_CALLBACK
#endif

/// 托管垃圾回收句柄的类型别名。
using ManagedGCHandle = intptr_t;
/// 创建托管实例函数的类型别名。
using CreateInstanceFn = ManagedGCHandle (LUMA_CALLBACK *)(void* scenePtr, uint32_t entityId, const char* typeName,
                                                           const char* assemblyName);
/// 销毁托管实例函数的类型别名。
using DestroyInstanceFn = void (LUMA_CALLBACK *)(ManagedGCHandle handle);
/// 更新托管实例函数的类型别名。
using UpdateInstanceFn = void (LUMA_CALLBACK *)(ManagedGCHandle handle, float deltaTime);
/// 设置托管实例属性的函数的类型别名。
using SetPropertyFn = void (LUMA_CALLBACK *)(ManagedGCHandle handle, const char* propName, const char* valueAsYaml);
/// 调试列表函数的类型别名。
using DebugListFn = void (LUMA_CALLBACK *)(const char* assemblyPath);
/// 调用托管实例方法的函数的类型别名。
using InvokeMethodFn = void (LUMA_CALLBACK *)(ManagedGCHandle handle, const char* methodName, const char* argsAsYaml);
/// 托管实例创建时回调函数的类型别名。
using OnCreateFn = void (LUMA_CALLBACK *)(ManagedGCHandle handle);
/// 托管实例启用时回调函数的类型别名。
using CallOnEnableFn = void (LUMA_CALLBACK *)(ManagedGCHandle handle);
/// 托管实例禁用时回调函数的类型别名。
using CallOnDisableFn = void (LUMA_CALLBACK *)(ManagedGCHandle handle);
/// 分发碰撞事件回调函数的类型别名。
using DispatchCollisionEventFn = void (LUMA_CALLBACK *)(ManagedGCHandle handlePtr, int contactType,
                                                        uint32_t otherEntityId);


/// 初始化托管域函数的类型别名。
using InitializeDomainFn = void (LUMA_CALLBACK *)(const char* baseDirUtf8);
/// 卸载托管域函数的类型别名。
using UnloadDomainFn = void (LUMA_CALLBACK *)();

/**
 * @brief CoreCLR宿主类，用于加载和管理.NET Core运行时。
 *
 * 该类提供了一个单例接口，用于初始化CoreCLR运行时，加载托管程序集，
 * 并通过函数指针与托管代码进行交互。
 */
class LUMA_API CoreCLRHost
{
public:
    /**
     * @brief 获取CoreCLRHost的单例实例。
     * @return CoreCLRHost* 返回CoreCLRHost的单例指针。
     */
    static CoreCLRHost* GetInstance();

    /**
     * @brief 创建CoreCLRHost的新单例实例。
     *
     * 如果实例已存在，此方法将销毁旧实例并创建新实例。
     */
    static void CreateNewInstance();

    /**
     * @brief 销毁CoreCLRHost的单例实例。
     */
    static void DestroyInstance();

    /**
     * @brief 初始化CoreCLR宿主。
     * @param mainAssemblyPath 主程序集的路径。
     * @param isEditorMode 指示是否为编辑器模式。
     * @return bool 如果初始化成功则返回true，否则返回false。
     */
    bool Initialize(const std::filesystem::path& mainAssemblyPath, bool isEditorMode);
    /**
     * @brief 关闭CoreCLR宿主。
     *
     * 释放所有相关资源。
     */
    void Shutdown();

    /**
     * @brief 获取创建实例函数的指针。
     * @return CreateInstanceFn 创建实例函数的指针。
     */
    CreateInstanceFn GetCreateInstanceFn() const { return m_createInstanceFn; }
    /**
     * @brief 获取销毁实例函数的指针。
     * @return DestroyInstanceFn 销毁实例函数的指针。
     */
    DestroyInstanceFn GetDestroyInstanceFn() const { return m_destroyInstanceFn; }
    /**
     * @brief 获取更新实例函数的指针。
     * @return UpdateInstanceFn 更新实例函数的指针。
     */
    UpdateInstanceFn GetUpdateInstanceFn() const { return m_updateInstanceFn; }
    /**
     * @brief 获取设置属性函数的指针。
     * @return SetPropertyFn 设置属性函数的指针。
     */
    SetPropertyFn GetSetPropertyFn() const { return m_setPropertyFn; }
    /**
     * @brief 获取调试列表函数的指针。
     * @return DebugListFn 调试列表函数的指针。
     */
    DebugListFn GetDebugListFn() const { return m_debugListFn; }
    /**
     * @brief 获取调用方法函数的指针。
     * @return InvokeMethodFn 调用方法函数的指针。
     */
    InvokeMethodFn GetInvokeMethodFn() const { return m_invokeMethodFn; }
    /**
     * @brief 获取创建时回调函数的指针。
     * @return OnCreateFn 创建时回调函数的指针。
     */
    OnCreateFn GetOnCreateFn() const { return m_onCreateFn; }
    /**
     * @brief 获取分发碰撞事件函数的指针。
     * @return DispatchCollisionEventFn 分发碰撞事件函数的指针。
     */
    DispatchCollisionEventFn GetDispatchCollisionEventFn() const { return m_dispatchCollisionEventFn; }
    /**
     * @brief 获取启用时回调函数的指针。
     * @return CallOnEnableFn 启用时回调函数的指针。
     */
    CallOnEnableFn GetCallOnEnableFn() const { return m_callOnEnableFn; }
    /**
     * @brief 获取禁用时回调函数的指针。
     * @return CallOnDisableFn 禁用时回调函数的指针。
     */
    CallOnDisableFn GetCallOnDisableFn() const { return m_callOnDisableFn; }

    /**
     * @brief CoreCLRHost的默认构造函数。
     */
    CoreCLRHost() = default;
    /**
     * @brief CoreCLRHost的析构函数。
     *
     * 在对象销毁时执行清理操作。
     */
    ~CoreCLRHost();

private:
    /**
     * @brief 禁用拷贝构造函数。
     */
    CoreCLRHost(const CoreCLRHost&) = delete;
    /**
     * @brief 禁用拷贝赋值运算符。
     */
    CoreCLRHost& operator=(const CoreCLRHost&) = delete;

    /**
     * @brief 如果需要，引导CoreCLR运行时。
     * @param sdkDir SDK目录路径。
     * @return bool 如果运行时引导成功则返回true，否则返回false。
     */
    static bool bootstrapRuntimeIfNeeded(const std::filesystem::path& sdkDir);
    /**
     * @brief 将文件系统路径转换为UTF-8字符串。
     * @param p 要转换的文件系统路径。
     * @return std::string 转换后的UTF-8字符串。
     */
    static std::string pathToUtf8(const std::filesystem::path& p);

    /**
     * @brief 获取托管函数的指针。
     * @param assemblyName 程序集名称。
     * @param typeName 类型名称。
     * @param methodName 方法名称。
     * @return void* 托管函数的指针。
     */
    void* getManagedFunction(const std::wstring& assemblyName, const std::wstring& typeName,
                             const std::wstring& methodName) const;

    /**
     * @brief 创建一个新的影子拷贝目录。
     * @return std::filesystem::path 新创建的影子拷贝目录路径。
     */
    std::filesystem::path createNewShadowCopyDirectory();

    /**
     * @brief 将程序集复制到影子目录。
     * @param sourceDir 源目录路径。
     * @param targetDir 目标目录路径。
     * @return bool 如果复制成功则返回true，否则返回false。
     */
    bool copyAssembliesToShadowDirectory(const std::filesystem::path& sourceDir,
                                         const std::filesystem::path& targetDir);

    /**
     * @brief 清理旧的影子拷贝目录。
     */
    void cleanupOldShadowCopies();

    /// CoreCLRHost的单例实例。
    inline static std::unique_ptr<CoreCLRHost> s_instance = nullptr;
    /// 用于保护单例实例访问的静态互斥锁。
    static std::mutex s_mutex;

    /// 运行时是否已引导的标志。
    static inline bool s_runtimeBootstrapped = false;
    /// hostfxr库的句柄。
    static inline NATIVE_LIB_HANDLE s_hostfxrLibrary = nullptr;
    /// 获取函数指针的共享函数。
    static inline get_function_pointer_fn s_sharedGetFunctionPtrFn = nullptr;
    /// 加载程序集的共享函数。
    static inline load_assembly_fn s_sharedLoadAssemblyFn = nullptr;

    /// 脚本程序集所在的路径。
    std::filesystem::path m_scriptAssembliesPath;

    /// 初始化托管域的函数指针。
    InitializeDomainFn m_initializeDomainFn = nullptr;
    /// 卸载托管域的函数指针。
    UnloadDomainFn m_unloadDomainFn = nullptr;

    /// 创建托管实例的函数指针。
    CreateInstanceFn m_createInstanceFn = nullptr;
    /// 托管实例创建时回调的函数指针。
    OnCreateFn m_onCreateFn = nullptr;
    /// 销毁托管实例的函数指针。
    DestroyInstanceFn m_destroyInstanceFn = nullptr;
    /// 更新托管实例的函数指针。
    UpdateInstanceFn m_updateInstanceFn = nullptr;
    /// 设置托管实例属性的函数指针。
    SetPropertyFn m_setPropertyFn = nullptr;
    /// 调试列表的函数指针。
    DebugListFn m_debugListFn = nullptr;
    /// 调用托管实例方法的函数指针。
    InvokeMethodFn m_invokeMethodFn = nullptr;
    /// 分发碰撞事件的函数指针。
    DispatchCollisionEventFn m_dispatchCollisionEventFn = nullptr;
    /// 托管实例启用时回调的函数指针。
    CallOnEnableFn m_callOnEnableFn = nullptr;
    /// 托管实例禁用时回调的函数指针。
    CallOnDisableFn m_callOnDisableFn = nullptr;

    /// 宿主是否已初始化的标志。
    bool m_isInitialized = false;
    /// 宿主是否处于编辑器模式的标志。
    bool m_isEditorMode = false;
    /// 影子拷贝的基路径。
    std::filesystem::path m_shadowCopyBasePath;
    /// 当前活动的影子拷贝目录路径。
    std::filesystem::path m_activeShadowCopyPath;
    /// 是否是首次初始化的标志。
    bool m_isFirstInitialization = true;
};

#endif