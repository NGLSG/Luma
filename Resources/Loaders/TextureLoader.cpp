#include "TextureLoader.h"
#include "../../Renderer/GraphicsBackend.h"
#include "../AssetManager.h"
#include "../Managers/RuntimeTextureManager.h"
#include "TextureImporterSettings.h"


sk_sp<RuntimeTexture> TextureLoader::LoadAsset(const AssetMetadata& metadata)
{
    if (metadata.type != AssetType::Texture || !metadata.importerSettings)
    {
        return nullptr;
    }


    TextureImporterSettings settings = metadata.importerSettings.as<TextureImporterSettings>();
    YAML::Binary binaryData = settings.rawData;


    if (binaryData.size() == 0 && metadata.importerSettings["encodedData"])
    {
        binaryData = metadata.importerSettings["encodedData"].as<YAML::Binary>();
    }


    if (binaryData.size() == 0)
    {
        return nullptr;
    }
    auto nutCtx = backend.GetNutContext();
    sk_sp<SkData> skData = SkData::MakeWithCopy(binaryData.data(), binaryData.size());
    if (!skData) return nullptr;


    sk_sp<SkImage> image = backend.CreateSpriteImageFromData(skData);
    if (!image) return nullptr;
    Nut::TextureAPtr texture;
    if (nutCtx)
    {
        texture = nutCtx->CreateTextureFromMemory(skData->data(), skData->size());
    }


    return sk_make_sp<RuntimeTexture>(metadata.guid, std::move(image), settings, std::move(texture));
}


sk_sp<RuntimeTexture> TextureLoader::LoadAsset(const Guid& guid)
{
    const AssetMetadata* metadata = AssetManager::GetInstance().GetMetadata(guid);
    if (!metadata || metadata->type != AssetType::Texture)
    {
        LogInfo("尝试加载纹理时未找到有效的元数据或类型不匹配: {}", guid.ToString());
        return nullptr;
    }

    sk_sp<RuntimeTexture> runtimeTexture;
    if (RuntimeTextureManager::GetInstance().TryGetAsset(guid, runtimeTexture))
    {
        return runtimeTexture;
    }

    runtimeTexture = LoadAsset(*metadata);
    if (runtimeTexture)
    {
        RuntimeTextureManager::GetInstance().TryAddOrUpdateAsset(guid, runtimeTexture);
    }
    return runtimeTexture;
}
