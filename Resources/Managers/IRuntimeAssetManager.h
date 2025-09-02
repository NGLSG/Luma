#ifndef IRUNTIMEASSETMANAGER_H
#define IRUNTIMEASSETMANAGER_H
#include "../../Utils/LazySingleton.h"
#include <include/core/SkRefCnt.h>
#include <unordered_map>

#include "../../Utils/Guid.h"

/**
 * @brief 资产性能数据结构体。
 * 这是一个占位符，用于未来存储资产的性能相关数据。
 */
struct AssetPerformanceData
{
};

/**
 * @brief 运行时资产管理器接口。
 * 定义了运行时资产管理器的基本操作，用于获取、添加、更新和移除资产。
 * @tparam T 资产类型，必须是SkRefCnt智能指针可管理的类型。
 */
template <typename T>
class IRuntimeAssetManager
{
protected:
    std::unordered_map<std::string, sk_sp<T>> assets; ///< 存储资产的映射表，键为资产的GUID字符串，值为资产的智能指针。

public:
    /**
     * @brief 虚析构函数。
     * 确保派生类对象能正确销毁。
     */
    virtual ~IRuntimeAssetManager() = default;

    /**
     * @brief 尝试根据GUID获取资产。
     * @param guid 资产的唯一标识符。
     * @return 如果找到资产，返回资产的智能指针；否则返回空指针。
     */
    virtual sk_sp<T> TryGetAsset(const Guid& guid) const =0;

    /**
     * @brief 尝试根据GUID获取资产，并通过输出参数返回。
     * @param guid 资产的唯一标识符。
     * @param out 如果找到资产，资产的智能指针将存储在此参数中；否则为nullptr。
     * @return 如果找到资产并成功获取，返回true；否则返回false。
     */
    virtual bool TryGetAsset(const Guid& guid, sk_sp<T>& out) const =0;

    /**
     * @brief 尝试添加或更新资产。
     * 如果GUID对应的资产已存在，则更新；否则添加新资产。
     * @param guid 资产的唯一标识符。
     * @param asset 要添加或更新的资产智能指针。
     * @return 如果操作成功，返回true；否则返回false。
     */
    virtual bool TryAddOrUpdateAsset(const Guid& guid, sk_sp<T> asset) =0;

    /**
     * @brief 尝试移除资产。
     * @param guid 要移除资产的唯一标识符。
     * @return 如果成功移除资产，返回true；否则返回false。
     */
    virtual bool TryRemoveAsset(const Guid& guid) =0;

    /**
     * @brief 获取资产性能数据。
     * @return 指向资产性能数据的指针。
     */
    virtual const AssetPerformanceData* GetPerformanceData() const =0;

    /**
     * @brief 关闭管理器并清理所有资产。
     */
    virtual void Shutdown() = 0;
};

/**
 * @brief 运行时资产管理器的基类实现。
 * 提供了IRuntimeAssetManager接口的默认实现。
 * @tparam T 资产类型，必须是SkRefCnt智能指针可管理的类型。
 */
template <typename T>
class RuntimeAssetManagerBase : public IRuntimeAssetManager<T>
{
public:
    /**
     * @brief 虚析构函数。
     * 确保派生类对象能正确销毁。
     */
    virtual ~RuntimeAssetManagerBase() = default;

    /**
     * @brief 尝试根据GUID获取资产。
     * @param guid 资产的唯一标识符。
     * @return 如果找到资产，返回资产的智能指针；否则返回空指针。
     */
    sk_sp<T> TryGetAsset(const Guid& guid) const override
    {
        if (m_assets.contains(guid))
        {
            return m_assets.at(guid);
        }
        return nullptr;
    }

    /**
     * @brief 尝试根据GUID获取资产，并通过输出参数返回。
     * @param guid 资产的唯一标识符。
     * @param out 如果找到资产，资产的智能指针将存储在此参数中；否则为nullptr。
     * @return 如果找到资产并成功获取，返回true；否则返回false。
     */
    bool TryGetAsset(const Guid& guid, sk_sp<T>& out) const override
    {
        if (m_assets.contains(guid))
        {
            out = m_assets.at(guid);
            return true;
        }
        out = nullptr;
        return false;
    }

    /**
     * @brief 尝试添加或更新资产。
     * 如果GUID对应的资产已存在，则更新；否则添加新资产。
     * @param guid 资产的唯一标识符。
     * @param asset 要添加或更新的资产智能指针。
     * @return 如果操作成功，返回true；否则返回false。
     */
    bool TryAddOrUpdateAsset(const Guid& guid, sk_sp<T> asset) override
    {
        if (!asset || !guid.Valid())
        {
            return false;
        }

        m_assets[guid] = std::move(asset);
        return true;
    }

    /**
     * @brief 尝试移除资产。
     * @param guid 要移除资产的唯一标识符。
     * @return 如果成功移除资产，返回true；否则返回false。
     */
    bool TryRemoveAsset(const Guid& guid) override
    {
        return m_assets.erase(guid) > 0;
    }

    /**
     * @brief 获取资产性能数据。
     * @return 指向资产性能数据的指针。此基类实现总是返回nullptr。
     */
    const AssetPerformanceData* GetPerformanceData() const override
    {
        return nullptr;
    }

    /**
     * @brief 关闭管理器并清理所有资产。
     * 此实现会清空内部的资产映射表。
     */
    void Shutdown() override
    {
        m_assets.clear();
    }

protected:
    std::unordered_map<std::string, sk_sp<T>> m_assets; ///< 存储资产的映射表，键为资产的GUID字符串，值为资产的智能指针。
};
#endif