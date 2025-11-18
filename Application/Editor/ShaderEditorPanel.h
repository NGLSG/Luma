#pragma once

#include "IEditorPanel.h"
#include "AssetHandle.h"
#include "MaterialData.h"
#include "TextEditor.h"
#include "Shader.h"
#include <string>
#include <memory>
#include <vector>

#include "Nut/Shader.h"

/**
 * @brief Shader编辑器面板
 * 
 * 提供可视化的WGSL/SkSL着色器编辑界面，包括：
 * - 语法高亮
 * - 代码编辑
 * - 实时编译验证
 * - Uniform参数编辑
 */
class ShaderEditorPanel : public IEditorPanel
{
public:
    ShaderEditorPanel();
    ~ShaderEditorPanel() override = default;

    /**
     * @brief 初始化面板
     */
    void Initialize(EditorContext* context) override;

    /**
     * @brief 更新面板状态
     */
    void Update(float deltaTime) override;

    /**
     * @brief 绘制面板UI
     */
    void Draw() override;

    /**
     * @brief 关闭面板
     */
    void Shutdown() override;

    /**
     * @brief 获取面板名称
     */
    const char* GetPanelName() const override { return "着色器编辑器"; }

    /**
     * @brief 打开材质进行编辑
     * @param materialHandle 材质资源句柄
     */
    void OpenMaterial(const AssetHandle& materialHandle);

    /**
     * @brief 保存当前编辑的材质
     */
    void SaveMaterial();

    /**
     * @brief 检查是否有未保存的更改
     */
    bool HasUnsavedChanges() const { return m_hasUnsavedChanges; }

private:
    /**
     * @brief 渲染工具栏
     */
    void RenderToolbar();

    /**
     * @brief 渲染代码编辑器
     */
    void RenderCodeEditor();

    /**
     * @brief 渲染Binding信息面板
     */
    void RenderBindingsPanel();

    /**
     * @brief 渲染Uniform编辑器（已弃用，保留兼容）
     */
    void RenderUniformEditor();

    /**
     * @brief 渲染编译输出
     */
    void RenderCompileOutput();

    /**
     * @brief 编译着色器并验证
     */
    void CompileShader();

    /**
     * @brief 添加新的Uniform
     */
    void AddUniform();

    /**
     * @brief 删除Uniform
     * @param index Uniform索引
     */
    void RemoveUniform(size_t index);

    AssetHandle m_currentMaterialHandle;
    Data::MaterialDefinition m_materialData;
    
    // 编辑器状态
    bool m_isOpen = false;
    bool m_hasUnsavedChanges = false;
    bool m_compileSuccess = false;
    std::string m_compileOutput;
    
    // 代码编辑
    TextEditor m_textEditor;  // 语法高亮的代码编辑器
    std::string m_shaderCodeBuffer;
    bool m_codeChanged = false;
    
    // Uniform编辑
    int m_selectedUniformIndex = -1;
    bool m_addingUniform = false;
    Data::MaterialUniform m_newUniform;
    
    /**
     * @brief 更新TextEditor的语言定义
     */
    void UpdateTextEditorLanguage();

    /**
     * @brief 解析Shader并提取binding信息
     */
    void ParseShaderBindings();

    /**
     * @brief 处理字体缩放
     */
    void HandleFontZoom();

    /**
     * @brief 加载字体大小设置
     */
    void LoadFontSize();

    /**
     * @brief 保存字体大小设置
     */
    void SaveFontSize();

    /**
     * @brief 渲染颜色和关键字配置面板
     */
    void RenderSettingsPanel();

    /**
     * @brief 加载颜色配置
     */
    void LoadColorSettings();

    /**
     * @brief 保存颜色配置
     */
    void SaveColorSettings();

    /**
     * @brief 加载自定义关键字
     */
    void LoadCustomKeywords();

    /**
     * @brief 保存自定义关键字
     */
    void SaveCustomKeywords();

    /**
     * @brief 应用颜色配置到TextEditor
     */
    void ApplyColorSettings();

    /**
     * @brief 应用自定义关键字到TextEditor
     */
    void ApplyCustomKeywords();

    // Shader反射信息
    std::vector<Nut::ShaderBindingInfo> m_shaderBindings;
    bool m_bindingsDirty = true;  // binding需要更新

    // 字体缩放
    float m_fontSize = 16.0f;  // 默认字体大小
    float m_fontSizeMin = 8.0f;
    float m_fontSizeMax = 48.0f;

    // 颜色配置
    TextEditor::Palette m_customPalette;
    bool m_useCustomColors = false;
    bool m_showSettingsPanel = false;

    // 自定义关键字
    std::vector<std::string> m_customKeywords;
    std::string m_newKeywordBuffer;
};
