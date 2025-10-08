#ifndef SYSTEMSMANAGER_H
#define SYSTEMSMANAGER_H
#pragma once

#include <memory>
#include <vector>

#include "ISystem.h"


class RuntimeScene;
struct EngineContext;

/**
 * @brief 系统管理器，负责管理和更新场景中的所有系统。
 *
 * 系统管理器将系统分为三类：
 * 1. 核心系统（Essential Systems）- 场景生命周期中不可或缺的系统
 * 2. 模拟线程系统（Simulation Thread Systems）- 在模拟线程中更新的系统（如物理系统）
 * 3. 主线程系统（Main Thread Systems）- 在主线程中更新的系统（如渲染系统）
 */
class SystemsManager
{
public:
    /**
     * @brief 构造函数。
     */
    SystemsManager() = default;

    /**
     * @brief 析构函数，清理所有系统。
     */
    ~SystemsManager();

    /**
     * @brief 禁用拷贝构造函数。
     */
    SystemsManager(const SystemsManager&) = delete;

    /**
     * @brief 禁用拷贝赋值运算符。
     */
    SystemsManager& operator=(const SystemsManager&) = delete;

    /**
     * @brief 向模拟线程添加一个系统。
     * @tparam T 系统的类型，必须继承自 ISystem。
     * @tparam Args 构造系统所需的参数类型。
     * @param args 构造系统所需的参数。
     * @return 添加的系统实例的原始指针。
     */
    template <typename T, typename... Args>
    T* AddSimulationSystem(Args&&... args);

    /**
     * @brief 向主线程添加一个系统。
     * @tparam T 系统的类型，必须继承自 ISystem。
     * @tparam Args 构造系统所需的参数类型。
     * @param args 构造系统所需的参数。
     * @return 添加的系统实例的原始指针。
     */
    template <typename T, typename... Args>
    T* AddMainThreadSystem(Args&&... args);

    /**
     * @brief 向核心系统列表添加一个模拟线程系统。
     * @tparam T 系统的类型，必须继承自 ISystem。
     * @tparam Args 构造系统所需的参数类型。
     * @param args 构造系统所需的参数。
     * @return 添加的系统实例的原始指针。
     */
    template <typename T, typename... Args>
    T* AddEssentialSimulationSystem(Args&&... args);

    /**
     * @brief 向核心系统列表添加一个主线程系统。
     * @tparam T 系统的类型，必须继承自 ISystem。
     * @tparam Args 构造系统所需的参数类型。
     * @param args 构造系统所需的参数。
     * @return 添加的系统实例的原始指针。
     */
    template <typename T, typename... Args>
    T* AddEssentialMainThreadSystem(Args&&... args);

    /**
     * @brief 获取指定类型的系统。
     * @tparam T 要获取的系统类型。
     * @return 指定类型的系统实例的原始指针，如果不存在则返回 nullptr。
     */
    template <typename T>
    T* GetSystem();

    /**
     * @brief 初始化所有系统。
     * @param scene 系统所属的场景。
     * @param engineCtx 引擎上下文。
     */
    void InitializeSystems(RuntimeScene* scene, EngineContext& engineCtx);

    /**
     * @brief 更新模拟线程系统。
     * @param scene 系统所属的场景。
     * @param deltaTime 帧时间间隔。
     * @param engineCtx 引擎上下文。
     * @param pauseNormalSystems 是否暂停普通（非核心）系统的更新。
     */
    void UpdateSimulationSystems(RuntimeScene* scene, float deltaTime, EngineContext& engineCtx,
                                 bool pauseNormalSystems);

    /**
     * @brief 更新主线程系统。
     * @param scene 系统所属的场景。
     * @param deltaTime 帧时间间隔。
     * @param engineCtx 引擎上下文。
     * @param pauseNormalSystems 是否暂停普通（非核心）系统的更新。
     */
    void UpdateMainThreadSystems(RuntimeScene* scene, float deltaTime, EngineContext& engineCtx,
                                 bool pauseNormalSystems);

    /**
     * @brief 销毁所有系统。
     * @param scene 系统所属的场景。
     */
    void DestroySystems(RuntimeScene* scene);

    /**
     * @brief 清空所有系统列表。
     */
    void Clear();

private:
    std::vector<std::unique_ptr<Systems::ISystem>> m_essentialSimulationSystems; ///< 核心模拟线程系统列表
    std::vector<std::unique_ptr<Systems::ISystem>> m_essentialMainThreadSystems; ///< 核心主线程系统列表
    std::vector<std::unique_ptr<Systems::ISystem>> m_simulationSystems; ///< 普通模拟线程系统列表
    std::vector<std::unique_ptr<Systems::ISystem>> m_mainThreadSystems; ///< 普通主线程系统列表
};


template <typename T, typename... Args>
T* SystemsManager::AddSimulationSystem(Args&&... args)
{
    static_assert(std::is_base_of_v<Systems::ISystem, T>, "T must inherit from ISystem");
    auto newSystem = std::make_unique<T>(std::forward<Args>(args)...);
    T* rawPtr = newSystem.get();
    m_simulationSystems.push_back(std::move(newSystem));
    return rawPtr;
}

template <typename T, typename... Args>
T* SystemsManager::AddMainThreadSystem(Args&&... args)
{
    static_assert(std::is_base_of_v<Systems::ISystem, T>, "T must inherit from ISystem");
    auto newSystem = std::make_unique<T>(std::forward<Args>(args)...);
    T* rawPtr = newSystem.get();
    m_mainThreadSystems.push_back(std::move(newSystem));
    return rawPtr;
}

template <typename T, typename... Args>
T* SystemsManager::AddEssentialSimulationSystem(Args&&... args)
{
    static_assert(std::is_base_of_v<Systems::ISystem, T>, "T must inherit from ISystem");
    auto newSystem = std::make_unique<T>(std::forward<Args>(args)...);
    T* rawPtr = newSystem.get();
    m_essentialSimulationSystems.push_back(std::move(newSystem));
    return rawPtr;
}

template <typename T, typename... Args>
T* SystemsManager::AddEssentialMainThreadSystem(Args&&... args)
{
    static_assert(std::is_base_of_v<Systems::ISystem, T>, "T must inherit from ISystem");
    auto newSystem = std::make_unique<T>(std::forward<Args>(args)...);
    T* rawPtr = newSystem.get();
    m_essentialMainThreadSystems.push_back(std::move(newSystem));
    return rawPtr;
}

template <typename T>
T* SystemsManager::GetSystem()
{
    static_assert(std::is_base_of_v<Systems::ISystem, T>, "T must inherit from ISystem");

    // 在核心模拟线程系统中查找
    for (const auto& system : m_essentialSimulationSystems)
    {
        if (auto* sys = dynamic_cast<T*>(system.get()))
        {
            return sys;
        }
    }

    // 在核心主线程系统中查找
    for (const auto& system : m_essentialMainThreadSystems)
    {
        if (auto* sys = dynamic_cast<T*>(system.get()))
        {
            return sys;
        }
    }

    // 在普通模拟线程系统中查找
    for (const auto& system : m_simulationSystems)
    {
        if (auto* sys = dynamic_cast<T*>(system.get()))
        {
            return sys;
        }
    }

    // 在普通主线程系统中查找
    for (const auto& system : m_mainThreadSystems)
    {
        if (auto* sys = dynamic_cast<T*>(system.get()))
        {
            return sys;
        }
    }

    return nullptr;
}

#endif // SYSTEMSMANAGER_H
