#ifndef LUMAENGINE_RULETILELOADER_H
#define LUMAENGINE_RULETILELOADER_H
#include "IAssetLoader.h"
#include "RuntimeAsset/RuntimeRuleTile.h"

/**
 * @brief 规则瓦片资源加载器。
 *
 * 该类负责从各种源加载和管理 RuntimeRuleTile 类型的资产。
 */
class RuleTileLoader : public IAssetLoader<RuntimeRuleTile>
{
public:
    /**
     * @brief 根据资产元数据加载规则瓦片资产。
     *
     * @param metadata 资产的元数据信息。
     * @return sk_sp<RuntimeRuleTile> 智能指针指向加载的运行时规则瓦片资产。
     */
    sk_sp<RuntimeRuleTile> LoadAsset(const AssetMetadata& metadata) override;

    /**
     * @brief 根据全局唯一标识符 (GUID) 加载规则瓦片资产。
     *
     * @param Guid 资产的全局唯一标识符。
     * @return sk_sp<RuntimeRuleTile> 智能指针指向加载的运行时规则瓦片资产。
     */
    sk_sp<RuntimeRuleTile> LoadAsset(const Guid& Guid) override;
};


#endif