#include "RuntimeFontManager.h"
#include <imgui.h>

#include "EventBus.h"
#include "Event/Events.h"

RuntimeFontManager::RuntimeFontManager()
{
    m_onAssetUpdatedHandle = EventBus::GetInstance().Subscribe<AssetUpdatedEvent>([this](const AssetUpdatedEvent& e)
    {
        OnAssetUpdated(e);
    });
}

RuntimeFontManager::~RuntimeFontManager()
{
    EventBus::GetInstance().Unsubscribe(m_onAssetUpdatedHandle);
}

sk_sp<SkTypeface> RuntimeFontManager::TryGetAsset(const Guid& guid) const
{
    auto asset = RuntimeAssetManagerBase<SkTypeface>::TryGetAsset(guid);

    if (asset)
    {
        m_lruTracker.splice(m_lruTracker.begin(), m_lruTracker, m_lruMap.at(guid));
        m_performanceData.cacheHits++;
    }
    else
    {
        m_performanceData.cacheMisses++;
    }

    return asset;
}

bool RuntimeFontManager::TryGetAsset(const Guid& guid, sk_sp<SkTypeface>& out) const
{
    if (RuntimeAssetManagerBase<SkTypeface>::TryGetAsset(guid, out))
    {
        m_lruTracker.splice(m_lruTracker.begin(), m_lruTracker, m_lruMap.at(guid));
        m_performanceData.cacheHits++;
        return true;
    }
    else
    {
        m_performanceData.cacheMisses++;
        return false;
    }
}

bool RuntimeFontManager::TryAddOrUpdateAsset(const Guid& guid, sk_sp<SkTypeface> asset)
{
    const bool isNewAsset = !m_assets.count(guid);

    if (!RuntimeAssetManagerBase<SkTypeface>::TryAddOrUpdateAsset(guid, asset))
    {
        return false;
    }

    m_performanceData.fontCount = m_assets.size();

    if (m_lruMap.count(guid))
    {
        m_lruTracker.erase(m_lruMap[guid]);
    }
    m_lruTracker.push_front(guid);
    m_lruMap[guid] = m_lruTracker.begin();

    EnforceBudget();
    return true;
}

bool RuntimeFontManager::TryRemoveAsset(const Guid& guid)
{
    if (!RuntimeAssetManagerBase::TryRemoveAsset(guid))
    {
        return false;
    }

    m_performanceData.fontCount = m_assets.size();

    if (m_lruMap.count(guid))
    {
        m_lruTracker.erase(m_lruMap[guid]);
        m_lruMap.erase(guid);
    }
    return true;
}

void RuntimeFontManager::EnforceBudget()
{
    while (!m_lruTracker.empty() &&
        ((m_performanceData.maxFontCount != -1 && m_assets.size() > m_performanceData.maxFontCount)))
    {
        Guid toEvict = m_lruTracker.back();
        TryRemoveAsset(toEvict);
        m_performanceData.evictions++;
    }
}

const AssetPerformanceData* RuntimeFontManager::GetPerformanceData() const
{
    return &m_performanceData;
}

void RuntimeFontManager::DrawDebugUI()
{
    if (!ImGui::Begin("Runtime Font Manager"))
    {
        ImGui::End();
        return;
    }

    ImGui::Text("Count: %d / %d", m_performanceData.fontCount,
                m_performanceData.maxFontCount > 0 ? m_performanceData.maxFontCount : -1);

    ImGui::Separator();
    ImGui::Text("Cache Hits: %d", m_performanceData.cacheHits);
    ImGui::Text("Cache Misses: %d", m_performanceData.cacheMisses);
    ImGui::Text("Evictions: %d", m_performanceData.evictions);

    if (ImGui::CollapsingHeader("Loaded Fonts"))
    {
        for (const auto& guid : m_lruTracker)
        {
            const sk_sp<SkTypeface>& asset = m_assets.at(guid);
            if (asset)
            {
                SkString familyName;
                asset->getFamilyName(&familyName);
                ImGui::Text("%s | %s",
                            guid.ToString().c_str(),
                            familyName.c_str());
            }
        }
    }
    ImGui::End();
}

void RuntimeFontManager::SetMemoryBudget(float mb)
{
    m_performanceData.memoryBudgetBytes = static_cast<size_t>(mb * 1024 * 1024);
    EnforceBudget();
}

void RuntimeFontManager::SetMaxFontCount(int count)
{
    m_performanceData.maxFontCount = count;
    EnforceBudget();
}

void RuntimeFontManager::OnAssetUpdated(const AssetUpdatedEvent& e)
{
    if (e.assetType == AssetType::Font)
    {
        
        TryRemoveAsset(e.guid);
    }
}
