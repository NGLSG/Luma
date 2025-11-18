#include "ShaderEditorPanel.h"
#include "AssetManager.h"
#include "Logger.h"
#include "Renderer/Nut/NutContext.h"
#include "imgui.h"
#include "imgui_stdlib.h"
#include <fstream>
#include <regex>
#include <set>

ShaderEditorPanel::ShaderEditorPanel()
{
    
    m_textEditor.SetLanguageDefinition(TextEditor::LanguageDefinition::WGSL());
    m_textEditor.SetShowWhitespaces(false);
    m_textEditor.SetImGuiChildIgnored(true);  
    
    
    m_customPalette = TextEditor::GetDarkPalette();
    
    
    LoadFontSize();
    LoadColorSettings();
    LoadCustomKeywords();
    
    
    ApplyColorSettings();
    ApplyCustomKeywords();
}

void ShaderEditorPanel::Initialize(EditorContext* context)
{
    m_context = context;
}

void ShaderEditorPanel::Update(float deltaTime)
{
    
}

void ShaderEditorPanel::Draw()
{
    if (!m_isVisible || !m_isOpen) return;

    ImGui::SetNextWindowSize(ImVec2(1200, 800), ImGuiCond_FirstUseEver);

    if (ImGui::Begin(GetPanelName(), &m_isOpen, ImGuiWindowFlags_MenuBar))
    {
        RenderToolbar();

        
        ImGui::BeginChild("##shader_editor_split", ImVec2(0, -200), false);
        {
            
            ImGui::BeginChild("##code_editor", ImVec2(ImGui::GetContentRegionAvail().x * 0.7f, 0), true);
            {
                RenderCodeEditor();
            }
            ImGui::EndChild();

            ImGui::SameLine();

            
            ImGui::BeginChild("##bindings_panel", ImVec2(0, 0), true);
            {
                RenderBindingsPanel();
            }
            ImGui::EndChild();
        }
        ImGui::EndChild();

        
        ImGui::BeginChild("##compile_output", ImVec2(0, 0), true);
        {
            RenderCompileOutput();
        }
        ImGui::EndChild();
    }
    ImGui::End();
    
    
    if (m_showSettingsPanel)
    {
        RenderSettingsPanel();
    }
}

void ShaderEditorPanel::OpenMaterial(const AssetHandle& materialHandle)
{
    if (!materialHandle.Valid())
    {
        LogError("ShaderEditorPanel::OpenMaterial - Invalid material handle");
        return;
    }

    auto metadata = AssetManager::GetInstance().GetMetadata(materialHandle.assetGuid);
    if (!metadata || metadata->type != AssetType::Material)
    {
        LogError("ShaderEditorPanel::OpenMaterial - Failed to load material metadata");
        return;
    }

    m_currentMaterialHandle = materialHandle;
    m_materialData = metadata->importerSettings.as<Data::MaterialDefinition>();
    m_shaderCodeBuffer = m_materialData.shaderCode;
    
    
    m_textEditor.SetText(m_shaderCodeBuffer);
    UpdateTextEditorLanguage();
    
    
    m_bindingsDirty = true;
    ParseShaderBindings();
    
    m_isOpen = true;
    m_isVisible = true; 
    m_hasUnsavedChanges = false;
    m_codeChanged = false;
    m_compileSuccess = false;
    m_compileOutput.clear();

    LogInfo("ShaderEditorPanel::OpenMaterial - Opened material: {}", metadata->assetPath.string());
}

void ShaderEditorPanel::SaveMaterial()
{
    if (!m_currentMaterialHandle.Valid())
    {
        LogError("ShaderEditorPanel::SaveMaterial - No material is currently open");
        return;
    }

    auto metadata = AssetManager::GetInstance().GetMetadata(m_currentMaterialHandle.assetGuid);
    if (!metadata)
    {
        LogError("ShaderEditorPanel::SaveMaterial - Failed to get material metadata");
        return;
    }

    
    m_shaderCodeBuffer = m_textEditor.GetText();
    m_materialData.shaderCode = m_shaderCodeBuffer;

    
    YAML::Node node;
    node = m_materialData;

    std::ofstream file(metadata->assetPath);
    if (!file.is_open())
    {
        LogError("ShaderEditorPanel::SaveMaterial - Failed to open file: {}", metadata->assetPath.string());
        return;
    }

    file << node;
    file.close();

    
    AssetMetadata updatedMeta = *metadata;
    updatedMeta.importerSettings = node;
    AssetManager::GetInstance().ReImport(updatedMeta);

    m_hasUnsavedChanges = false;
    m_codeChanged = false;

    LogInfo("ShaderEditorPanel::SaveMaterial - Saved material: {}", metadata->assetPath.string());
}

void ShaderEditorPanel::RenderToolbar()
{
    if (ImGui::BeginMenuBar())
    {
        if (ImGui::BeginMenu("文件"))
        {
            if (ImGui::MenuItem("保存", "Ctrl+S", nullptr, m_hasUnsavedChanges))
            {
                SaveMaterial();
            }

            if (ImGui::MenuItem("关闭"))
            {
                if (m_hasUnsavedChanges)
                {
                    // TODO: 显示保存确认对话框
                }
                m_isOpen = false;
            }

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("编辑"))
        {
            if (ImGui::MenuItem("撤销", "Ctrl+Z", nullptr, m_textEditor.CanUndo()))
            {
                m_textEditor.Undo();
            }

            if (ImGui::MenuItem("重做", "Ctrl+Y", nullptr, m_textEditor.CanRedo()))
            {
                m_textEditor.Redo();
            }

            ImGui::Separator();

            if (ImGui::MenuItem("查找", "Ctrl+F"))
            {
                // TODO: 实现查找
            }

            if (ImGui::MenuItem("替换", "Ctrl+H"))
            {
                // TODO: 实现替换
            }

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("着色器"))
        {
            if (ImGui::MenuItem("编译", "F5"))
            {
                CompileShader();
            }

            ImGui::Separator();

            
            const char* shaderTypes[] = {"SkSL (已弃用)", "WGSL"};
            int currentType = static_cast<int>(m_materialData.shaderType);
            if (ImGui::Combo("着色器类型", &currentType, shaderTypes, 2))
            {
                m_materialData.shaderType = static_cast<Data::ShaderType>(currentType);
                UpdateTextEditorLanguage();  
                m_hasUnsavedChanges = true;
                m_bindingsDirty = true;  
            }

            ImGui::EndMenu();
        }

        ImGui::EndMenuBar();
    }

    
    if (ImGui::Button("保存"))
    {
        SaveMaterial();
    }

    ImGui::SameLine();

    if (ImGui::Button("编译"))
    {
        CompileShader();
    }

    ImGui::SameLine();

    if (ImGui::Button("设置"))
    {
        m_showSettingsPanel = !m_showSettingsPanel;
    }

    ImGui::SameLine();

    ImGui::Text("字体大小: %.0f (Ctrl+滚轮)", m_fontSize);

    ImGui::SameLine();

    if (m_hasUnsavedChanges)
    {
        ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "* 未保存");
    }
    else
    {
        ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "已保存");
    }

    ImGui::Separator();
}

void ShaderEditorPanel::RenderCodeEditor()
{
    ImGui::Text("着色器代码:");
    ImGui::SameLine();

    if (m_materialData.shaderType == Data::ShaderType::WGSL)
    {
        ImGui::TextColored(ImVec4(0.3f, 0.7f, 1.0f, 1.0f), "(WGSL)");
    }
    else
    {
        ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "(SkSL - 已弃用)");
    }

    ImGui::Separator();

    
    auto cpos = m_textEditor.GetCursorPosition();
    ImGui::Text("行: %d | 列: %d | 总行数: %d | %s", 
        cpos.mLine + 1, 
        cpos.mColumn + 1, 
        m_textEditor.GetTotalLines(),
        m_textEditor.IsOverwrite() ? "改写" : "插入");

    
    ImGui::BeginChild("##code_editor_content", ImVec2(0, 0), true, 
        ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_AlwaysHorizontalScrollbar | ImGuiWindowFlags_NoMove);
    {
        
        HandleFontZoom();
        
        
        float scale = m_fontSize / 16.0f;
        ImGui::SetWindowFontScale(scale);
        
        
        static float lastScale = 1.0f;
        if (std::abs(scale - lastScale) > 0.01f)
        {
            LogInfo("Applying font scale: {} (fontSize: {})", scale, m_fontSize);
            lastScale = scale;
        }
        
        
        m_textEditor.Render("##shader_code_editor");
        
        
    }
    ImGui::EndChild();

    
    if (m_textEditor.IsTextChanged())
    {
        m_shaderCodeBuffer = m_textEditor.GetText();
        m_hasUnsavedChanges = true;
        m_codeChanged = true;
        m_bindingsDirty = true;  
    }
}

void ShaderEditorPanel::RenderUniformEditor()
{
    ImGui::Text("Uniforms");
    ImGui::Separator();

    
    for (size_t i = 0; i < m_materialData.uniforms.size(); i++)
    {
        auto& uniform = m_materialData.uniforms[i];

        ImGui::PushID(static_cast<int>(i));

        bool isSelected = (static_cast<int>(i) == m_selectedUniformIndex);
        if (ImGui::Selectable(uniform.name.c_str(), isSelected))
        {
            m_selectedUniformIndex = static_cast<int>(i);
        }

        
        if (ImGui::BeginPopupContextItem())
        {
            if (ImGui::MenuItem("Delete"))
            {
                RemoveUniform(i);
            }
            ImGui::EndPopup();
        }

        ImGui::PopID();
    }

    ImGui::Separator();

    
    if (ImGui::Button("Add Uniform"))
    {
        m_addingUniform = true;
        m_newUniform = Data::MaterialUniform{};
        m_newUniform.name = "newUniform";
        m_newUniform.type = Data::UniformType::Float;
    }

    
    if (m_addingUniform)
    {
        ImGui::OpenPopup("Add Uniform");
    }

    if (ImGui::BeginPopupModal("Add Uniform", &m_addingUniform, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::InputText("Name", &m_newUniform.name);

        const char* typeNames[] = {"Float", "Color4f", "Int", "Point", "Vec2", "Vec3", "Vec4", "Mat4"};
        int currentType = static_cast<int>(m_newUniform.type);
        if (ImGui::Combo("Type", &currentType, typeNames, IM_ARRAYSIZE(typeNames)))
        {
            m_newUniform.type = static_cast<Data::UniformType>(currentType);
        }

        ImGui::Separator();

        if (ImGui::Button("Add"))
        {
            AddUniform();
            m_addingUniform = false;
        }

        ImGui::SameLine();

        if (ImGui::Button("Cancel"))
        {
            m_addingUniform = false;
        }

        ImGui::EndPopup();
    }

    
    if (m_selectedUniformIndex >= 0 && m_selectedUniformIndex < static_cast<int>(m_materialData.uniforms.size()))
    {
        ImGui::Separator();
        ImGui::Text("Edit Uniform:");

        auto& uniform = m_materialData.uniforms[m_selectedUniformIndex];

        if (ImGui::InputText("Name", &uniform.name))
        {
            m_hasUnsavedChanges = true;
        }

        const char* typeNames[] = {"Float", "Color4f", "Int", "Point", "Vec2", "Vec3", "Vec4", "Mat4"};
        int currentType = static_cast<int>(uniform.type);
        if (ImGui::Combo("Type", &currentType, typeNames, IM_ARRAYSIZE(typeNames)))
        {
            uniform.type = static_cast<Data::UniformType>(currentType);
            m_hasUnsavedChanges = true;
        }
    }
}

void ShaderEditorPanel::RenderCompileOutput()
{
    ImGui::Text("编译输出:");
    ImGui::Separator();

    if (!m_compileOutput.empty())
    {
        if (m_compileSuccess)
        {
            ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "[成功]");
        }
        else
        {
            ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "[错误]");
        }

        ImGui::SameLine();
        ImGui::TextWrapped("%s", m_compileOutput.c_str());
    }
    else
    {
        ImGui::TextDisabled("暂无编译输出。按F5或点击'编译'按钮验证着色器。");
    }
}

void ShaderEditorPanel::CompileShader()
{
    m_compileOutput.clear();
    m_compileSuccess = false;

    
    if (m_shaderCodeBuffer.empty())
    {
        m_compileOutput = "Error: Shader code is empty.";
        return;
    }

    
    if (m_materialData.shaderType == Data::ShaderType::WGSL)
    {
        bool hasVertexEntry = m_shaderCodeBuffer.find("@vertex") != std::string::npos;
        bool hasFragmentEntry = m_shaderCodeBuffer.find("@fragment") != std::string::npos;

        if (!hasVertexEntry)
        {
            m_compileOutput = "Warning: No @vertex entry point found.";
        }

        if (!hasFragmentEntry)
        {
            if (!m_compileOutput.empty()) m_compileOutput += "\n";
            m_compileOutput += "Warning: No @fragment entry point found.";
        }

        if (hasVertexEntry && hasFragmentEntry)
        {
            m_compileOutput = "Basic syntax check passed. Vertex and fragment entry points found.\n";
            m_compileOutput += "Note: Full compilation will occur when material is loaded at runtime.";
            m_compileSuccess = true;
        }
    }
    else 
    {
        m_compileOutput = "SkSL validation not implemented (deprecated).";
        m_compileSuccess = false;
    }

    LogInfo("ShaderEditorPanel::CompileShader - {}", m_compileOutput);
}

void ShaderEditorPanel::AddUniform()
{
    
    for (const auto& uniform : m_materialData.uniforms)
    {
        if (uniform.name == m_newUniform.name)
        {
            LogWarn("ShaderEditorPanel::AddUniform - Uniform name '{}' already exists", m_newUniform.name);
            return;
        }
    }

    m_materialData.uniforms.push_back(m_newUniform);
    m_hasUnsavedChanges = true;

    LogInfo("ShaderEditorPanel::AddUniform - Added uniform '{}'", m_newUniform.name);
}

void ShaderEditorPanel::RemoveUniform(size_t index)
{
    if (index >= m_materialData.uniforms.size())
    {
        LogError("ShaderEditorPanel::RemoveUniform - Invalid index: {}", index);
        return;
    }

    std::string uniformName = m_materialData.uniforms[index].name;
    m_materialData.uniforms.erase(m_materialData.uniforms.begin() + index);
    m_hasUnsavedChanges = true;

    if (m_selectedUniformIndex == static_cast<int>(index))
    {
        m_selectedUniformIndex = -1;
    }
    else if (m_selectedUniformIndex > static_cast<int>(index))
    {
        m_selectedUniformIndex--;
    }

    LogInfo("ShaderEditorPanel::RemoveUniform - Removed uniform '{}'", uniformName);
}

void ShaderEditorPanel::Shutdown()
{
    
    m_currentMaterialHandle = AssetHandle();
    m_materialData = Data::MaterialDefinition();
    m_shaderCodeBuffer.clear();
    m_textEditor.SetText("");
    m_compileOutput.clear();
    m_isOpen = false;
    m_hasUnsavedChanges = false;
    m_codeChanged = false;
    m_compileSuccess = false;
    m_selectedUniformIndex = -1;
    m_addingUniform = false;
}

void ShaderEditorPanel::UpdateTextEditorLanguage()
{
    
    if (m_materialData.shaderType == Data::ShaderType::WGSL)
    {
        m_textEditor.SetLanguageDefinition(TextEditor::LanguageDefinition::WGSL());
        LogInfo("ShaderEditorPanel: Switched to WGSL syntax highlighting");
    }
    else 
    {
        m_textEditor.SetLanguageDefinition(TextEditor::LanguageDefinition::GLSL());
        LogInfo("ShaderEditorPanel: Switched to GLSL/SkSL syntax highlighting");
    }
}

void ShaderEditorPanel::RenderBindingsPanel()
{
    ImGui::Text("Shader Bindings");
    ImGui::Separator();

    
    if (m_bindingsDirty && m_materialData.shaderType == Data::ShaderType::WGSL)
    {
        ParseShaderBindings();
        m_bindingsDirty = false;
    }

    if (m_shaderBindings.empty())
    {
        ImGui::TextDisabled("无Binding信息");
        ImGui::TextWrapped("编译WGSL着色器后，binding信息将显示在此处。");
        return;
    }

    ImGui::Text("找到 %zu 个Binding:", m_shaderBindings.size());
    ImGui::Separator();

    
    for (size_t i = 0; i < m_shaderBindings.size(); i++)
    {
        const auto& binding = m_shaderBindings[i];
        
        ImGui::PushID(static_cast<int>(i));
        
        
        bool nodeOpen = ImGui::TreeNodeEx(binding.name.c_str(), ImGuiTreeNodeFlags_DefaultOpen);
        
        if (nodeOpen)
        {
            
            ImGui::Text("组: %zu", binding.groupIndex);
            ImGui::Text("位置: %zu", binding.location);
            
            
            const char* typeStr = "Unknown";
            ImVec4 typeColor = ImVec4(1, 1, 1, 1);
            
            switch (binding.type)
            {
                case Nut::BindingType::UniformBuffer:
                    typeStr = "Uniform Buffer";
                    typeColor = ImVec4(0.3f, 0.7f, 1.0f, 1.0f);
                    break;
                case Nut::BindingType::StorageBuffer:
                    typeStr = "Storage Buffer";
                    typeColor = ImVec4(1.0f, 0.7f, 0.3f, 1.0f);
                    break;
                case Nut::BindingType::Texture:
                    typeStr = "Texture";
                    typeColor = ImVec4(0.7f, 0.3f, 1.0f, 1.0f);
                    break;
                case Nut::BindingType::Sampler:
                    typeStr = "Sampler";
                    typeColor = ImVec4(0.3f, 1.0f, 0.7f, 1.0f);
                    break;
            }
            
            ImGui::Text("类型: ");
            ImGui::SameLine();
            ImGui::TextColored(typeColor, "%s", typeStr);
            
            // TODO: 对于UniformBuffer，可以进一步解析其字段并提供编辑界面
            // TODO: 对于Texture和Sampler，可以提供资源选择器
            
            ImGui::TreePop();
        }
        
        ImGui::PopID();
    }
}

void ShaderEditorPanel::ParseShaderBindings()
{
    m_shaderBindings.clear();
    
    if (m_materialData.shaderType != Data::ShaderType::WGSL)
    {
        return;  
    }
    
    const std::string& shaderCode = m_shaderCodeBuffer;
    
    
    std::regex re(R"(@group\((\d+)\)\s*@binding\((\d+)\)\s*var(?:<([^>]+)>)?\s+(\w+)\s*:\s*([\w_<>,\s]+))");
    std::smatch match;
    std::string::const_iterator searchStart(shaderCode.cbegin());
    
    while (std::regex_search(searchStart, shaderCode.cend(), match, re))
    {
        Nut::ShaderBindingInfo bindingInfo;
        bindingInfo.groupIndex = std::stoul(match[1]);
        bindingInfo.location = std::stoul(match[2]);
        
        std::string varModifierFull = match[3];
        std::string name = match[4];
        std::string typeStr = match[5];
        
        bindingInfo.name = name;
        
        
        if (typeStr.find("sampler") != std::string::npos)
        {
            bindingInfo.type = Nut::BindingType::Sampler;
        }
        else if (typeStr.find("texture") != std::string::npos)
        {
            bindingInfo.type = Nut::BindingType::Texture;
        }
        else if (varModifierFull.find("uniform") != std::string::npos)
        {
            bindingInfo.type = Nut::BindingType::UniformBuffer;
        }
        else if (varModifierFull.find("storage") != std::string::npos)
        {
            bindingInfo.type = Nut::BindingType::StorageBuffer;
        }
        
        m_shaderBindings.push_back(bindingInfo);
        searchStart = match.suffix().first;
    }
    
    LogInfo("ShaderEditorPanel::ParseShaderBindings - Found {} bindings", m_shaderBindings.size());
}

void ShaderEditorPanel::HandleFontZoom()
{
    
    ImGuiIO& io = ImGui::GetIO();
    
    
    if (ImGui::IsWindowHovered() && io.KeyCtrl && io.MouseWheel != 0.0f)
    {
        float delta = io.MouseWheel * 2.0f;  
        m_fontSize += delta;
        
        
        if (m_fontSize < m_fontSizeMin) m_fontSize = m_fontSizeMin;
        if (m_fontSize > m_fontSizeMax) m_fontSize = m_fontSizeMax;
        
        
        SaveFontSize();
        
        LogInfo("ShaderEditorPanel: Font size changed to {} (scale: {})", m_fontSize, m_fontSize / 16.0f);
    }
}

void ShaderEditorPanel::LoadFontSize()
{
    
    std::ifstream file("editor_config.txt");
    if (file.is_open())
    {
        std::string line;
        while (std::getline(file, line))
        {
            if (line.find("shader_editor_font_size=") == 0)
            {
                try
                {
                    m_fontSize = std::stof(line.substr(24));
                    if (m_fontSize < m_fontSizeMin) m_fontSize = m_fontSizeMin;
                    if (m_fontSize > m_fontSizeMax) m_fontSize = m_fontSizeMax;
                    LogInfo("ShaderEditorPanel: Loaded font size {}", m_fontSize);
                }
                catch (...)
                {
                    LogWarn("ShaderEditorPanel: Failed to parse font size, using default");
                }
                break;
            }
        }
        file.close();
    }
}

void ShaderEditorPanel::SaveFontSize()
{
    
    std::vector<std::string> lines;
    std::ifstream inFile("editor_config.txt");
    bool foundFontSize = false;
    
    if (inFile.is_open())
    {
        std::string line;
        while (std::getline(inFile, line))
        {
            if (line.find("shader_editor_font_size=") == 0)
            {
                lines.push_back("shader_editor_font_size=" + std::to_string(m_fontSize));
                foundFontSize = true;
            }
            else
            {
                lines.push_back(line);
            }
        }
        inFile.close();
    }
    
    
    if (!foundFontSize)
    {
        lines.push_back("shader_editor_font_size=" + std::to_string(m_fontSize));
    }
    
    
    std::ofstream outFile("editor_config.txt");
    if (outFile.is_open())
    {
        for (const auto& line : lines)
        {
            outFile << line << "\n";
        }
        outFile.close();
    }
}

void ShaderEditorPanel::RenderSettingsPanel()
{
    ImGui::SetNextWindowSize(ImVec2(600, 700), ImGuiCond_FirstUseEver);
    
    if (ImGui::Begin("着色器编辑器设置", &m_showSettingsPanel))
    {
        if (ImGui::BeginTabBar("##settings_tabs"))
        {
            
            if (ImGui::BeginTabItem("颜色"))
            {
                ImGui::Checkbox("使用自定义颜色", &m_useCustomColors);
                
                if (m_useCustomColors)
                {
                    ImGui::Separator();
                    ImGui::Text("编辑器配色方案：");
                    
                    const char* colorNames[] = {
                        "默认", "关键字", "数字", "字符串", "字符字面量",
                        "标点符号", "预处理", "标识符", "已知标识符", "预处理标识符",
                        "单行注释", "多行注释", "背景", "光标", "选择",
                        "错误标记", "断点", "行号", "当前行填充", "当前行填充(非活动)", "当前行边缘"
                    };
                    
                    for (int i = 0; i < (int)TextEditor::PaletteIndex::Max; ++i)
                    {
                        ImGui::PushID(i);
                        
                        ImVec4 color = ImGui::ColorConvertU32ToFloat4(m_customPalette[i]);
                        if (ImGui::ColorEdit4(colorNames[i], &color.x, 
                            ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_AlphaPreview))
                        {
                            m_customPalette[i] = ImGui::ColorConvertFloat4ToU32(color);
                        }
                        
                        ImGui::PopID();
                    }
                    
                    ImGui::Separator();
                    
                    if (ImGui::Button("应用"))
                    {
                        ApplyColorSettings();
                        SaveColorSettings();
                    }
                    
                    ImGui::SameLine();
                    
                    if (ImGui::Button("重置为暗色"))
                    {
                        m_customPalette = TextEditor::GetDarkPalette();
                        ApplyColorSettings();
                    }
                    
                    ImGui::SameLine();
                    
                    if (ImGui::Button("重置为亮色"))
                    {
                        m_customPalette = TextEditor::GetLightPalette();
                        ApplyColorSettings();
                    }
                }
                else
                {
                    ImGui::TextWrapped("当前使用默认暗色主题。勾选上方复选框以自定义颜色。");
                }
                
                ImGui::EndTabItem();
            }
            
            
            if (ImGui::BeginTabItem("关键字"))
            {
                ImGui::Text("自定义关键字列表：");
                ImGui::Separator();
                
                
                ImGui::BeginChild("##keywords_list", ImVec2(0, -60), true);
                {
                    for (size_t i = 0; i < m_customKeywords.size(); ++i)
                    {
                        ImGui::PushID(static_cast<int>(i));
                        
                        ImGui::Text("%s", m_customKeywords[i].c_str());
                        
                        ImGui::SameLine();
                        
                        if (ImGui::SmallButton("删除"))
                        {
                            m_customKeywords.erase(m_customKeywords.begin() + i);
                            ApplyCustomKeywords();
                            SaveCustomKeywords();
                            --i;
                        }
                        
                        ImGui::PopID();
                    }
                }
                ImGui::EndChild();
                
                ImGui::Separator();
                
                
                ImGui::InputText("新关键字", &m_newKeywordBuffer);
                
                ImGui::SameLine();
                
                if (ImGui::Button("添加"))
                {
                    if (!m_newKeywordBuffer.empty())
                    {
                        
                        bool exists = false;
                        for (const auto& kw : m_customKeywords)
                        {
                            if (kw == m_newKeywordBuffer)
                            {
                                exists = true;
                                break;
                            }
                        }
                        
                        if (!exists)
                        {
                            m_customKeywords.push_back(m_newKeywordBuffer);
                            ApplyCustomKeywords();
                            SaveCustomKeywords();
                            m_newKeywordBuffer.clear();
                        }
                    }
                }
                
                ImGui::SameLine();
                
                if (ImGui::Button("清空全部"))
                {
                    m_customKeywords.clear();
                    ApplyCustomKeywords();
                    SaveCustomKeywords();
                }
                
                ImGui::EndTabItem();
            }
            
            ImGui::EndTabBar();
        }
    }
    ImGui::End();
}

void ShaderEditorPanel::LoadColorSettings()
{
    std::ifstream file("editor_config.txt");
    if (file.is_open())
    {
        std::string line;
        while (std::getline(file, line))
        {
            if (line.find("use_custom_colors=") == 0)
            {
                m_useCustomColors = (line.substr(18) == "true");
            }
            else if (line.find("palette_") == 0)
            {
                
                size_t eq = line.find('=');
                if (eq != std::string::npos)
                {
                    std::string indexStr = line.substr(8, eq - 8);
                    std::string valueStr = line.substr(eq + 1);
                    
                    try
                    {
                        int index = std::stoi(indexStr);
                        uint32_t value = std::stoul(valueStr, nullptr, 16);
                        
                        if (index >= 0 && index < (int)TextEditor::PaletteIndex::Max)
                        {
                            m_customPalette[index] = value;
                        }
                    }
                    catch (...)
                    {
                        
                    }
                }
            }
        }
        file.close();
        
        LogInfo("ShaderEditorPanel: Loaded color settings");
    }
}

void ShaderEditorPanel::SaveColorSettings()
{
    
    std::vector<std::string> lines;
    std::ifstream inFile("editor_config.txt");
    std::set<std::string> processedKeys;
    
    if (inFile.is_open())
    {
        std::string line;
        while (std::getline(inFile, line))
        {
            
            if (line.find("use_custom_colors=") == 0 || line.find("palette_") == 0)
            {
                continue;
            }
            lines.push_back(line);
        }
        inFile.close();
    }
    
    
    lines.push_back("use_custom_colors=" + std::string(m_useCustomColors ? "true" : "false"));
    
    for (int i = 0; i < (int)TextEditor::PaletteIndex::Max; ++i)
    {
        char buffer[64];
        snprintf(buffer, sizeof(buffer), "palette_%d=0x%08x", i, m_customPalette[i]);
        lines.push_back(buffer);
    }
    
    
    std::ofstream outFile("editor_config.txt");
    if (outFile.is_open())
    {
        for (const auto& line : lines)
        {
            outFile << line << "\n";
        }
        outFile.close();
        
        LogInfo("ShaderEditorPanel: Saved color settings");
    }
}

void ShaderEditorPanel::LoadCustomKeywords()
{
    std::ifstream file("editor_config.txt");
    if (file.is_open())
    {
        std::string line;
        while (std::getline(file, line))
        {
            if (line.find("custom_keyword=") == 0)
            {
                std::string keyword = line.substr(15);
                if (!keyword.empty())
                {
                    m_customKeywords.push_back(keyword);
                }
            }
        }
        file.close();
        
        LogInfo("ShaderEditorPanel: Loaded {} custom keywords", m_customKeywords.size());
    }
}

void ShaderEditorPanel::SaveCustomKeywords()
{
    
    std::vector<std::string> lines;
    std::ifstream inFile("editor_config.txt");
    
    if (inFile.is_open())
    {
        std::string line;
        while (std::getline(inFile, line))
        {
            
            if (line.find("custom_keyword=") == 0)
            {
                continue;
            }
            lines.push_back(line);
        }
        inFile.close();
    }
    
    
    for (const auto& keyword : m_customKeywords)
    {
        lines.push_back("custom_keyword=" + keyword);
    }
    
    
    std::ofstream outFile("editor_config.txt");
    if (outFile.is_open())
    {
        for (const auto& line : lines)
        {
            outFile << line << "\n";
        }
        outFile.close();
        
        LogInfo("ShaderEditorPanel: Saved {} custom keywords", m_customKeywords.size());
    }
}

void ShaderEditorPanel::ApplyColorSettings()
{
    if (m_useCustomColors)
    {
        m_textEditor.SetPalette(m_customPalette);
        LogInfo("ShaderEditorPanel: Applied custom color palette");
    }
    else
    {
        m_textEditor.SetPalette(TextEditor::GetDarkPalette());
        LogInfo("ShaderEditorPanel: Applied default dark palette");
    }
}

void ShaderEditorPanel::ApplyCustomKeywords()
{
    if (m_customKeywords.empty())
    {
        return;
    }
    
    
    auto langDef = m_textEditor.GetLanguageDefinition();
    
    
    for (const auto& keyword : m_customKeywords)
    {
        langDef.mKeywords.insert(keyword);
    }
    
    
    m_textEditor.SetLanguageDefinition(langDef);
    
    LogInfo("ShaderEditorPanel: Applied {} custom keywords", m_customKeywords.size());
}
