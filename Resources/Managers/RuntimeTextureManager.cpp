#include "RuntimeTextureManager.h"
#include <imgui.h>

sk_sp<RuntimeTexture> RuntimeTextureManager::TryGetAsset(const Guid& guid) const
{
    auto asset = RuntimeAssetManagerBase<RuntimeTexture>::TryGetAsset(guid);
    if (asset)
    {
        m_lruTracker.splice(m_lruTracker.begin(), m_lruTracker, m_lruMap.at(guid.ToString()));
        m_performanceData.cacheHits++;
    }
    else
    {
        m_performanceData.cacheMisses++;
    }
    return asset;
}

bool RuntimeTextureManager::TryGetAsset(const Guid& guid, sk_sp<RuntimeTexture>& out) const
{
    if (RuntimeAssetManagerBase<RuntimeTexture>::TryGetAsset(guid, out))
    {
        m_lruTracker.splice(m_lruTracker.begin(), m_lruTracker, m_lruMap.at(guid.ToString()));
        m_performanceData.cacheHits++;
        return true;
    }
    else
    {
        m_performanceData.cacheMisses++;
        return false;
    }
}

bool RuntimeTextureManager::TryAddOrUpdateAsset(const Guid& guid, sk_sp<RuntimeTexture> asset)
{
    const bool isNewAsset = !m_assets.count(guid.ToString());
    size_t oldSize = 0;

    if (!isNewAsset && m_assets.at(guid.ToString()) && m_assets.at(guid.ToString())->getImage())
    {
        oldSize = m_assets.at(guid.ToString())->getImage()->imageInfo().computeMinByteSize();
    }

    if (!RuntimeAssetManagerBase<RuntimeTexture>::TryAddOrUpdateAsset(guid, asset))
    {
        return false;
    }

    m_performanceData.memoryUsageBytes -= oldSize;
    if (asset && asset->getImage())
    {
        m_performanceData.memoryUsageBytes += asset->getImage()->imageInfo().computeMinByteSize();
    }
    m_performanceData.textureCount = m_assets.size();

    if (m_lruMap.count(guid.ToString()))
    {
        m_lruTracker.erase(m_lruMap[guid.ToString()]);
    }
    m_lruTracker.push_front(guid);
    m_lruMap[guid.ToString()] = m_lruTracker.begin();

    EnforceBudget();
    return true;
}

bool RuntimeTextureManager::TryRemoveAsset(const Guid& guid)
{
    if (m_assets.count(guid.ToString()))
    {
        auto& asset = m_assets[guid.ToString()];
        if (asset && asset->getImage())
        {
            m_performanceData.memoryUsageBytes -= asset->getImage()->imageInfo().computeMinByteSize();
        }
    }

    if (!RuntimeAssetManagerBase::TryRemoveAsset(guid))
    {
        return false;
    }

    m_performanceData.textureCount = m_assets.size();

    if (m_lruMap.count(guid.ToString()))
    {
        m_lruTracker.erase(m_lruMap[guid.ToString()]);
        m_lruMap.erase(guid.ToString());
    }
    return true;
}

void RuntimeTextureManager::EnforceBudget()
{
    while (!m_lruTracker.empty() &&
        ((m_performanceData.maxTextureCount != -1 && m_assets.size() > (size_t)m_performanceData.maxTextureCount) ||
            (m_performanceData.memoryBudgetBytes != 0 && m_performanceData.memoryUsageBytes > m_performanceData.
                memoryBudgetBytes)))
    {
        Guid toEvict = m_lruTracker.back();
        TryRemoveAsset(toEvict);
        m_performanceData.evictions++;
    }
}

void RuntimeTextureManager::OnAssetUpdated(const AssetUpdatedEvent& e)
{
    if (e.assetType == AssetType::Texture)
    {
        TryRemoveAsset(e.guid);
    }
}

const AssetPerformanceData* RuntimeTextureManager::GetPerformanceData() const
{
    return &m_performanceData;
}

void RuntimeTextureManager::DrawDebugUI()
{
    if (!ImGui::Begin("Runtime Texture Manager"))
    {
        ImGui::End();
        return;
    }


    ImGui::Text("Count: %d / %d", m_performanceData.textureCount,
                m_performanceData.maxTextureCount > 0 ? m_performanceData.maxTextureCount : -1);
    ImGui::Text("Memory: %.2f MB / %.2f MB", m_performanceData.memoryUsageBytes / 1024.0 / 1024.0,
                m_performanceData.memoryBudgetBytes > 0
                    ? m_performanceData.memoryBudgetBytes / 1024.0 / 1024.0
                    : -1);


    ImGui::Separator();
    ImGui::Text("Cache Hits: %d", m_performanceData.cacheHits);
    ImGui::Text("Cache Misses: %d", m_performanceData.cacheMisses);
    ImGui::Text("Evictions: %d", m_performanceData.evictions);


    if (ImGui::CollapsingHeader("Loaded Textures"))
    {
        for (const auto& guid : m_lruTracker)
        {
            const sk_sp<RuntimeTexture>& asset = m_assets.at(guid.ToString());
            if (asset && asset->getImage())
            {
                const auto& image = asset->getImage();
                ImGui::Text("%s | %dx%d | %.2f KB",
                            guid.ToString().c_str(),
                            image->width(),
                            image->height(),
                            image->imageInfo().computeMinByteSize() / 1024.0);
            }
        }
    }
    ImGui::End();
}

void RuntimeTextureManager::SetMemoryBudget(float mb)
{
    m_performanceData.memoryBudgetBytes = static_cast<size_t>(mb * 1024 * 1024);
    EnforceBudget();
}

void RuntimeTextureManager::SetMaxTextureCount(int count)
{
    m_performanceData.maxTextureCount = count;
    EnforceBudget();
}

RuntimeTextureManager::~RuntimeTextureManager()
{
    EventBus::GetInstance().Unsubscribe(m_onAssetUpdatedHandle);
}

RuntimeTextureManager::RuntimeTextureManager()
{
    m_onAssetUpdatedHandle = EventBus::GetInstance().Subscribe<AssetUpdatedEvent>([this](const AssetUpdatedEvent& e)
    {
        OnAssetUpdated(e);
    });
}
