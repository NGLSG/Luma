#ifndef LUMAENGINE_BLUEPRINTLOADER_H
#define LUMAENGINE_BLUEPRINTLOADER_H

#include "IAssetLoader.h"
#include "Data/BlueprintData.h"
#include "RuntimeAsset/RuntimeBlueprint.h"

/**
 * @brief Blueprint数据加载器
 *
 * 负责从资产元数据加载蓝图描述符文件（.blueprint）以供编辑器使用。
 */
class BlueprintLoader : public IAssetLoader<RuntimeBlueprint>
{
public:
    sk_sp<RuntimeBlueprint> LoadAsset(const AssetMetadata& metadata) override;
    sk_sp<RuntimeBlueprint> LoadAsset(const Guid& Guid) override;
};

#endif // LUMAENGINE_BLUEPRINTLOADER_H