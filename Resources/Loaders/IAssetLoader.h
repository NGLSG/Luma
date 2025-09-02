#ifndef IASSETLOADER_H
#define IASSETLOADER_H
#include "../AssetMetadata.h"
#include "include/core/SkRefCnt.h"

/**
 * @brief 资产加载器接口。
 *
 * 定义了加载特定类型资产的方法。这是一个模板类，允许加载不同类型的资产。
 *
 * @tparam T 资产的类型。
 */
template <typename T>
class IAssetLoader
{
public:
    /**
     * @brief 虚析构函数。
     *
     * 确保派生类对象能正确销毁。
     */
    virtual ~IAssetLoader() = default;

    /**
     * @brief 根据资产元数据加载资产。
     *
     * @param metadata 包含资产信息的元数据对象。
     * @return 指向加载的资产的智能指针。如果加载失败，可能返回空指针。
     */
    virtual sk_sp<T> LoadAsset(const AssetMetadata& metadata) = 0;

    /**
     * @brief 根据全局唯一标识符 (GUID) 加载资产。
     *
     * @param Guid 资产的全局唯一标识符。
     * @return 指向加载的资产的智能指针。如果加载失败，可能返回空指针。
     */
    virtual sk_sp<T> LoadAsset(const Guid& Guid) = 0;
};


#endif