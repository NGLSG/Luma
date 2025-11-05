#include "TextureSlicerPanel.h"
#include <imgui.h>
#include <imgui_internal.h>
#include "AssetManager.h"
#include "Path.h"
#include "Logger.h"
#include "Profiler.h"
#include "TextureImporterSettings.h"
#include "GraphicsBackend.h"
#include "ImGuiRenderer.h"
#include <fstream>
#include <algorithm>
#include <webgpu/webgpu_cpp.h>

#include "Utils/stb_image.h"
#include "Utils/stb_image_write.h"


constexpr size_t SLICE_PREVIEW_PERF_THRESHOLD = 500;

void TextureSlicerPanel::Initialize(EditorContext* context)
{
    m_context = context;
    m_isVisible = false;
}

void TextureSlicerPanel::Update(float deltaTime)
{
}

void TextureSlicerPanel::Draw()
{
    if (!m_isOpen)
    {
        if (m_gpuTexture)
        {
            LogInfo("TextureSlicerPanel: 释放已关闭面板的资源...");
            if (m_textureData)
            {
                stbi_image_free(m_textureData);
                m_textureData = nullptr;
            }

            m_gpuTexture.Destroy();
            m_gpuTexture = nullptr;

            m_slices.clear();
            m_currentTextureGuid = Guid::Invalid();
            m_textureID = -1;
        }

        m_isVisible = false;
        return;
    }

    if (!m_isVisible)
        return;

    PROFILE_FUNCTION();

    ImGui::SetNextWindowSize(ImVec2(1200, 800), ImGuiCond_FirstUseEver);

    if (ImGui::Begin("纹理切片编辑器", &m_isOpen, ImGuiWindowFlags_NoCollapse))
    {
        drawToolbar();

        ImGui::Separator();

        ImGui::BeginChild("MainContent", ImVec2(0, -40), false);
        {
            ImGui::BeginChild("TexturePreview", ImVec2(ImGui::GetContentRegionAvail().x * 0.7f, 0), true);
            {
                drawTexturePreview();
            }
            ImGui::EndChild();

            ImGui::SameLine();

            ImGui::BeginChild("RightPanel", ImVec2(0, 0), true);
            {
                drawSettingsPanel();
                ImGui::Separator();
                drawSliceList();
            }
            ImGui::EndChild();
        }
        ImGui::EndChild();

        ImGui::Separator();
        if (ImGui::Button("应用切片", ImVec2(120, 30)))
        {
            applySlices();
        }
        ImGui::SameLine();
        if (ImGui::Button("关闭", ImVec2(120, 30)))
        {
            m_isOpen = false;
            m_isVisible = false;
        }
    }
    ImGui::End();
}

void TextureSlicerPanel::Shutdown()
{
    if (m_textureData)
    {
        stbi_image_free(m_textureData);
        m_textureData = nullptr;
    }

    if (m_gpuTexture)
    {
        m_gpuTexture.Destroy();
        m_gpuTexture = nullptr;
    }

    m_slices.clear();
    m_textureID = -1;
}

void TextureSlicerPanel::OpenTexture(const Guid& textureGuid)
{
    if (m_gpuTexture)
    {
        m_gpuTexture.Destroy();
        m_gpuTexture = nullptr;
    }
    if (m_textureData)
    {
        stbi_image_free(m_textureData);
        m_textureData = nullptr;
    }

    m_textureID = -1;
    m_currentTextureGuid = textureGuid;
    m_isOpen = true;
    m_isVisible = true;
    m_slices.clear();


    m_showSlicePreviews = true;
    m_interactionMode = InteractionMode::Create;
    m_selectedSliceIndex = -1;
    m_isMovingSlice = false;
    m_isResizingSlice = false;

    loadTexture();

    if (m_textureData)
    {
        m_gridRows = 1;
        m_gridColumns = 1;
        generateGridSlices();
    }
}

void TextureSlicerPanel::Close()
{
    m_isOpen = false;
    m_isVisible = false;
}

void TextureSlicerPanel::drawToolbar()
{
    PROFILE_FUNCTION();

    
    ImGui::Text("切片模式:");
    ImGui::SameLine();

    if (ImGui::RadioButton("网格切片", m_sliceMode == SliceMode::Grid))
    {
        m_sliceMode = SliceMode::Grid;
        generateGridSlices();
    }
    ImGui::SameLine();

    if (ImGui::RadioButton("手动切片", m_sliceMode == SliceMode::Manual))
    {
        m_sliceMode = SliceMode::Manual;
        m_showSlicePreviews = true;
    }

    
    ImGui::SameLine();
    ImGui::Dummy(ImVec2(10, 0));
    ImGui::SameLine();

    
    ImGui::Text("交互模式:");
    ImGui::SameLine();
    if (ImGui::RadioButton("创建", m_interactionMode == InteractionMode::Create))
    {
        m_interactionMode = InteractionMode::Create;
        
        m_isMovingSlice = false;
        m_isResizingSlice = false;
    }
    ImGui::SameLine();
    if (ImGui::RadioButton("编辑", m_interactionMode == InteractionMode::Edit))
    {
        m_interactionMode = InteractionMode::Edit;
        
        m_isDragging = false;
    }

    
    ImGui::SameLine();
    ImGui::Dummy(ImVec2(10, 0));
    ImGui::SameLine();

    
    if (ImGui::Button("清除所有切片", ImVec2(140, 0)))
    {
        m_slices.clear();
        m_selectedSliceIndex = -1;
        m_isMovingSlice = false;
        m_isResizingSlice = false;
        LogInfo("已清除所有切片");
    }

    
    ImGui::SameLine();
    if (ImGui::Checkbox("显示切片预览", &m_showSlicePreviews))
    {
        if (m_showSlicePreviews && m_slices.size() >= SLICE_PREVIEW_PERF_THRESHOLD)
        {
            LogWarn("启用切片预览，但切片数量 ({}) 较多，可能会导致性能下降。", m_slices.size());
        }
    }

    
    ImGui::SameLine();
    ImGui::Dummy(ImVec2(20, 0));
    ImGui::SameLine();

    if (m_textureData)
    {
        ImGui::Text("纹理: %dx%d (%d通道) | 缩放: %.0f%%",
                    m_textureWidth, m_textureHeight, m_textureChannels, m_zoom * 100);
    }
    else
    {
        ImGui::Text("纹理: %s", m_texturePath.c_str());
    }
}

void TextureSlicerPanel::drawTexturePreview()
{
    PROFILE_FUNCTION();

    if (!m_textureData)
    {
        ImGui::TextColored(ImVec4(1, 0, 0, 1), "无法加载纹理图像");
        return;
    }

    ImVec2 availSize = ImGui::GetContentRegionAvail();


    float scaleX = (availSize.x - 20) / m_textureWidth;
    float scaleY = (availSize.y - 20) / m_textureHeight;
    float scale = std::min(scaleX, scaleY) * m_zoom;

    ImVec2 displaySize(m_textureWidth * scale, m_textureHeight * scale);


    ImVec2 cursorPos = ImGui::GetCursorScreenPos();
    ImVec2 imagePos(
        cursorPos.x + (availSize.x - displaySize.x) * 0.5f + m_panX,
        cursorPos.y + (availSize.y - displaySize.y) * 0.5f + m_panY
    );

    ImDrawList* drawList = ImGui::GetWindowDrawList();


    drawList->AddRectFilled(
        imagePos,
        ImVec2(imagePos.x + displaySize.x, imagePos.y + displaySize.y),
        IM_COL32(50, 50, 50, 255)
    );


    if (m_textureID != -1)
    {
        drawList->AddImage(
            m_textureID,
            imagePos,
            ImVec2(imagePos.x + displaySize.x, imagePos.y + displaySize.y),
            ImVec2(0, 0),
            ImVec2(1, 1)
        );
    }
    else
    {
        ImVec2 textPos(imagePos.x + displaySize.x * 0.5f - 100, imagePos.y + displaySize.y * 0.5f);
        drawList->AddText(textPos, IM_COL32(255, 200, 0, 255), "GPU纹理加载中...");
    }


    drawList->AddRect(
        imagePos,
        ImVec2(imagePos.x + displaySize.x, imagePos.y + displaySize.y),
        IM_COL32(150, 150, 150, 255), 0.0f, 0, 1.0f
    );

    
    if (m_showSlicePreviews)
    {
        for (size_t i = 0; i < m_slices.size(); ++i)
        {
            const auto& slice = m_slices[i];

            ImVec2 sliceMin(
                imagePos.x + slice.rect.left() * scale,
                imagePos.y + slice.rect.top() * scale
            );
            ImVec2 sliceMax(
                imagePos.x + slice.rect.right() * scale,
                imagePos.y + slice.rect.bottom() * scale
            );

            ImU32 sliceColor = slice.selected ? IM_COL32(0, 255, 0, 255) : IM_COL32(255, 255, 0, 255);
            drawList->AddRect(sliceMin, sliceMax, sliceColor, 0.0f, 0, 2.0f);

            
            if (m_interactionMode == InteractionMode::Edit && slice.selected)
            {
                const float handleSize = 6.0f;
                
                ImVec2 corners[4] = {
                    sliceMin,
                    ImVec2(sliceMax.x, sliceMin.y),
                    sliceMax,
                    ImVec2(sliceMin.x, sliceMax.y)
                };
                for (int c = 0; c < 4; ++c)
                {
                    ImVec2 hmin(corners[c].x - handleSize, corners[c].y - handleSize);
                    ImVec2 hmax(corners[c].x + handleSize, corners[c].y + handleSize);
                    drawList->AddRectFilled(hmin, hmax, IM_COL32(0, 200, 255, 255));
                    drawList->AddRect(hmin, hmax, IM_COL32(0, 100, 150, 255));
                }
            }

            if ((sliceMax.x - sliceMin.x) > 15.0f)
            {
                char label[32];
                snprintf(label, sizeof(label), "%zu", i);
                drawList->AddText(sliceMin, IM_COL32(255, 255, 255, 255), label);
            }
        }
    }
    else if (m_slices.size() >= SLICE_PREVIEW_PERF_THRESHOLD)
    {
        ImVec2 textPos(imagePos.x + 5, imagePos.y + 5);
        drawList->AddRectFilled(ImVec2(textPos.x - 2, textPos.y - 2), ImVec2(textPos.x + 350, textPos.y + 20),
                                IM_COL32(0, 0, 0, 150));
        drawList->AddText(textPos, IM_COL32(255, 150, 0, 255), "切片预览已关闭 (切片数 > 500)");
    }


    
    if (m_interactionMode == InteractionMode::Create && m_isDragging)
    {
        ImVec2 dragStartScreen(
            imagePos.x + m_dragStartX * scale,
            imagePos.y + m_dragStartY * scale
        );
        ImVec2 dragEndScreen(
            imagePos.x + m_dragEndX * scale,
            imagePos.y + m_dragEndY * scale
        );

        drawList->AddRect(dragStartScreen, dragEndScreen, IM_COL32(0, 255, 255, 255), 0.0f, 0, 2.0f);
    }

    
    ImGui::SetCursorScreenPos(imagePos);
    ImGui::InvisibleButton("TextureCanvas", displaySize);

    if (ImGui::IsItemHovered())
    {
        float wheel = ImGui::GetIO().MouseWheel;
        if (wheel != 0)
        {
            m_zoom = std::clamp(m_zoom + wheel * 0.1f, 0.1f, 5.0f);
        }

        
        if (m_sliceMode == SliceMode::Manual && m_interactionMode == InteractionMode::Create)
        {
            handleManualSlicing(imagePos, displaySize, scale);
        }

        
        if (m_interactionMode == InteractionMode::Edit)
        {
            handlePreviewEditing(imagePos, displaySize, scale);
        }
    }

    ImVec2 tipPos(cursorPos.x + 10, cursorPos.y + availSize.y - 50);
    drawList->AddRectFilled(ImVec2(tipPos.x - 5, tipPos.y - 5),
                            ImVec2(tipPos.x + 420, tipPos.y + 40),
                            IM_COL32(0, 0, 0, 150));
    drawList->AddText(tipPos, IM_COL32(200, 200, 200, 255),
                      "滚轮: 缩放 | 创建: 左键拖拽 | 编辑: 拖拽移动/角落缩放");
    ImVec2 tipPos2(tipPos.x, tipPos.y + 20);
    char tipText[128];
    snprintf(tipText, sizeof(tipText), "缩放: %.0f%% | 切片数: %zu", m_zoom * 100, m_slices.size());
    drawList->AddText(tipPos2, IM_COL32(150, 200, 255, 255), tipText);
}






void TextureSlicerPanel::drawSettingsPanel()
{
    PROFILE_FUNCTION();

    ImGui::Text("设置");
    ImGui::Separator();

    ImGui::InputText("切片名称前缀", m_namePrefix, sizeof(m_namePrefix));

    ImGui::Spacing();

    
    ImGui::Text("网格切片设置");
    ImGui::Spacing();

    if (ImGui::Checkbox("使用像素单元大小", &m_usePixelGrid))
    {
        if (m_usePixelGrid && m_textureData)
        {
            m_gridColumns = std::max(1, m_textureWidth / m_cellWidth);
            m_gridRows = std::max(1, m_textureHeight / m_cellHeight);
        }
    }

    ImGui::Spacing();

    bool changed = false;

    if (m_usePixelGrid)
    {
        ImGui::Text("每个切片单元的像素大小：");
        changed |= ImGui::InputInt("单元宽度 (px)", &m_cellWidth);
        changed |= ImGui::InputInt("单元高度 (px)", &m_cellHeight);

        m_cellWidth = std::max(1, m_cellWidth);
        m_cellHeight = std::max(1, m_cellHeight);

        if (m_textureData && changed)
        {
            m_gridColumns = std::max(1, m_textureWidth / m_cellWidth);
            m_gridRows = std::max(1, m_textureHeight / m_cellHeight);
        }

        ImGui::TextColored(ImVec4(0.5f, 0.8f, 1.0f, 1.0f),
                           "将生成 %d x %d = %d 个切片",
                           m_gridColumns, m_gridRows, m_gridColumns * m_gridRows);
    }
    else
    {
        ImGui::Text("网格行列数");
        changed |= ImGui::InputInt("行数", &m_gridRows);
        changed |= ImGui::InputInt("列数", &m_gridColumns);

        m_gridRows = std::max(1, m_gridRows);
        m_gridColumns = std::max(1, m_gridColumns);

        if (m_textureData)
        {
            int sliceWidth = m_textureWidth / m_gridColumns;
            int sliceHeight = m_textureHeight / m_gridRows;
            ImGui::TextColored(ImVec4(0.5f, 0.8f, 1.0f, 1.0f),
                               "每个切片: %d x %d 像素", sliceWidth, sliceHeight);
        }
    }

    if (changed)
    {
        generateGridSlices();
    }

    if (ImGui::Button("生成网格", ImVec2(-1, 0)))
    {
        generateGridSlices();
    }

    ImGui::Spacing();

    
    if (m_sliceMode == SliceMode::Manual)
    {
        ImGui::Text("手动切片");
        ImGui::TextWrapped("在左侧图像上按住鼠标左键拖拽以创建切片区域（创建模式）；编辑模式用于移动/缩放已存在切片。");

        
        if (ImGui::Button("清除所有切片（工具栏已提供）", ImVec2(-1, 0)))
        {
            m_slices.clear();
            m_selectedSliceIndex = -1;
        }

        if (m_selectedSliceIndex >= 0 && m_selectedSliceIndex < static_cast<int>(m_slices.size()))
        {
            if (ImGui::Button("删除选中切片", ImVec2(-1, 0)))
            {
                m_slices.erase(m_slices.begin() + m_selectedSliceIndex);
                m_selectedSliceIndex = -1;
            }
        }
    }

    ImGui::Separator();
    ImGui::Text("编辑选中切片（应用前可修改）");
    if (m_selectedSliceIndex >= 0 && m_selectedSliceIndex < static_cast<int>(m_slices.size()))
    {
        SliceRect& s = m_slices[m_selectedSliceIndex];

        float left = s.rect.left();
        float top = s.rect.top();
        float width = s.rect.width();
        float height = s.rect.height();

        if (ImGui::InputFloat("Left (px)", &left))
        {
            left = std::clamp(left, 0.0f, static_cast<float>(m_textureWidth - 1));
        }
        if (ImGui::InputFloat("Top (px)", &top))
        {
            top = std::clamp(top, 0.0f, static_cast<float>(m_textureHeight - 1));
        }
        if (ImGui::InputFloat("Width (px)", &width))
        {
            width = std::max(1.0f, width);
            if (left + width > m_textureWidth) width = m_textureWidth - left;
        }
        if (ImGui::InputFloat("Height (px)", &height))
        {
            height = std::max(1.0f, height);
            if (top + height > m_textureHeight) height = m_textureHeight - top;
        }

        s.rect = SimpleRect(left, top, width, height);

        char nameBuf[256];
        strncpy(nameBuf, s.name.c_str(), sizeof(nameBuf));
        nameBuf[sizeof(nameBuf) - 1] = '\0';
        if (ImGui::InputText("Name", nameBuf, sizeof(nameBuf)))
        {
            s.name = std::string(nameBuf);
        }

        if (ImGui::Button("将选中切片居中在纹理中", ImVec2(-1, 0)))
        {
            float centerX = (m_textureWidth - width) * 0.5f;
            float centerY = (m_textureHeight - height) * 0.5f;
            s.rect = SimpleRect(centerX, centerY, width, height);
        }

        ImGui::SameLine();
        if (ImGui::Button("对齐到像素网格", ImVec2(-1, 0)))
        {
            float l = std::round(s.rect.left());
            float t = std::round(s.rect.top());
            float w = std::round(s.rect.width());
            float h = std::round(s.rect.height());
            if (l + w > m_textureWidth) w = m_textureWidth - l;
            if (t + h > m_textureHeight) h = m_textureHeight - t;
            s.rect = SimpleRect(l, t, w, h);
        }
    }
    else
    {
        ImGui::TextDisabled("未选择切片");
    }
}

void TextureSlicerPanel::drawSliceList()
{
    PROFILE_FUNCTION();

    ImGui::Text("切片列表 (%zu)", m_slices.size());
    ImGui::Separator();

    ImGui::BeginChild("SliceListScroll", ImVec2(0, 0), false);

    const float thumbnailSize = 48.0f;
    ImDrawList* drawList = ImGui::GetWindowDrawList();

    for (size_t i = 0; i < m_slices.size(); ++i)
    {
        auto& slice = m_slices[i];

        ImGui::PushID(static_cast<int>(i));

        ImVec2 cursorPos = ImGui::GetCursorScreenPos();

        bool selected = (m_selectedSliceIndex == static_cast<int>(i));

        if (selected)
        {
            ImVec2 selectSize(ImGui::GetContentRegionAvail().x, thumbnailSize + 8);
            drawList->AddRectFilled(cursorPos, ImVec2(cursorPos.x + selectSize.x, cursorPos.y + selectSize.y),
                                    IM_COL32(50, 100, 200, 100));
        }

        ImVec2 thumbStart(cursorPos.x + 4, cursorPos.y + 4);
        ImVec2 thumbEnd(thumbStart.x + thumbnailSize, thumbStart.y + thumbnailSize);

        if (m_textureID != -1 && m_textureWidth > 0 && m_textureHeight > 0)
        {
            const auto& rect = slice.rect;
            ImVec2 uv0(rect.left() / m_textureWidth, rect.top() / m_textureHeight);
            ImVec2 uv1(rect.right() / m_textureWidth, rect.bottom() / m_textureHeight);

            drawList->AddImage(
                m_textureID,
                thumbStart,
                thumbEnd,
                uv0,
                uv1,
                IM_COL32_WHITE
            );
        }
        else
        {
            drawList->AddRectFilled(thumbStart, thumbEnd, IM_COL32(60, 60, 60, 255));
        }

        drawList->AddRect(thumbStart, thumbEnd, IM_COL32(200, 200, 200, 255));

        ImGui::SetCursorScreenPos(ImVec2(cursorPos.x + thumbnailSize + 12, cursorPos.y + 4));
        ImGui::Text("%s", slice.name.c_str());
        ImGui::SetCursorScreenPos(ImVec2(cursorPos.x + thumbnailSize + 12, cursorPos.y + 24));
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "%.0fx%.0f", slice.rect.width(), slice.rect.height());

        ImGui::SetCursorScreenPos(cursorPos);
        
        if (ImGui::InvisibleButton("##SliceItem", ImVec2(ImGui::GetContentRegionAvail().x, thumbnailSize + 8)))
        {
            
            if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
            {
                if (m_selectedSliceIndex >= 0 && m_selectedSliceIndex < static_cast<int>(m_slices.size()))
                {
                    m_slices[m_selectedSliceIndex].selected = false;
                }
                m_selectedSliceIndex = static_cast<int>(i);
                slice.selected = true;
            }
        }

        
        if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right))
        {
            
            ImGui::OpenPopup(("slice_context_" + std::to_string(i)).c_str());
        }
        
        if (ImGui::BeginPopup(("slice_context_" + std::to_string(i)).c_str()))
        {
            if (ImGui::MenuItem("删除切片"))
            {
                
                if (static_cast<int>(i) == m_selectedSliceIndex)
                {
                    m_selectedSliceIndex = -1;
                }
                m_slices.erase(m_slices.begin() + i);
                ImGui::EndPopup();
                ImGui::PopID();
                
                break;
            }
            if (ImGui::MenuItem("重命名"))
            {
                
                if (m_selectedSliceIndex >= 0 && m_selectedSliceIndex < static_cast<int>(m_slices.size()))
                    m_slices[m_selectedSliceIndex].selected = false;
                m_selectedSliceIndex = static_cast<int>(i);
                m_slices[m_selectedSliceIndex].selected = true;
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }

        if (ImGui::IsItemHovered())
        {
            ImGui::SetTooltip("位置: (%.0f, %.0f)\n大小: %.0f x %.0f\n右键: 操作菜单",
                              slice.rect.left(), slice.rect.top(),
                              slice.rect.width(), slice.rect.height());
        }

        ImGui::PopID();
    }

    ImGui::EndChild();
}

void TextureSlicerPanel::applySlices()
{
    PROFILE_FUNCTION();

    if (m_slices.empty())
    {
        LogWarn("没有切片可以应用");
        return;
    }

    if (!m_textureData)
    {
        LogError("纹理图像无效");
        return;
    }

    for (size_t i = 0; i < m_slices.size(); ++i)
    {
        saveSlice(m_slices[i], static_cast<int>(i));
    }

    LogInfo("成功保存 {} 个切片", m_slices.size());
}

void TextureSlicerPanel::generateGridSlices()
{
    PROFILE_FUNCTION();

    if (!m_textureData)
        return;

    m_slices.clear();

    float sliceWidth, sliceHeight;

    if (m_usePixelGrid)
    {
        sliceWidth = static_cast<float>(m_cellWidth);
        sliceHeight = static_cast<float>(m_cellHeight);


        m_gridColumns = (m_textureWidth + m_cellWidth - 1) / m_cellWidth;
        m_gridRows = (m_textureHeight + m_cellHeight - 1) / m_cellHeight;
    }
    else
    {
        sliceWidth = static_cast<float>(m_textureWidth) / m_gridColumns;
        sliceHeight = static_cast<float>(m_textureHeight) / m_gridRows;
    }

    for (int row = 0; row < m_gridRows; ++row)
    {
        for (int col = 0; col < m_gridColumns; ++col)
        {
            SliceRect slice;

            float x = col * sliceWidth;
            float y = row * sliceHeight;
            float w = sliceWidth;
            float h = sliceHeight;


            if (m_usePixelGrid)
            {
                if (x + w > m_textureWidth)
                    w = m_textureWidth - x;
                if (y + h > m_textureHeight)
                    h = m_textureHeight - y;


                if (w < 1 || h < 1)
                    continue;
            }

            slice.rect = SimpleRect(x, y, w, h);

            char nameBuf[256];
            snprintf(nameBuf, sizeof(nameBuf), "%s_%d_%d", m_namePrefix, row, col);
            slice.name = nameBuf;

            m_slices.push_back(slice);
        }
    }

    if (m_usePixelGrid)
    {
        LogInfo("生成了 {} 个像素级网格切片 (每个 {}x{} 像素)",
                m_slices.size(), m_cellWidth, m_cellHeight);
    }
    else
    {
        LogInfo("生成了 {} 个网格切片 ({}x{})", m_slices.size(), m_gridRows, m_gridColumns);
    }

    m_showSlicePreviews = (m_slices.size() < SLICE_PREVIEW_PERF_THRESHOLD);
}



void TextureSlicerPanel::generatePixelSizeSlices(int sliceW, int sliceH)
{
    PROFILE_FUNCTION();

    if (!m_textureData)
        return;

    m_slices.clear();

    sliceW = std::max(1, sliceW);
    sliceH = std::max(1, sliceH);

    int cols = (m_textureWidth + sliceW - 1) / sliceW;
    int rows = (m_textureHeight + sliceH - 1) / sliceH;

    for (int r = 0; r < rows; ++r)
    {
        for (int c = 0; c < cols; ++c)
        {
            float x = c * sliceW;
            float y = r * sliceH;
            float w = static_cast<float>(sliceW);
            float h = static_cast<float>(sliceH);

            if (x + w > m_textureWidth) w = m_textureWidth - x;
            if (y + h > m_textureHeight) h = m_textureHeight - y;

            if (w < 1 || h < 1) continue;

            SliceRect slice;
            slice.rect = SimpleRect(x, y, w, h);

            char nameBuf[256];
            snprintf(nameBuf, sizeof(nameBuf), "%s_%d_%d", m_namePrefix, r, c);
            slice.name = nameBuf;

            m_slices.push_back(slice);
        }
    }

    LogInfo("按像素尺寸生成 {} 个切片 (每个 {}x{} px)", m_slices.size(), sliceW, sliceH);

    m_showSlicePreviews = (m_slices.size() < SLICE_PREVIEW_PERF_THRESHOLD);
}


void TextureSlicerPanel::handleManualSlicing(ImVec2 imagePos, ImVec2 imageSize, float scale)
{
    if (!m_textureData)
        return;

    ImVec2 mousePos = ImGui::GetMousePos();


    float relX = (mousePos.x - imagePos.x) / imageSize.x;
    float relY = (mousePos.y - imagePos.y) / imageSize.y;


    float texX = relX * m_textureWidth;
    float texY = relY * m_textureHeight;


    texX = std::clamp(texX, 0.0f, static_cast<float>(m_textureWidth));
    texY = std::clamp(texY, 0.0f, static_cast<float>(m_textureHeight));

    if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
    {
        m_isDragging = true;
        m_dragStartX = texX;
        m_dragStartY = texY;
        m_dragEndX = texX;
        m_dragEndY = texY;
    }

    if (m_isDragging)
    {
        m_dragEndX = texX;
        m_dragEndY = texY;

        if (ImGui::IsMouseReleased(ImGuiMouseButton_Left))
        {
            m_isDragging = false;


            SliceRect slice;
            float left = std::min(m_dragStartX, m_dragEndX);
            float top = std::min(m_dragStartY, m_dragEndY);
            float right = std::max(m_dragStartX, m_dragEndX);
            float bottom = std::max(m_dragStartY, m_dragEndY);

            slice.rect = SimpleRect(left, top, right - left, bottom - top);


            if (slice.rect.width() > 5 && slice.rect.height() > 5)
            {
                char nameBuf[256];
                snprintf(nameBuf, sizeof(nameBuf), "%s_%d", m_namePrefix,
                         static_cast<int>(m_slices.size()));
                slice.name = nameBuf;

                m_slices.push_back(slice);
                LogInfo("添加手动切片: {}", slice.name);
            }
        }
    }
}


void TextureSlicerPanel::handlePreviewEditing(ImVec2 imagePos, ImVec2 imageSize, float scale)
{
    if (!m_textureData)
        return;

    ImVec2 mousePos = ImGui::GetMousePos();

    
    float texX = (mousePos.x - imagePos.x) / scale;
    float texY = (mousePos.y - imagePos.y) / scale;
    texX = std::clamp(texX, 0.0f, static_cast<float>(m_textureWidth));
    texY = std::clamp(texY, 0.0f, static_cast<float>(m_textureHeight));

    bool clicked = ImGui::IsMouseClicked(ImGuiMouseButton_Left);
    bool released = ImGui::IsMouseReleased(ImGuiMouseButton_Left);
    bool down = ImGui::IsMouseDown(ImGuiMouseButton_Left);

    
    if (clicked && m_selectedSliceIndex >= 0 && m_selectedSliceIndex < static_cast<int>(m_slices.size()))
    {
        SliceRect& s = m_slices[m_selectedSliceIndex];
        
        float cornersX[4] = {s.rect.left(), s.rect.right(), s.rect.right(), s.rect.left()};
        float cornersY[4] = {s.rect.top(), s.rect.top(), s.rect.bottom(), s.rect.bottom()};
        const float handleRadiusPx = 6.0f / scale; 
        for (int c = 0; c < 4; ++c)
        {
            float dx = texX - cornersX[c];
            float dy = texY - cornersY[c];
            if (dx * dx + dy * dy <= handleRadiusPx * handleRadiusPx)
            {
                
                m_isResizingSlice = true;
                m_isMovingSlice = false;
                m_resizeCorner = c;
                m_moveStartMouseX = texX;
                m_moveStartMouseY = texY;
                m_moveStartRect = s.rect;
                return;
            }
        }
    }

    
    if (m_isResizingSlice)
    {
        if (!down)
        {
            
            m_isResizingSlice = false;
            m_resizeCorner = -1;
            return;
        }

        SliceRect& s = m_slices[m_selectedSliceIndex];
        SimpleRect r = m_moveStartRect;

        
        switch (m_resizeCorner)
        {
        case 0: 
            {
                float newLeft = std::clamp(texX, 0.0f, r.right() - 1.0f);
                float newTop = std::clamp(texY, 0.0f, r.bottom() - 1.0f);
                float newW = r.right() - newLeft;
                float newH = r.bottom() - newTop;
                s.rect = SimpleRect(newLeft, newTop, newW, newH);
                break;
            }
        case 1: 
            {
                float newRight = std::clamp(texX, r.left() + 1.0f, static_cast<float>(m_textureWidth));
                float newTop = std::clamp(texY, 0.0f, r.bottom() - 1.0f);
                float newW = newRight - r.left();
                float newH = r.bottom() - newTop;
                s.rect = SimpleRect(r.left(), newTop, newW, newH);
                break;
            }
        case 2: 
            {
                float newRight = std::clamp(texX, r.left() + 1.0f, static_cast<float>(m_textureWidth));
                float newBottom = std::clamp(texY, r.top() + 1.0f, static_cast<float>(m_textureHeight));
                float newW = newRight - r.left();
                float newH = newBottom - r.top();
                s.rect = SimpleRect(r.left(), r.top(), newW, newH);
                break;
            }
        case 3: 
            {
                float newLeft = std::clamp(texX, 0.0f, r.right() - 1.0f);
                float newBottom = std::clamp(texY, r.top() + 1.0f, static_cast<float>(m_textureHeight));
                float newW = r.right() - newLeft;
                float newH = newBottom - r.top();
                s.rect = SimpleRect(newLeft, r.top(), newW, newH);
                break;
            }
        default:
            break;
        }

        return;
    }

    
    if (clicked)
    {
        
        int hitIndex = -1;
        for (int i = static_cast<int>(m_slices.size()) - 1; i >= 0; --i)
        {
            const SliceRect& s = m_slices[i];
            if (texX >= s.rect.left() && texX <= s.rect.right() &&
                texY >= s.rect.top() && texY <= s.rect.bottom())
            {
                hitIndex = i;
                break;
            }
        }

        if (hitIndex >= 0)
        {
            
            if (m_selectedSliceIndex >= 0 && m_selectedSliceIndex < static_cast<int>(m_slices.size()))
                m_slices[m_selectedSliceIndex].selected = false;
            m_selectedSliceIndex = hitIndex;
            m_slices[m_selectedSliceIndex].selected = true;

            
            m_isMovingSlice = true;
            m_isResizingSlice = false;
            m_moveStartMouseX = texX;
            m_moveStartMouseY = texY;
            m_moveStartRect = m_slices[m_selectedSliceIndex].rect;
        }
        else
        {
            
            if (m_selectedSliceIndex >= 0 && m_selectedSliceIndex < static_cast<int>(m_slices.size()))
            {
                m_slices[m_selectedSliceIndex].selected = false;
            }
            m_selectedSliceIndex = -1;
        }
    }

    
    if (m_isMovingSlice)
    {
        if (!down)
        {
            
            m_isMovingSlice = false;
            return;
        }

        if (m_selectedSliceIndex < 0 || m_selectedSliceIndex >= static_cast<int>(m_slices.size()))
        {
            m_isMovingSlice = false;
            return;
        }

        SliceRect& s = m_slices[m_selectedSliceIndex];

        float dx = texX - m_moveStartMouseX;
        float dy = texY - m_moveStartMouseY;

        float newLeft = m_moveStartRect.left() + dx;
        float newTop = m_moveStartRect.top() + dy;

        
        if (newLeft < 0) newLeft = 0;
        if (newTop < 0) newTop = 0;
        if (newLeft + m_moveStartRect.width() > m_textureWidth) newLeft = m_textureWidth - m_moveStartRect.width();
        if (newTop + m_moveStartRect.height() > m_textureHeight) newTop = m_textureHeight - m_moveStartRect.height();

        s.rect = SimpleRect(newLeft, newTop, m_moveStartRect.width(), m_moveStartRect.height());
    }
}

void TextureSlicerPanel::saveSlice(const SliceRect& slice, int index)
{
    if (!m_textureData)
        return;


    int left = static_cast<int>(slice.rect.left());
    int top = static_cast<int>(slice.rect.top());
    int width = static_cast<int>(slice.rect.width());
    int height = static_cast<int>(slice.rect.height());


    left = std::clamp(left, 0, m_textureWidth - 1);
    top = std::clamp(top, 0, m_textureHeight - 1);
    width = std::clamp(width, 1, m_textureWidth - left);
    height = std::clamp(height, 1, m_textureHeight - top);

    if (width <= 0 || height <= 0)
    {
        LogWarn("切片 {} 区域无效", slice.name);
        return;
    }


    int channels = m_textureChannels;
    if (channels != 4)
    {
        LogError("saveSlice: 内部状态错误，纹理通道不为 4");
        return;
    }

    unsigned char* sliceData = new unsigned char[width * height * channels];

    for (int y = 0; y < height; ++y)
    {
        for (int x = 0; x < width; ++x)
        {
            int srcX = left + x;
            int srcY = top + y;
            int srcIdx = (srcY * m_textureWidth + srcX) * channels;
            int dstIdx = (y * width + x) * channels;


            sliceData[dstIdx + 0] = m_textureData[srcIdx + 0];
            sliceData[dstIdx + 1] = m_textureData[srcIdx + 1];
            sliceData[dstIdx + 2] = m_textureData[srcIdx + 2];
            sliceData[dstIdx + 3] = m_textureData[srcIdx + 3];
        }
    }


    std::filesystem::path outputPath = std::filesystem::path(m_texturePath).parent_path();
    std::string filename = slice.name + ".png";
    outputPath /= filename;


    int result = stbi_write_png(outputPath.string().c_str(), width, height, channels,
                                sliceData, width * channels);

    delete[] sliceData;

    if (!result)
    {
        LogError("无法保存PNG: {}", outputPath.string());
        return;
    }

    LogInfo("保存切片: {} -> {}", slice.name, outputPath.string());
}

void TextureSlicerPanel::loadTexture()
{
    PROFILE_FUNCTION();

    auto* metadata = AssetManager::GetInstance().GetMetadata(m_currentTextureGuid);
    if (!metadata)
    {
        LogError("无法找到纹理元数据");
        return;
    }

    m_texturePath = (AssetManager::GetInstance().GetAssetsRootPath() / metadata->assetPath).string();


    int originalChannels = 0;
    m_textureData = stbi_load(m_texturePath.c_str(), &m_textureWidth, &m_textureHeight,
                              &originalChannels, 4);

    if (!m_textureData)
    {
        LogError("无法读取纹理文件: {}", m_texturePath);
        return;
    }


    m_textureChannels = 4;

    LogInfo("成功加载纹理数据: {} ({}x{}, {} channels -> 4 channels)", m_texturePath,
            m_textureWidth, m_textureHeight, originalChannels);


    if (m_context && m_context->graphicsBackend)
    {
        try
        {
            if (wgpu::Texture gpuTexture = m_context->graphicsBackend->LoadTextureFromFile(m_texturePath))
            {
                m_gpuTexture = gpuTexture;


                if (m_context->imguiRenderer)
                {
                    m_textureID = m_context->imguiRenderer->GetOrCreateTextureIdFor(gpuTexture);
                    LogInfo("成功创建 GPU 纹理预览");
                }
                else
                {
                    LogWarn("ImGuiRenderer 不可用，使用后备预览方式");
                }
            }
            else
            {
                LogWarn("无法创建 GPU 纹理，将使用后备预览方式");
            }
        }
        catch (const std::exception& e)
        {
            LogError("创建 GPU 纹理时发生异常: {}", e.what());
        }
    }
    else
    {
        LogWarn("GraphicsBackend 不可用，将使用后V预览方式");
    }

    
    if (m_defaultSliceWidthPx <= 0) m_defaultSliceWidthPx = 64;
    if (m_defaultSliceHeightPx <= 0) m_defaultSliceHeightPx = 64;
}
