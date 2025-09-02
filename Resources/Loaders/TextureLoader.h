/**
 * @file TextureLoader.h
 * @brief 定义了纹理加载器类，用于加载运行时纹理。
 */

#ifndef TEXTURELOADER_H
#define TEXTURELOADER_H

#include "IAssetLoader.h"
#include "yaml-cpp/yaml.h"
#include "include/core/SkData.h"
#include "include/core/SkImage.h"
#include "RuntimeAsset/RuntimeTexture.h"

class GraphicsBackend;

/**
 * @brief 纹理加载器，负责从元数据或GUID加载运行时纹理。
 *
 * 该类实现了IAssetLoader接口，专门用于加载RuntimeTexture类型的资产。
 */
class TextureLoader : IAssetLoader<RuntimeTexture>
{
private:
    /**
     * @brief 获取纹理数据。
     * @param textureMetadata 纹理的资产元数据。
     * @return 纹理数据的智能指针。
     */
    static sk_sp<SkData> GetTextureData(const AssetMetadata& textureMetadata);

    GraphicsBackend& backend; ///< 图形后端引用。

public:
    /**
     * @brief 构造函数，初始化纹理加载器。
     * @param backend 图形后端引用。
     */
    explicit TextureLoader(GraphicsBackend& backend) : backend(backend)
    {
    }

    /**
     * @brief 根据资产元数据加载纹理资产。
     * @param metadata 纹理的资产元数据。
     * @return 运行时纹理的智能指针。
     */
    sk_sp<RuntimeTexture> LoadAsset(const AssetMetadata& metadata) override;

    /**
     * @brief 根据全局唯一标识符（GUID）加载纹理资产。
     * @param Guid 纹理的全局唯一标识符。
     * @return 运行时纹理的智能指针。
     */
    sk_sp<RuntimeTexture> LoadAsset(const Guid& Guid) override;
};


#endif