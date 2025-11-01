#ifndef LUMAENGINE_RULETILEPANEL_H
#define LUMAENGINE_RULETILEPANEL_H

#pragma once
#include "IEditorPanel.h"
#include "EditorContext.h"
#include "Data/RuleTile.h"
#include <vector>

/**
 * @brief 规则瓦片编辑器面板。
 */
class RuleTilePanel : public IEditorPanel
{
public:
    RuleTilePanel() = default;
    ~RuleTilePanel() override = default;

    void Initialize(EditorContext* context) override;
    void Update(float deltaTime) override;
    void Draw() override;
    void Shutdown() override;

    const char* GetPanelName() const override { return "规则瓦片编辑器"; }

private:
    void openRuleTile(Guid guid);
    void closeCurrentRuleTile();
    void saveCurrentRuleTile();

    void drawRuleList();

private:
    EditorContext* m_context = nullptr;
    Guid m_currentRuleTileGuid;
    RuleTileAssetData m_editingData;
};

#endif

