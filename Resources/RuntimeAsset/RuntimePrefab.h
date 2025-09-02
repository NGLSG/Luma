#ifndef RUNTIMEPREFAB_H
#define RUNTIMEPREFAB_H

#include "IRuntimeAsset.h"
#include "../../Data/PrefabData.h"
#include <memory>

// 前向声明运行时场景类
class RuntimeScene;
// 前向声明运行时游戏对象类
class RuntimeGameObject;

/**
 * @brief 运行时预制体类。
 *
 * 继承自IRuntimeAsset，并支持从自身创建共享指针。
 * 用于在运行时实例化游戏对象。
 */
class RuntimePrefab : public IRuntimeAsset, public std::enable_shared_from_this<RuntimePrefab>
{
public:
    /**
     * @brief 构造函数，创建一个运行时预制体。
     * @param sourceGuid 源GUID，标识预制体的唯一ID。
     * @param data 预制体数据。
     */
    RuntimePrefab(const Guid& sourceGuid, Data::PrefabData data)
        : m_data(std::move(data))
    {
        m_sourceGuid = sourceGuid;
    }

    /**
     * @brief 在指定场景中实例化预制体。
     * @param targetScene 目标场景，预制体将被实例化到此场景中。
     * @return 实例化后的运行时游戏对象。
     */
    RuntimeGameObject Instantiate(RuntimeScene& targetScene);

    /**
     * @brief 获取预制体的原始数据。
     * @return 预制体数据的常量引用。
     */
    [[nodiscard]] const Data::PrefabData& GetData() const { return m_data; }

private:
    Data::PrefabData m_data; ///< 预制体包含的数据。
};
#endif