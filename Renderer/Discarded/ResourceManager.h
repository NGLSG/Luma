#ifndef RESOURCEMANAGER_H
#define RESOURCEMANAGER_H
#include <string>
#include <unordered_map>
#include <vector>

#include "Meta.h"
#include "../Utils/LazySingleton.h"

/**
 * @brief 资源管理器类，采用懒汉式单例模式。
 * 负责管理应用程序中的各种资源，如加载、添加和获取资源。
 */
class ResourceManager : public LazySingleton<ResourceManager>
{
private:
    /// 存储所有资源的哈希映射，键为资源的唯一标识符（GUID字符串），值为资源的元数据（Meta对象）。
    std::unordered_map<std::string, Meta> Resources;

public:
    /**
     * @brief 默认构造函数。
     */
    ResourceManager() = default;

    /**
     * @brief 初始化资源管理器。
     * @param ResourccePath 资源文件的根路径（可选）。
     */
    void Initialize(std::string ResourccePath = "")
    {
    }

    /**
     * @brief 添加一个资源到管理器中。
     * @param meta 待添加资源的元数据。
     */
    void AddResource(const Meta& meta)
    {
        Resources[meta.guid.ToString()] = meta;
    }

    /**
     * @brief 根据唯一标识符获取一个资源。
     * @tparam T 期望返回的资源类型。
     * @param uid 资源的唯一标识符（GUID字符串）。
     * @return 指向指定类型资源的指针，如果未找到或类型不匹配则返回nullptr。
     */
    template <typename T>
    T* GetResource(const std::string& uid)
    {
        auto it = Resources.find(uid);
        if (it != Resources.end())
        {
            return dynamic_cast<T*>(&it->second);
        }
        return nullptr;
    }
};


#endif