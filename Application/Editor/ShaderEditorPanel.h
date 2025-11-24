#pragma once

#include "IEditorPanel.h"
#include "AssetHandle.h"
#include "MaterialData.h"
#include "ShaderData.h"
#include "TextEditor.h"
#include "Nut/Shader.h" // 引入 Nut Shader 模块
#include "imgui.h"
#include <string>
#include <memory>
#include <vector>

// 前置声明
class EditorContext;

/**
 * @brief 着色器编辑器面板
 * * 提供可视化的WGSL/SkSL着色器编辑界面，集成以下功能：
 * - 语法高亮与自定义配色
 * - 智能代码自动补全 (IntelliSense风格)
 * - 基于 Nut 引擎的实时编译验证
 * - Shader 反射信息查看
 */
class ShaderEditorPanel : public IEditorPanel
{
public:
    ShaderEditorPanel();
    ~ShaderEditorPanel() override = default;

    /**
     * @brief 初始化面板
     * @param context 编辑器上下文，用于访问图形后端
     */
    void Initialize(EditorContext* context) override;

    /**
     * @brief 更新每帧逻辑
     */
    void Update(float deltaTime) override;

    /**
     * @brief 绘制面板UI
     */
    void Draw() override;

    /**
     * @brief 关闭并清理资源
     */
    void Shutdown() override;

    /**
     * @brief 获取面板标题
     */
    const char* GetPanelName() const override { return "着色器编辑器"; }

    /**
     * @brief 打开Shader资产进行编辑
     * @param shaderHandle Shader资源句柄
     */
    void OpenShader(const AssetHandle& shaderHandle);

    /**
     * @brief 保存当前编辑的Shader
     */
    void SaveShader();

    /**
     * @brief 检查是否有未保存的更改
     */
    bool HasUnsavedChanges() const { return m_hasUnsavedChanges; }

    // --- 已弃用的接口 (保留兼容性) ---
    void OpenMaterial(const AssetHandle& materialHandle);
    void SaveMaterial();

private:
    // --- UI 渲染方法 ---
    void RenderToolbar();
    void RenderCodeEditor();
    void RenderBindingsPanel();
    void RenderCompileOutput();
    void RenderSettingsPanel();

    // [Obsolete] 旧版 Uniform 编辑器
    void RenderUniformEditor();

    // --- 核心功能 ---

    /**
     * @brief 编译着色器
     * 使用 Nut::ShaderManager 进行预编译，验证语法并提取反射数据
     */
    void CompileShader();

    // --- 自动补全系统 ---

    /**
     * @brief 处理自动补全的输入逻辑
     * 在 TextEditor 处理输入前调用，拦截特定按键
     */
    void HandleAutoComplete();

    /**
     * @brief 渲染自动补全的悬浮列表
     */
    void RenderAutoCompletePopup();

    /**
     * @brief 获取光标左侧的单词前缀
     */
    std::string GetWordUnderCursor() const;

    /**
     * @brief 提取当前文件中的局部变量
     */
    std::vector<std::string> ExtractLocalVariables() const;

    // --- 配置与状态管理 ---
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

    // [Obsolete] 辅助函数
    void AddUniform();
    void RemoveUniform(size_t index);

    // --- 成员变量 ---

    EditorContext* m_context = nullptr;

    // 资产数据
    AssetHandle m_currentShaderHandle;
    Data::ShaderData m_shaderData;
    AssetHandle m_currentMaterialHandle; // [Obsolete]
    Data::MaterialDefinition m_materialData; // [Obsolete]

    // 面板状态
    bool m_isOpen = false;
    bool m_isVisible = true;
    bool m_hasUnsavedChanges = false;
    bool m_showSettingsPanel = false;

    // 编译状态
    bool m_compileSuccess = false;
    std::string m_compileOutput;

    // 代码编辑器核心
    TextEditor m_textEditor;
    std::string m_shaderCodeBuffer;
    bool m_codeChanged = false;

    // 自动补全状态
    enum class CandidateType
    {
        Keyword,    // K - 关键字 (蓝色)
        Function,   // F - 函数 (黄色)
        Module,     // M - 模块 (绿色)
        Type,       // T - 类型 (青色)
        Variable    // V - 变量 (橙色)
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

    // Shader 反射数据
    std::vector<Nut::ShaderBindingInfo> m_shaderBindings;
    bool m_bindingsDirty = true;

    // 用户配置
    float m_fontSize = 16.0f;
    const float m_fontSizeMin = 8.0f;
    const float m_fontSizeMax = 48.0f;
    TextEditor::Palette m_customPalette;
    bool m_useCustomColors = false;
    std::vector<std::string> m_customKeywords;
    std::string m_newKeywordBuffer;

    // [Obsolete] 旧版 Uniform 编辑状态
    int m_selectedUniformIndex = -1;
    bool m_addingUniform = false;
    Data::MaterialUniform m_newUniform;
};