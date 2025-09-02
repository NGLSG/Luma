#include "RuntimePrefabManager.h"
#include <imgui.h>

sk_sp<RuntimePrefab> RuntimePrefabManager::TryGetAsset(const Guid& guid) const
{
    auto asset = RuntimeAssetManagerBase<RuntimePrefab>::TryGetAsset(guid);
    if (asset)
    {
        m_performanceData.cacheHits++;
    }
    else
    {
        m_performanceData.cacheMisses++;
    }
    return asset;
}

bool RuntimePrefabManager::TryAddOrUpdateAsset(const Guid& guid, sk_sp<RuntimePrefab> asset)
{
    if (!RuntimeAssetManagerBase<RuntimePrefab>::TryAddOrUpdateAsset(guid, asset))
    {
        return false;
    }
    m_performanceData.prefabCount = m_assets.size();
    return true;
}

bool RuntimePrefabManager::TryRemoveAsset(const Guid& guid)
{
    if (!RuntimeAssetManagerBase<RuntimePrefab>::TryRemoveAsset(guid))
    {
        return false;
    }
    m_performanceData.prefabCount = m_assets.size();
    return true;
}

bool RuntimePrefabManager::TryGetAsset(const Guid& guid, sk_sp<RuntimePrefab>& out) const
{
    bool found = RuntimeAssetManagerBase<RuntimePrefab>::TryGetAsset(guid, out);

    if (found)
    {
        m_performanceData.cacheHits++;
    }
    else
    {
        m_performanceData.cacheMisses++;
    }

    return found;
}

const AssetPerformanceData* RuntimePrefabManager::GetPerformanceData() const
{
    return &m_performanceData;
}

void RuntimePrefabManager::DrawDebugUI()
{
    if (!ImGui::Begin("Runtime Prefab Manager"))
    {
        ImGui::End();
        return;
    }

    ImGui::Text("Cached Prefabs: %d", m_performanceData.prefabCount);
    ImGui::Separator();
    ImGui::Text("Cache Hits: %d", m_performanceData.cacheHits);
    ImGui::Text("Cache Misses: %d", m_performanceData.cacheMisses);

    if (ImGui::CollapsingHeader("Loaded Prefabs"))
    {
        for (const auto& [guid, asset] : m_assets)
        {
            if (asset)
            {
                ImGui::Text("%s | Name: %s",
                            guid.c_str(),
                            asset->GetData().root.name.c_str()
                );
            }
        }
    }
    ImGui::End();
}
