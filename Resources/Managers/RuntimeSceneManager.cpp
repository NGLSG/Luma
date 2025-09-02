#include "RuntimeSceneManager.h"
#include <imgui.h>

sk_sp<RuntimeScene> RuntimeSceneManager::TryGetAsset(const Guid& guid) const
{
    auto asset = RuntimeAssetManagerBase<RuntimeScene>::TryGetAsset(guid);
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

bool RuntimeSceneManager::TryGetAsset(const Guid& guid, sk_sp<RuntimeScene>& out) const
{
    bool found = RuntimeAssetManagerBase<RuntimeScene>::TryGetAsset(guid, out);

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

bool RuntimeSceneManager::TryAddOrUpdateAsset(const Guid& guid, sk_sp<RuntimeScene> asset)
{
    if (!RuntimeAssetManagerBase<RuntimeScene>::TryAddOrUpdateAsset(guid, asset))
    {
        return false;
    }
    m_performanceData.sceneCount = m_assets.size();
    return true;
}

bool RuntimeSceneManager::TryRemoveAsset(const Guid& guid)
{
    if (!RuntimeAssetManagerBase<RuntimeScene>::TryRemoveAsset(guid))
    {
        return false;
    }
    m_performanceData.sceneCount = m_assets.size();
    return true;
}

const AssetPerformanceData* RuntimeSceneManager::GetPerformanceData() const
{
    return &m_performanceData;
}

void RuntimeSceneManager::DrawDebugUI()
{
    if (!ImGui::Begin("Runtime Scene Manager"))
    {
        ImGui::End();
        return;
    }

    ImGui::Text("Cached Scenes: %d", m_performanceData.sceneCount);
    ImGui::Separator();
    ImGui::Text("Cache Hits: %d", m_performanceData.cacheHits);
    ImGui::Text("Cache Misses: %d", m_performanceData.cacheMisses);

    if (ImGui::CollapsingHeader("Loaded Scenes"))
    {
        for (const auto& [guid, asset] : m_assets)
        {
            if (asset)
            {
                ImGui::Text("%s", guid.c_str());
            }
        }
    }
    ImGui::End();
}
