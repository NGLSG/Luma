#pragma once
#include "IEditorPanel.h"
#include "AssetHandle.h"
#include "MaterialData.h"
#include "ShaderData.h"
#include "TextEditor.h"
#include "Nut/Shader.h" 
#include "imgui.h"
#include <string>
#include <memory>
#include <vector>
class EditorContext;
class ShaderEditorPanel : public IEditorPanel
{
public:
    ShaderEditorPanel();
    ~ShaderEditorPanel() override = default;
    void Initialize(EditorContext* context) override;
    void Update(float deltaTime) override;
    void Draw() override;
    void Shutdown() override;
    const char* GetPanelName() const override { return "着色器编辑器"; }
    void OpenShader(const AssetHandle& shaderHandle);
    void SaveShader();
    bool HasUnsavedChanges() const { return m_hasUnsavedChanges; }
    void OpenMaterial(const AssetHandle& materialHandle);
    void SaveMaterial();
private:
    void RenderToolbar();
    void RenderCodeEditor();
    void RenderBindingsPanel();
    void RenderCompileOutput();
    void RenderSettingsPanel();
    void RenderUniformEditor();
    void CompileShader();
    void HandleAutoComplete();
    void RenderAutoCompletePopup();
    std::string GetWordUnderCursor() const;
    std::vector<std::string> ExtractLocalVariables() const;
    void UpdateTextEditorLanguage();
    void HandleFontZoom();
    void LoadFontSize();
    void SaveFontSize();
    void LoadColorSettings();
    void SaveColorSettings();
    void LoadCustomKeywords();
    void SaveCustomKeywords();
    void ApplyColorSettings();
    void ApplyCustomKeywords();
    void AddUniform();
    void RemoveUniform(size_t index);
    EditorContext* m_context = nullptr;
    AssetHandle m_currentShaderHandle;
    Data::ShaderData m_shaderData;
    AssetHandle m_currentMaterialHandle; 
    Data::MaterialDefinition m_materialData; 
    bool m_isOpen = false;
    bool m_isVisible = true;
    bool m_hasUnsavedChanges = false;
    bool m_showSettingsPanel = false;
    bool m_compileSuccess = false;
    std::string m_compileOutput;
    TextEditor m_textEditor;
    std::string m_shaderCodeBuffer;
    bool m_codeChanged = false;
    enum class CandidateType
    {
        Keyword,    
        Function,   
        Module,     
        Type,       
        Variable    
    };
    struct AutoCompleteCandidate
    {
        std::string text;
        CandidateType type;
    };
    bool m_isAutoCompleteOpen = false;
    std::vector<AutoCompleteCandidate> m_autoCompleteCandidates;
    int m_autoCompleteSelectedIndex = 0;
    std::string m_currentWordPrefix;
    ImVec2 m_popupPos;
    std::vector<Nut::ShaderBindingInfo> m_shaderBindings;
    bool m_bindingsDirty = true;
    float m_fontSize = 16.0f;
    const float m_fontSizeMin = 8.0f;
    const float m_fontSizeMax = 48.0f;
    TextEditor::Palette m_customPalette;
    bool m_useCustomColors = false;
    std::vector<std::string> m_customKeywords;
    std::string m_newKeywordBuffer;
    int m_selectedUniformIndex = -1;
    bool m_addingUniform = false;
    Data::MaterialUniform m_newUniform;
};
