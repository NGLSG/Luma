#include "ShaderEditorPanel.h"
#include "AssetManager.h"
#include "Logger.h"
#include "EditorContext.h"
#include "Renderer/Nut/NutContext.h"
#include "imgui.h"
#include "imgui_stdlib.h"
#include <fstream>
#include <regex>
#include <set>
#include <algorithm>
#include <cctype>
#include <filesystem>

#include "GraphicsBackend.h"

ShaderEditorPanel::ShaderEditorPanel()
{
    m_textEditor.SetLanguageDefinition(TextEditor::LanguageDefinition::WGSL());
    m_textEditor.SetShowWhitespaces(false);
    m_textEditor.SetImGuiChildIgnored(true);
    m_textEditor.SetTabSize(4);


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
    if (!m_context)
    {
        LogError("ShaderEditorPanel::Initialize - Invalid EditorContext provided.");
    }
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


    RenderAutoCompletePopup();
}


void ShaderEditorPanel::HandleAutoComplete()
{
    if (!m_textEditor.IsHandleKeyboardInputsEnabled())
    {
        m_textEditor.SetHandleKeyboardInputs(true);
    }

    ImGuiIO& io = ImGui::GetIO();
    bool isCtrl = io.KeyCtrl;
    bool isAlt = io.KeyAlt;


    if (m_isAutoCompleteOpen)
    {
        std::string prefix = GetWordUnderCursor();


        m_popupPos = m_textEditor.GetCursorScreenPosition();
        m_popupPos.y += 20;

        if (ImGui::IsKeyPressed(ImGuiKey_DownArrow))
        {
            m_autoCompleteSelectedIndex++;
            if (m_autoCompleteSelectedIndex >= static_cast<int>(m_autoCompleteCandidates.size()))
                m_autoCompleteSelectedIndex = 0;
            return;
        }
        else if (ImGui::IsKeyPressed(ImGuiKey_UpArrow))
        {
            m_autoCompleteSelectedIndex--;
            if (m_autoCompleteSelectedIndex < 0)
                m_autoCompleteSelectedIndex = static_cast<int>(m_autoCompleteCandidates.size()) - 1;
            return;
        }
        else if (ImGui::IsKeyPressed(ImGuiKey_Tab) || ImGui::IsKeyPressed(ImGuiKey_Enter))
        {
            if (!m_autoCompleteCandidates.empty() &&
                m_autoCompleteSelectedIndex >= 0 &&
                m_autoCompleteSelectedIndex < static_cast<int>(m_autoCompleteCandidates.size()))
            {
                std::string toInsert = m_autoCompleteCandidates[m_autoCompleteSelectedIndex];
                if (toInsert.length() > prefix.length())
                {
                    m_textEditor.InsertText(toInsert.substr(prefix.length()));
                }
            }

            m_isAutoCompleteOpen = false;
            m_autoCompleteCandidates.clear();
            m_currentWordPrefix.clear();
            m_textEditor.SetHandleKeyboardInputs(false);
            return;
        }
        else if (ImGui::IsKeyPressed(ImGuiKey_Escape))
        {
            m_isAutoCompleteOpen = false;
            m_currentWordPrefix.clear();
            return;
        }


        if (!prefix.empty())
        {
            if (prefix != m_currentWordPrefix)
            {
                m_currentWordPrefix = prefix;
                m_autoCompleteCandidates.clear();

                const auto& langDef = m_textEditor.GetLanguageDefinition();


                std::string lowerPrefix = prefix;
                std::transform(lowerPrefix.begin(), lowerPrefix.end(), lowerPrefix.begin(), ::tolower);

                auto AddCandidate = [&](const std::string& candidate)
                {
                    std::string lowerCandidate = candidate;
                    std::transform(lowerCandidate.begin(), lowerCandidate.end(), lowerCandidate.begin(), ::tolower);

                    if (lowerCandidate.find(lowerPrefix) == 0 && candidate != prefix)
                    {
                        m_autoCompleteCandidates.push_back(candidate);
                    }
                };

                for (const auto& kw : langDef.mKeywords) AddCandidate(kw);
                for (const auto& ident : langDef.mIdentifiers) AddCandidate(ident.first);
                for (const auto& kw : m_customKeywords) AddCandidate(kw);

                std::sort(m_autoCompleteCandidates.begin(), m_autoCompleteCandidates.end());

                if (m_autoCompleteCandidates.empty())
                {
                    m_isAutoCompleteOpen = false;
                    m_currentWordPrefix.clear();
                }
                else
                {
                    m_autoCompleteSelectedIndex = 0;
                }
            }
        }
        else
        {
            m_isAutoCompleteOpen = false;
            m_currentWordPrefix.clear();
        }

        return;
    }


    std::string prefix = GetWordUnderCursor();


    if (prefix != "")
        LogInfo("AutoComplete Trigger: prefix='{}', cached='{}', empty={}",
            prefix, m_currentWordPrefix, prefix.empty());

    if (prefix.empty())
    {
        if (m_currentWordPrefix != "")
        {
            m_currentWordPrefix.clear();
        }
        return;
    }

    bool prefixChanged = (prefix != m_currentWordPrefix);
    bool shouldUpdate = !isCtrl && !isAlt && prefixChanged;

    LogInfo("AutoComplete Check: prefixChanged={}, shouldUpdate={}, isCtrl={}, isAlt={}",
            prefixChanged, shouldUpdate, isCtrl, isAlt);

    if (shouldUpdate)
    {
        if (prefix.length() >= 1)
        {
            m_currentWordPrefix = prefix;
            m_autoCompleteCandidates.clear();

            const auto& langDef = m_textEditor.GetLanguageDefinition();


            std::string lowerPrefix = prefix;
            std::transform(lowerPrefix.begin(), lowerPrefix.end(), lowerPrefix.begin(), ::tolower);

            auto AddCandidate = [&](const std::string& candidate)
            {
                std::string lowerCandidate = candidate;
                std::transform(lowerCandidate.begin(), lowerCandidate.end(), lowerCandidate.begin(), ::tolower);

                if (lowerCandidate.find(lowerPrefix) == 0 && candidate != prefix)
                {
                    m_autoCompleteCandidates.push_back(candidate);
                }
            };

            for (const auto& kw : langDef.mKeywords) AddCandidate(kw);
            for (const auto& ident : langDef.mIdentifiers) AddCandidate(ident.first);
            for (const auto& kw : m_customKeywords) AddCandidate(kw);

            std::sort(m_autoCompleteCandidates.begin(), m_autoCompleteCandidates.end());

            LogInfo("AutoComplete Result: found {} candidates for prefix '{}'",
                    m_autoCompleteCandidates.size(), prefix);

            if (!m_autoCompleteCandidates.empty())
            {
                m_isAutoCompleteOpen = true;
                m_autoCompleteSelectedIndex = 0;


                m_popupPos = m_textEditor.GetCursorScreenPosition();
                m_popupPos.y += 20;

                LogInfo("AutoComplete OPENED at pos ({}, {})", m_popupPos.x, m_popupPos.y);
            }
            else
            {
                m_isAutoCompleteOpen = false;
                m_currentWordPrefix.clear();
                LogInfo("AutoComplete CLOSED: no candidates");
            }
        }
        else
        {
            m_isAutoCompleteOpen = false;
            m_currentWordPrefix.clear();
        }
    }
}

void ShaderEditorPanel::RenderAutoCompletePopup()
{
    if (!m_isAutoCompleteOpen || m_autoCompleteCandidates.empty()) return;


    ImGui::SetNextWindowPos(m_popupPos, ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(220, 0), ImGuiCond_Always);


    ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_AlwaysAutoResize |
        ImGuiWindowFlags_NoFocusOnAppearing |
        ImGuiWindowFlags_NoDocking |
        ImGuiWindowFlags_NoNav |
        ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoScrollWithMouse |
        ImGuiWindowFlags_Tooltip;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 4.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(6, 6));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.15f, 0.15f, 0.15f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));


    bool windowVisible = ImGui::Begin("ShaderAutoComplete", nullptr, flags);


    if (windowVisible)
    {
        ImGui::TextColored(ImVec4(0.6f, 0.8f, 1.0f, 1.0f), "Auto Complete (%d)", (int)m_autoCompleteCandidates.size());
        ImGui::Separator();


        for (int i = 0; i < static_cast<int>(m_autoCompleteCandidates.size()); ++i)
        {
            bool isSelected = (i == m_autoCompleteSelectedIndex);

            if (isSelected)
            {
                ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.3f, 0.5f, 0.8f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.4f, 0.6f, 0.9f, 1.0f));
            }

            if (ImGui::Selectable(m_autoCompleteCandidates[i].c_str(), isSelected))
            {
                std::string prefix = GetWordUnderCursor();
                std::string toInsert = m_autoCompleteCandidates[i];
                if (toInsert.length() > prefix.length())
                {
                    m_textEditor.InsertText(toInsert.substr(prefix.length()));
                }

                m_isAutoCompleteOpen = false;
                m_currentWordPrefix.clear();
                m_autoCompleteCandidates.clear();
            }

            if (isSelected)
            {
                ImGui::PopStyleColor(2);
            }
        }
    }
    ImGui::End();

    ImGui::PopStyleColor(2);
    ImGui::PopStyleVar(3);
}

std::string ShaderEditorPanel::GetWordUnderCursor() const
{
    auto pos = m_textEditor.GetCursorPosition();
    std::string line = m_textEditor.GetCurrentLineText();

    if (pos.mColumn == 0 || pos.mColumn > static_cast<int>(line.length())) return "";


    char leftChar = line[pos.mColumn - 1];


    if (!isalnum(leftChar) && leftChar != '_')
    {
        return "";
    }


    int start = pos.mColumn - 1;
    while (start >= 0)
    {
        char c = line[start];
        if (!isalnum(c) && c != '_') break;
        start--;
    }

    return line.substr(start + 1, pos.mColumn - (start + 1));
}


void ShaderEditorPanel::RenderCodeEditor()
{
    ImGui::Text("代码视图:");
    ImGui::SameLine();
    if (m_currentShaderHandle.Valid())
    {
        if (m_shaderData.language == Data::ShaderLanguage::WGSL)
            ImGui::TextColored(ImVec4(0.3f, 0.7f, 1.0f, 1.0f), "(WGSL)");
        else
            ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "(SkSL - [Obsolete])");
    }
    ImGui::Separator();

    auto cpos = m_textEditor.GetCursorPosition();
    ImGui::Text("Ln: %d | Col: %d | Lines: %d", cpos.mLine + 1, cpos.mColumn + 1, m_textEditor.GetTotalLines());


    ImGui::BeginChild("##code_editor_content", ImVec2(0, 0), true,
                      ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_AlwaysHorizontalScrollbar |
                      ImGuiWindowFlags_NoMove);
    {
        HandleFontZoom();
        float scale = m_fontSize / 16.0f;
        ImGui::SetWindowFontScale(scale);


        HandleAutoComplete();


        m_textEditor.Render("##shader_code_editor");


        if (!m_textEditor.IsHandleKeyboardInputsEnabled())
        {
            m_textEditor.SetHandleKeyboardInputs(true);
        }
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


void ShaderEditorPanel::CompileShader()
{
    m_compileOutput.clear();
    m_compileSuccess = false;
    m_shaderBindings.clear();

    if (m_shaderCodeBuffer.empty())
    {
        m_compileOutput = "错误: 代码为空。";
        return;
    }

    bool isWGSL = false;
    if (m_currentShaderHandle.Valid())
    {
        isWGSL = (m_shaderData.language == Data::ShaderLanguage::WGSL);
    }

    if (!isWGSL)
    {
        m_compileOutput = "警告: 仅支持 WGSL 的实时编译验证。SkSL 已弃用。";
        return;
    }

    if (!m_context)
    {
        LogError("ShaderEditorPanel::CompileShader - Context is null");
        m_compileOutput = "系统错误: 无法访问引擎上下文。";
        return;
    }

    try
    {
        auto nutCtx = m_context->engineContext->graphicsBackend->GetNutContext();
        if (!nutCtx)
        {
            throw std::runtime_error("NutContext 获取失败，图形后端未就绪。");
        }

        LogInfo("ShaderEditorPanel: Compiling WGSL shader...");
        Nut::ShaderModule& module = Nut::ShaderManager::GetFromString(m_shaderCodeBuffer, nutCtx);

        if (module)
        {
            m_compileSuccess = true;
            m_compileOutput = "编译成功 (Validation Passed)";

            module.ForeachBinding([this](const Nut::ShaderBindingInfo& info)
            {
                m_shaderBindings.push_back(info);
            });

            std::sort(m_shaderBindings.begin(), m_shaderBindings.end(),
                      [](const Nut::ShaderBindingInfo& a, const Nut::ShaderBindingInfo& b)
                      {
                          if (a.groupIndex != b.groupIndex) return a.groupIndex < b.groupIndex;
                          return a.location < b.location;
                      });

            LogInfo("ShaderEditorPanel: Compilation successful. Found {} bindings.", m_shaderBindings.size());
        }
        else
        {
            m_compileSuccess = false;
            m_compileOutput = "编译失败: ShaderModule 创建失败 (请查看控制台日志)。";
        }
    }
    catch (const std::exception& e)
    {
        m_compileSuccess = false;
        m_compileOutput = std::string("编译异常:\n") + e.what();
        LogError("ShaderEditorPanel::CompileShader - Exception: {}", e.what());
    }
}

void ShaderEditorPanel::RenderBindingsPanel()
{
    ImGui::Text("资源绑定 (Reflection)");
    ImGui::Separator();

    if (!m_compileSuccess && m_shaderBindings.empty())
    {
        ImGui::TextDisabled("请先编译着色器以查看绑定信息。");
        return;
    }

    if (m_shaderBindings.empty())
    {
        ImGui::TextDisabled("无绑定资源。");
        return;
    }

    int currentGroup = -1;

    for (const auto& binding : m_shaderBindings)
    {
        if (static_cast<int>(binding.groupIndex) != currentGroup)
        {
            currentGroup = static_cast<int>(binding.groupIndex);
            ImGui::Separator();
            ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.8f, 1.0f), "@group(%d)", currentGroup);
        }

        ImGui::PushID((std::to_string(binding.groupIndex * 1000 + binding.location) + binding.name).c_str());

        std::string label = std::format("@binding({}) {}", binding.location, binding.name);

        bool nodeOpen = ImGui::TreeNodeEx(label.c_str(), ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Leaf);

        if (nodeOpen)
        {
            ImGui::Indent();

            const char* typeStr = "Unknown";
            ImVec4 typeColor = ImVec4(1, 1, 1, 1);

            switch (binding.type)
            {
            case Nut::BindingType::UniformBuffer:
                typeStr = "Uniform Buffer";
                typeColor = ImVec4(0.4f, 0.8f, 1.0f, 1.0f);
                break;
            case Nut::BindingType::StorageBuffer:
                typeStr = "Storage Buffer";
                typeColor = ImVec4(1.0f, 0.6f, 0.2f, 1.0f);
                break;
            case Nut::BindingType::Texture:
                typeStr = "Texture";
                typeColor = ImVec4(0.8f, 0.4f, 1.0f, 1.0f);
                break;
            case Nut::BindingType::Sampler:
                typeStr = "Sampler";
                typeColor = ImVec4(0.4f, 1.0f, 0.6f, 1.0f);
                break;
            }

            ImGui::Text("类型:");
            ImGui::SameLine();
            ImGui::TextColored(typeColor, "%s", typeStr);

            ImGui::Unindent();
            ImGui::TreePop();
        }
        ImGui::PopID();
    }
}


void ShaderEditorPanel::OpenShader(const AssetHandle& shaderHandle)
{
    if (!shaderHandle.Valid())
    {
        LogError("ShaderEditorPanel::OpenShader - Invalid shader handle");
        return;
    }

    auto metadata = AssetManager::GetInstance().GetMetadata(shaderHandle.assetGuid);
    if (!metadata || metadata->type != AssetType::Shader)
    {
        LogError("ShaderEditorPanel::OpenShader - Failed to load shader metadata");
        return;
    }

    m_currentShaderHandle = shaderHandle;
    m_shaderData = metadata->importerSettings.as<Data::ShaderData>();
    m_shaderCodeBuffer = m_shaderData.source;

    m_textEditor.SetText(m_shaderCodeBuffer);

    UpdateTextEditorLanguage();

    m_isOpen = true;
    m_isVisible = true;
    m_hasUnsavedChanges = false;
    m_codeChanged = false;

    CompileShader();

    LogInfo("ShaderEditorPanel::OpenShader - Opened: {}", metadata->assetPath.string());
}

void ShaderEditorPanel::SaveShader()
{
    if (!m_currentShaderHandle.Valid())
    {
        LogError("ShaderEditorPanel::SaveShader - No shader open");
        return;
    }

    auto metadata = AssetManager::GetInstance().GetMetadata(m_currentShaderHandle.assetGuid);
    if (!metadata) return;

    m_shaderCodeBuffer = m_textEditor.GetText();
    m_shaderData.source = m_shaderCodeBuffer;

    YAML::Node node;
    node = m_shaderData;

    std::ofstream file(metadata->assetPath);
    if (!file.is_open())
    {
        LogError("ShaderEditorPanel::SaveShader - Failed to write file: {}", metadata->assetPath.string());
        return;
    }

    file << node;
    file.close();

    AssetMetadata updatedMeta = *metadata;
    updatedMeta.importerSettings = node;
    AssetManager::GetInstance().ReImport(updatedMeta);

    m_hasUnsavedChanges = false;
    m_codeChanged = false;

    LogInfo("ShaderEditorPanel::SaveShader - Saved: {}", metadata->assetPath.string());
}

void ShaderEditorPanel::OpenMaterial(const AssetHandle& materialHandle)
{
    LogWarn("ShaderEditorPanel::OpenMaterial is deprecated. Please edit Shader assets directly.");
}

void ShaderEditorPanel::SaveMaterial()
{
    LogWarn("ShaderEditorPanel::SaveMaterial is deprecated.");
}


void ShaderEditorPanel::UpdateTextEditorLanguage()
{
    if (m_currentShaderHandle.Valid() && m_shaderData.language == Data::ShaderLanguage::WGSL)
    {
        m_textEditor.SetLanguageDefinition(TextEditor::LanguageDefinition::WGSL());
    }
    else
    {
        m_textEditor.SetLanguageDefinition(TextEditor::LanguageDefinition::GLSL());
    }
}

void ShaderEditorPanel::RenderToolbar()
{
    if (ImGui::BeginMenuBar())
    {
        if (ImGui::BeginMenu("文件"))
        {
            if (ImGui::MenuItem("保存", "Ctrl+S", nullptr, m_hasUnsavedChanges)) SaveShader();
            if (ImGui::MenuItem("关闭")) m_isOpen = false;
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("编辑"))
        {
            if (ImGui::MenuItem("撤销", "Ctrl+Z", nullptr, m_textEditor.CanUndo())) m_textEditor.Undo();
            if (ImGui::MenuItem("重做", "Ctrl+Y", nullptr, m_textEditor.CanRedo())) m_textEditor.Redo();
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("构建"))
        {
            if (ImGui::MenuItem("编译 Shader", "F5")) CompileShader();
            ImGui::EndMenu();
        }
        ImGui::EndMenuBar();
    }

    if (ImGui::Button("保存")) SaveShader();
    ImGui::SameLine();
    if (ImGui::Button("编译 (F5)")) CompileShader();
    ImGui::SameLine();
    if (ImGui::Button("设置")) m_showSettingsPanel = !m_showSettingsPanel;

    ImGui::SameLine();
    if (m_hasUnsavedChanges)
        ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.0f, 1.0f), "  * 未保存");
    else
        ImGui::TextColored(ImVec4(0.4f, 0.8f, 0.4f, 1.0f), "  已保存");
}

void ShaderEditorPanel::RenderCompileOutput()
{
    ImGui::Text("输出日志:");
    ImGui::Separator();

    if (!m_compileOutput.empty())
    {
        if (m_compileSuccess)
            ImGui::TextColored(ImVec4(0.4f, 0.8f, 0.4f, 1.0f), "[Success] %s", m_compileOutput.c_str());
        else
            ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "[Error] %s", m_compileOutput.c_str());
    }
}

void ShaderEditorPanel::Shutdown()
{
    m_isOpen = false;
    m_compileOutput.clear();
    m_shaderBindings.clear();
    m_context = nullptr;
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
    bool found = false;

    if (inFile.is_open())
    {
        std::string line;
        while (std::getline(inFile, line))
        {
            if (line.find("shader_editor_font_size=") == 0)
            {
                lines.push_back("shader_editor_font_size=" + std::to_string(m_fontSize));
                found = true;
            }
            else lines.push_back(line);
        }
        inFile.close();
    }
    if (!found) lines.push_back("shader_editor_font_size=" + std::to_string(m_fontSize));

    std::ofstream outFile("editor_config.txt");
    if (outFile.is_open())
    {
        for (const auto& line : lines) outFile << line << "\n";
        outFile.close();
    }
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
                    std::string idxStr = line.substr(8, eq - 8);
                    std::string valStr = line.substr(eq + 1);
                    try
                    {
                        int idx = std::stoi(idxStr);
                        uint32_t val = std::stoul(valStr, nullptr, 16);
                        if (idx >= 0 && idx < (int)TextEditor::PaletteIndex::Max) m_customPalette[idx] = val;
                    }
                    catch (...)
                    {
                    }
                }
            }
        }
        file.close();
    }
}

void ShaderEditorPanel::SaveColorSettings()
{
    std::vector<std::string> lines;
    std::ifstream inFile("editor_config.txt");
    if (inFile.is_open())
    {
        std::string line;
        while (std::getline(inFile, line))
        {
            if (line.find("use_custom_colors=") == 0 || line.find("palette_") == 0) continue;
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
        for (const auto& line : lines) outFile << line << "\n";
        outFile.close();
    }
}

void ShaderEditorPanel::ApplyColorSettings()
{
    if (m_useCustomColors) m_textEditor.SetPalette(m_customPalette);
    else m_textEditor.SetPalette(TextEditor::GetDarkPalette());
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
                std::string kw = line.substr(15);
                if (!kw.empty()) m_customKeywords.push_back(kw);
            }
        }
        file.close();
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
            if (line.find("custom_keyword=") == 0) continue;
            lines.push_back(line);
        }
        inFile.close();
    }

    for (const auto& kw : m_customKeywords) lines.push_back("custom_keyword=" + kw);

    std::ofstream outFile("editor_config.txt");
    if (outFile.is_open())
    {
        for (const auto& line : lines) outFile << line << "\n";
        outFile.close();
    }
}

void ShaderEditorPanel::ApplyCustomKeywords()
{
    if (m_customKeywords.empty()) return;
    auto langDef = m_textEditor.GetLanguageDefinition();
    for (const auto& kw : m_customKeywords) langDef.mKeywords.insert(kw);
    m_textEditor.SetLanguageDefinition(langDef);
}


void ShaderEditorPanel::RenderUniformEditor()
{
}

void ShaderEditorPanel::AddUniform()
{
}

void ShaderEditorPanel::RemoveUniform(size_t index)
{
}
