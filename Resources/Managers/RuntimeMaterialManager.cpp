#include "RuntimeMaterialManager.h"
#include <imgui.h>

sk_sp<Material> RuntimeMaterialManager::TryGetAsset(const Guid& guid) const
{
    auto asset = RuntimeAssetManagerBase<Material>::TryGetAsset(guid);

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

bool RuntimeMaterialManager::TryGetAsset(const Guid& guid, sk_sp<Material>& out) const
{
    bool found = RuntimeAssetManagerBase<Material>::TryGetAsset(guid, out);

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

bool RuntimeMaterialManager::TryAddOrUpdateAsset(const Guid& guid, sk_sp<Material> asset)
{
    if (!RuntimeAssetManagerBase<Material>::TryAddOrUpdateAsset(guid, asset))
    {
        return false;
    }

    m_performanceData.materialCount = m_assets.size();
    return true;
}

bool RuntimeMaterialManager::TryRemoveAsset(const Guid& guid)
{
    if (!RuntimeAssetManagerBase<Material>::TryRemoveAsset(guid))
    {
        return false;
    }

    m_performanceData.materialCount = m_assets.size();
    return true;
}

const AssetPerformanceData* RuntimeMaterialManager::GetPerformanceData() const
{
    return &m_performanceData;
}

void RuntimeMaterialManager::DrawDebugUI()
{
    if (!ImGui::Begin("Runtime Material Manager"))
    {
        ImGui::End();
        return;
    }


    ImGui::Text("Cached Materials: %d", m_performanceData.materialCount);
    ImGui::Separator();
    ImGui::Text("Cache Hits: %d", m_performanceData.cacheHits);
    ImGui::Text("Cache Misses: %d", m_performanceData.cacheMisses);


    if (ImGui::CollapsingHeader("Loaded Materials"))
    {
        for (const auto& [guid, asset] : m_assets)
        {
            if (asset && asset->effect)
            {
                ImGui::Text("%s | Uniforms: %zu",
                            guid.c_str(),
                            asset->uniforms.size());
            }
        }
    }
    ImGui::End();
}
