#include "AnimationEditorPanel.h"
#include "../Utils/Logger.h"
#include "../Resources/Loaders/AnimationClipLoader.h"
#include "../Utils/PopupManager.h"
#include <imgui.h>
#include <algorithm>
#include "../Resources/RuntimeAsset/RuntimeScene.h"
#include "AssetManager.h"
#include "ComponentRegistry.h"
#include "SceneManager.h"
#include "Sprite.h"
#include "Loaders/TextureLoader.h"

void AnimationEditorPanel::Initialize(EditorContext* context)
{
    m_context = context;
    m_textureLoader = std::make_unique<TextureLoader>(*m_context->graphicsBackend);

    m_totalFrames = 60;
}

void AnimationEditorPanel::Update(float deltaTime)
{
    if (!m_isVisible)
        return;


    if (m_context->currentEditingAnimationClipGuid.Valid() &&
        m_context->currentEditingAnimationClipGuid != m_currentClipGuid)
    {
        openAnimationClipFromContext(m_context->currentEditingAnimationClipGuid);
    }


    if (!m_context->currentEditingAnimationClipGuid.Valid() && m_currentClip)
    {
        closeCurrentClipFromContext();
    }


    updateTargetObject();


    if (m_currentClip)
    {
        updatePlayback(deltaTime);
    }
}

void AnimationEditorPanel::Shutdown()
{
    CloseCurrentClip();
}

void AnimationEditorPanel::OpenAnimationClip(const Guid& clipGuid)
{
    m_context->currentEditingAnimationClipGuid = clipGuid;
}

void AnimationEditorPanel::CloseCurrentClip()
{
    m_context->currentEditingAnimationClipGuid = Guid();
}

void AnimationEditorPanel::Focus()
{
    m_isVisible = true;
    m_requestFocus = true;
}

void AnimationEditorPanel::openAnimationClipFromContext(const Guid& clipGuid)
{
    if (m_currentClipGuid == clipGuid && m_currentClip)
        return;

    closeCurrentClipFromContext();

    auto loader = AnimationClipLoader();
    m_currentClip = loader.LoadAsset(clipGuid);

    if (!m_currentClip)
    {
        LogError("无法加载动画切片，GUID: {}", clipGuid.ToString());
        m_context->currentEditingAnimationClipGuid = Guid();
        return;
    }

    m_currentClipGuid = clipGuid;
    m_currentClipName = m_currentClip->getAnimationClip().Name;
    m_targetObjectGuid = m_currentClip->getAnimationClip().TargetEntityGuid;
    m_currentTime = 0.0f;
    m_currentFrame = 0;
    m_totalFrames = 60;
    for (const auto& [frameIndex, frame] : m_currentClip->getAnimationClip().Frames)
    {
        m_totalFrames = std::max(m_totalFrames, frameIndex + 1);
    }
    m_multiSelectedFrames.clear();
    m_frameEditWindowOpen = false;

    LogInfo("打开动画切片进行编辑: {}", m_currentClipName);
}

void AnimationEditorPanel::closeCurrentClipFromContext()
{
    if (!m_currentClip)
        return;

    LogInfo("关闭动画切片: {}", m_currentClipName);

    m_currentClip = nullptr;
    m_currentClipGuid = Guid();
    m_currentClipName.clear();
    m_targetObjectGuid = Guid();
    m_targetObjectName.clear();
    m_currentTime = 0.0f;
    m_currentFrame = 0;
    m_totalFrames = 60;
    m_isPlaying = false;
    m_multiSelectedFrames.clear();
    m_frameEditWindowOpen = false;
}

void AnimationEditorPanel::createNewAnimation()
{
    AnimationClip newClip;
    newClip.Name = "新动画";


    if (!m_context->selectionList.empty() && m_context->selectionType == SelectionType::GameObject)
    {
        newClip.TargetEntityGuid = m_context->selectionList[0];
    }
    else
    {
        newClip.TargetEntityGuid = Guid::NewGuid();
    }


    Guid newGuid = Guid::NewGuid();
    m_currentClip = sk_make_sp<RuntimeAnimationClip>(newGuid, newClip);
    m_currentClipGuid = newGuid;
    m_currentClipName = newClip.Name;
    m_targetObjectGuid = newClip.TargetEntityGuid;


    m_currentTime = 0.0f;
    m_currentFrame = 0;
    m_totalFrames = 60;
    m_selectedFrameIndex = -1;


    m_context->currentEditingAnimationClipGuid = newGuid;

    LogInfo("创建新动画: {}", m_currentClipName);
}

void AnimationEditorPanel::drawTargetObjectSelector()
{
    ImGui::Text("目标物体:");
    ImGui::SameLine();

    if (hasValidTargetObject())
    {
        ImGui::Text("%s", m_targetObjectName.c_str());
        ImGui::SameLine();

        if (ImGui::Button("选中"))
        {
            m_context->selectionType = SelectionType::GameObject;
            m_context->selectionList.clear();
            m_context->selectionList.push_back(m_targetObjectGuid);
            m_context->selectionAnchor = m_targetObjectGuid;
            m_context->objectToFocusInHierarchy = m_targetObjectGuid;
        }
    }
    else
    {
        ImGui::Text("没有有效的目标物体");
    }

    ImGui::SameLine();
    if (ImGui::Button("从选中设置"))
    {
        if (!m_context->selectionList.empty() && m_context->selectionType == SelectionType::GameObject)
        {
            m_targetObjectGuid = m_context->selectionList[0];


            if (m_currentClip)
            {
                m_currentClip->getAnimationClip().TargetEntityGuid = m_targetObjectGuid;
            }

            LogInfo("设置目标物体为当前选中的物体");
        }
        else
        {
            LogWarn("请先在层级面板中选择一个物体");
        }
    }
}


void AnimationEditorPanel::drawControlPanel()
{
    if (m_currentClip)
    {
        char nameBuffer[256];
        strncpy(nameBuffer, m_currentClipName.c_str(), sizeof(nameBuffer));
        nameBuffer[sizeof(nameBuffer) - 1] = '\0';

        ImGui::Text("动画名称:");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(220.0f);


        if (ImGui::InputText("##AnimationNameEditor", nameBuffer, sizeof(nameBuffer)))
        {
            m_currentClipName = nameBuffer;
            m_currentClip->getAnimationClip().Name = m_currentClipName;
        }
    }
    else
    {
        ImGui::Text("没有打开的动画");
    }

    ImGui::SameLine();


    if (ImGui::Button(m_isPlaying ? "暂停" : "播放"))
    {
        m_isPlaying = !m_isPlaying;
    }

    ImGui::SameLine();
    if (ImGui::Button("停止"))
    {
        m_isPlaying = false;
        seekToFrame(0);
    }

    ImGui::SameLine();
    if (ImGui::Button("前一帧"))
    {
        seekToFrame(std::max(0, m_currentFrame - 1));
    }

    ImGui::SameLine();
    if (ImGui::Button("后一帧"))
    {
        seekToFrame(std::min(m_totalFrames - 1, m_currentFrame + 1));
    }

    ImGui::SameLine();
    ImGui::Checkbox("循环", &m_isLooping);


    ImGui::SetNextItemWidth(100);
    if (ImGui::DragFloat("帧率", &m_frameRate, 1.0f, 1.0f, 120.0f, "%.1f"))
    {
        m_frameRate = std::clamp(m_frameRate, 1.0f, 120.0f);
    }

    ImGui::SameLine();
    ImGui::Text("当前帧: %d / %d", m_currentFrame, m_totalFrames);


    ImGui::SetNextItemWidth(100);
    if (ImGui::DragInt("总帧数", &m_totalFrames, 1.0f, 1, 1000))
    {
        m_totalFrames = std::clamp(m_totalFrames, 1, 1000);

        m_currentFrame = std::clamp(m_currentFrame, 0, m_totalFrames - 1);
    }


    float maxTime = static_cast<float>(m_totalFrames) / m_frameRate;
    if (ImGui::SliderFloat("时间", &m_currentTime, 0.0f, maxTime, "%.2fs"))
    {
        m_currentFrame = static_cast<int>(m_currentTime * m_frameRate);
        m_currentFrame = std::clamp(m_currentFrame, 0, m_totalFrames - 1);
    }


    if (hasValidTargetObject() && m_currentClip)
    {
        ImGui::SameLine();
        if (ImGui::Button("应用"))
        {
            applyFrameToObject(m_currentFrame);
        }
    }
}

void AnimationEditorPanel::drawTimeline()
{
    ImGui::Text("时间轴 (总帧数: %d)", m_totalFrames);

    ImVec2 canvasPos = ImGui::GetCursorScreenPos();
    ImVec2 canvasSize = ImGui::GetContentRegionAvail();
    canvasSize.y = m_timelineHeight;

    ImDrawList* drawList = ImGui::GetWindowDrawList();
    const float keyframeRadius = 6.0f;
    const float rulerHeight = 25.0f;
    float pixelsPerFrame = 20.0f * m_timelineZoom;


    drawList->AddRectFilled(canvasPos, ImVec2(canvasPos.x + canvasSize.x, canvasPos.y + canvasSize.y),
                            IM_COL32(50, 50, 50, 255));


    int visibleFrameStart = (pixelsPerFrame > 0)
                                ? std::max(0, static_cast<int>(m_timelineScrollX / pixelsPerFrame))
                                : 0;
    int visibleFrameEnd = (pixelsPerFrame > 0)
                              ? std::min(m_totalFrames,
                                         static_cast<int>((m_timelineScrollX + canvasSize.x) / pixelsPerFrame) + 2)
                              : m_totalFrames;
    for (int frame = visibleFrameStart; frame < visibleFrameEnd; ++frame)
    {
        float x = canvasPos.x + (static_cast<float>(frame) * pixelsPerFrame) - m_timelineScrollX;
        if (x < canvasPos.x || x > canvasPos.x + canvasSize.x) continue;
        ImU32 lineColor = (frame % 10 == 0) ? IM_COL32(150, 150, 150, 255) : IM_COL32(100, 100, 100, 255);
        drawList->AddLine(ImVec2(x, canvasPos.y + rulerHeight), ImVec2(x, canvasPos.y + canvasSize.y), lineColor);
        if (frame % 5 == 0)
        {
            char buf[16];
            snprintf(buf, 16, "%d", frame);
            drawList->AddText(ImVec2(x + 2, canvasPos.y + 2), IM_COL32(200, 200, 200, 255), buf);
        }
    }


    if (m_currentClip)
    {
        for (const auto& [frameIndex, frameData] : m_currentClip->getAnimationClip().Frames)
        {
            float x = canvasPos.x + (static_cast<float>(frameIndex) * pixelsPerFrame) - m_timelineScrollX;
            if (x < canvasPos.x - keyframeRadius || x > canvasPos.x + canvasSize.x + keyframeRadius) continue;

            bool isSelected = m_multiSelectedFrames.count(frameIndex);
            ImU32 color = isSelected ? IM_COL32(255, 255, 0, 255) : IM_COL32(255, 100, 100, 255);
            drawList->AddCircleFilled(ImVec2(x, canvasPos.y + (canvasSize.y + rulerHeight) * 0.5f), keyframeRadius,
                                      color);
        }
    }


    ImGui::SetCursorScreenPos(canvasPos);
    ImGui::InvisibleButton("##TimelineCanvas", canvasSize);

    const bool isCanvasHovered = ImGui::IsItemHovered();
    const bool isCtrlDown = ImGui::GetIO().KeyCtrl;
    const float mouseXOnCanvas = ImGui::GetIO().MousePos.x - canvasPos.x;
    int hoveredFrame = (pixelsPerFrame > 0)
                           ? static_cast<int>(round((mouseXOnCanvas + m_timelineScrollX) / pixelsPerFrame))
                           : 0;
    hoveredFrame = std::clamp(hoveredFrame, 0, m_totalFrames - 1);

    int clickedOnFrame = -1;
    if (m_currentClip && isCanvasHovered && ImGui::GetIO().MousePos.y > canvasPos.y + rulerHeight)
    {
        for (const auto& [frameIndex, frameData] : m_currentClip->getAnimationClip().Frames)
        {
            float keyframeScreenX = canvasPos.x + (static_cast<float>(frameIndex) * pixelsPerFrame) - m_timelineScrollX;
            if (std::abs(ImGui::GetIO().MousePos.x - keyframeScreenX) < keyframeRadius)
            {
                clickedOnFrame = frameIndex;
                break;
            }
        }
    }


    if (isCanvasHovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
    {
        bool clickedOnRuler = ImGui::GetIO().MousePos.y < canvasPos.y + rulerHeight;
        if (clickedOnRuler)
        {
            m_isDraggingPlayhead = true;
            seekToFrame(hoveredFrame);
        }
        else if (clickedOnFrame != -1)
        {
            m_isDraggingKeyframe = true;
            m_dragHandleFrame = clickedOnFrame;

            if (isCtrlDown)
            {
                if (m_multiSelectedFrames.count(clickedOnFrame)) m_multiSelectedFrames.erase(clickedOnFrame);
                else m_multiSelectedFrames.insert(clickedOnFrame);
            }
            else if (!m_multiSelectedFrames.count(clickedOnFrame))
            {
                m_multiSelectedFrames.clear();
                m_multiSelectedFrames.insert(clickedOnFrame);
            }
            m_dragInitialSelectionState.assign(m_multiSelectedFrames.begin(), m_multiSelectedFrames.end());
            std::ranges::sort(m_dragInitialSelectionState);
        }
        else
        {
            m_isBoxSelecting = true;
            m_boxSelectionStart = ImGui::GetIO().MousePos;
            if (!isCtrlDown) m_multiSelectedFrames.clear();
        }
    }


    if (ImGui::IsMouseDragging(ImGuiMouseButton_Left))
    {
        if (m_isDraggingPlayhead)
        {
            seekToFrame(hoveredFrame);
        }
        else if (m_isDraggingKeyframe)
        {
        }
        else if (m_isBoxSelecting)
        {
            ImVec2 currentMousePos = ImGui::GetIO().MousePos;
            drawList->AddRectFilled(m_boxSelectionStart, currentMousePos, IM_COL32(100, 150, 255, 50));
            drawList->AddRect(m_boxSelectionStart, currentMousePos, IM_COL32(100, 150, 255, 150));
        }
    }


    if (m_isDraggingKeyframe && !m_dragInitialSelectionState.empty())
    {
        int draggedFrameNewPos = hoveredFrame;
        int firstSelectedFrame = m_dragInitialSelectionState.front();
        bool isTranslation = (m_dragHandleFrame == firstSelectedFrame);

        if (isTranslation)
        {
            int delta = draggedFrameNewPos - m_dragHandleFrame;
            for (int oldIndex : m_dragInitialSelectionState)
            {
                int newIndex = oldIndex + delta;
                float ghostX = canvasPos.x + (static_cast<float>(newIndex) * pixelsPerFrame) - m_timelineScrollX;
                drawList->AddCircleFilled(ImVec2(ghostX, canvasPos.y + (canvasSize.y + rulerHeight) * 0.5f),
                                          keyframeRadius,
                                          IM_COL32(255, 255, 0, 128));
            }
        }
        else
        {
            int anchorFrame = firstSelectedFrame;
            float oldSpan = static_cast<float>(m_dragHandleFrame - anchorFrame);
            float newSpan = static_cast<float>(draggedFrameNewPos - anchorFrame);

            if (std::abs(oldSpan) > 0.001f)
            {
                float scale = newSpan / oldSpan;
                for (int oldIndex : m_dragInitialSelectionState)
                {
                    int newIndex = anchorFrame + static_cast<int>(round((oldIndex - anchorFrame) * scale));
                    float ghostX = canvasPos.x + (static_cast<float>(newIndex) * pixelsPerFrame) - m_timelineScrollX;
                    drawList->AddCircleFilled(ImVec2(ghostX, canvasPos.y + (canvasSize.y + rulerHeight) * 0.5f),
                                              keyframeRadius,
                                              IM_COL32(255, 255, 0, 128));
                }
            }
        }
    }


    if (ImGui::IsMouseReleased(ImGuiMouseButton_Left))
    {
        if (m_isDraggingPlayhead) { m_isDraggingPlayhead = false; }
        if (m_isDraggingKeyframe)
        {
            if (m_currentClip && !m_dragInitialSelectionState.empty() && hoveredFrame != m_dragHandleFrame)
            {
                std::map<int, int> newPositions;
                int firstSelectedFrame = m_dragInitialSelectionState.front();

                bool isTranslation = (m_dragHandleFrame == firstSelectedFrame);

                if (isTranslation)
                {
                    int delta = hoveredFrame - m_dragHandleFrame;
                    for (int oldIndex : m_dragInitialSelectionState)
                    {
                        newPositions[oldIndex] = oldIndex + delta;
                    }
                }
                else
                {
                    int anchorFrame = firstSelectedFrame;
                    float oldSpan = static_cast<float>(m_dragHandleFrame - anchorFrame);
                    float newSpan = static_cast<float>(hoveredFrame - anchorFrame);

                    if (std::abs(oldSpan) > 0.001f)
                    {
                        float scale = newSpan / oldSpan;
                        for (int oldIndex : m_dragInitialSelectionState)
                        {
                            if (oldIndex == anchorFrame)
                            {
                                newPositions[oldIndex] = anchorFrame;
                            }
                            else
                            {
                                newPositions[oldIndex] = anchorFrame + static_cast<int>(round(
                                    (oldIndex - anchorFrame) * scale));
                            }
                        }
                    }
                    else
                    {
                        int delta = hoveredFrame - m_dragHandleFrame;
                        for (int oldIndex : m_dragInitialSelectionState) newPositions[oldIndex] = oldIndex + delta;
                    }
                }


                bool collision = false;
                std::set<int> finalDestinations;


                std::set<int> unselectedFrames;
                for (const auto& [frameIndex, frameData] : m_currentClip->getAnimationClip().Frames)
                {
                    if (m_multiSelectedFrames.count(frameIndex) == 0)
                    {
                        unselectedFrames.insert(frameIndex);
                    }
                }


                for (const auto& [oldIndex, newIndex] : newPositions)
                {
                    if (newIndex < 0)
                    {
                        collision = true;
                        break;
                    }

                    if (!finalDestinations.insert(newIndex).second)
                    {
                        collision = true;
                        break;
                    }

                    if (unselectedFrames.count(newIndex))
                    {
                        collision = true;
                        break;
                    }
                }


                if (!collision)
                {
                    auto& frames = m_currentClip->getAnimationClip().Frames;
                    std::vector<std::pair<int, AnimFrame>> framesToMove;

                    for (int oldIndex : m_dragInitialSelectionState)
                    {
                        framesToMove.push_back({newPositions.at(oldIndex), frames.at(oldIndex)});
                    }


                    for (int oldIndex : m_dragInitialSelectionState) frames.erase(oldIndex);


                    m_multiSelectedFrames.clear();
                    for (const auto& pair : framesToMove)
                    {
                        frames[pair.first] = pair.second;
                        m_multiSelectedFrames.insert(pair.first);
                    }
                    LogInfo("批量移动 {} 个关键帧", framesToMove.size());
                }
                else
                {
                    LogWarn("移动关键帧失败：发生碰撞或超出边界");
                }
            }
            m_isDraggingKeyframe = false;
        }
        else if (m_isBoxSelecting)
        {
            ImVec2 boxEnd = ImGui::GetIO().MousePos;
            ImVec2 boxMin(std::min(m_boxSelectionStart.x, boxEnd.x), std::min(m_boxSelectionStart.y, boxEnd.y));
            ImVec2 boxMax(std::max(m_boxSelectionStart.x, boxEnd.x), std::max(m_boxSelectionStart.y, boxEnd.y));
            if (m_currentClip)
            {
                for (const auto& [frameIndex, frameData] : m_currentClip->getAnimationClip().Frames)
                {
                    float keyframeScreenX = canvasPos.x + (static_cast<float>(frameIndex) * pixelsPerFrame) -
                        m_timelineScrollX;
                    float keyframeScreenY = canvasPos.y + (canvasSize.y + rulerHeight) * 0.5f;
                    if (keyframeScreenX >= boxMin.x && keyframeScreenX <= boxMax.x && keyframeScreenY >= boxMin.y &&
                        keyframeScreenY <= boxMax.y)
                    {
                        m_multiSelectedFrames.insert(frameIndex);
                    }
                }
            }
            m_isBoxSelecting = false;
        }
    }


    if (isCanvasHovered && !ImGui::IsMouseDown(ImGuiMouseButton_Left))
    {
        if (ImGui::IsMouseDragging(ImGuiMouseButton_Middle))
        {
            m_timelineScrollX -= ImGui::GetIO().MouseDelta.x;
            m_timelineScrollX = std::max(0.0f, m_timelineScrollX);
        }
        if (ImGui::GetIO().MouseWheel != 0.0f && pixelsPerFrame > 0)
        {
            float oldZoom = m_timelineZoom;
            m_timelineZoom += ImGui::GetIO().MouseWheel * 0.1f;
            m_timelineZoom = std::clamp(m_timelineZoom, 0.1f, 5.0f);
            if (oldZoom != m_timelineZoom)
            {
                float mouseX = mouseXOnCanvas;
                float frameAtMouse = (mouseX + m_timelineScrollX) / (20.0f * oldZoom);
                m_timelineScrollX = frameAtMouse * (20.0f * m_timelineZoom) - mouseX;
                m_timelineScrollX = std::max(0.0f, m_timelineScrollX);
            }
        }
    }


    float currentX = canvasPos.x + (static_cast<float>(m_currentFrame) * pixelsPerFrame) - m_timelineScrollX;
    if (currentX >= canvasPos.x && currentX <= canvasPos.x + canvasSize.x)
    {
        drawList->AddLine(ImVec2(currentX, canvasPos.y), ImVec2(currentX, canvasPos.y + canvasSize.y),
                          IM_COL32(255, 255, 255, 255), 2.0f);
        ImVec2 trianglePoints[3] = {
            {currentX - 5, canvasPos.y}, {currentX + 5, canvasPos.y}, {currentX, canvasPos.y + 10}
        };
        drawList->AddTriangleFilled(trianglePoints[0], trianglePoints[1], trianglePoints[2],
                                    IM_COL32(255, 255, 255, 255));
    }


    if (ImGui::BeginDragDropTarget())
    {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("DRAG_DROP_ASSET_HANDLES_MULTI"))
        {
            if (hasValidTargetObject() && m_currentClip)
            {
                size_t handleCount = payload->DataSize / sizeof(AssetHandle);
                const AssetHandle* handles = static_cast<const AssetHandle*>(payload->Data);

                auto scene = SceneManager::GetInstance().GetCurrentScene();
                auto targetObject = scene->FindGameObjectByGuid(m_targetObjectGuid);
                const auto* compInfo = ComponentRegistry::GetInstance().Get("SpriteComponent");

                if (compInfo)
                {
                    if (!targetObject.HasComponent<ECS::SpriteComponent>())
                    {
                        targetObject.AddComponent<ECS::SpriteComponent>();
                    }
                    auto& spriteComp = targetObject.GetComponent<ECS::SpriteComponent>();

                    int keyframesCreated = 0;
                    for (size_t i = 0; i < handleCount; ++i)
                    {
                        const AssetHandle& handle = handles[i];
                        if (handle.assetType != AssetType::Texture) continue;

                        int frameIndex = hoveredFrame + keyframesCreated;

                        spriteComp.textureHandle = handle;

                        AnimFrame& frame = m_currentClip->getAnimationClip().Frames[frameIndex];
                        frame.animationData["SpriteComponent"] = compInfo->serialize(
                            scene->GetRegistry(), targetObject.GetEntityHandle());

                        keyframesCreated++;
                    }

                    if (keyframesCreated > 0)
                    {
                        const AssetHandle& lastHandle = handles[handleCount - 1];
                        spriteComp.textureHandle = lastHandle;
                        LogInfo("通过批量拖拽创建了 {} 个连续的关键帧", keyframesCreated);
                    }
                }
            }
        }

        else if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("DRAG_DROP_ASSET_HANDLE"))
        {
            AssetHandle handle = *static_cast<const AssetHandle*>(payload->Data);
            if (handle.assetType == AssetType::Texture)
            {
                if (hasValidTargetObject() && m_currentClip)
                {
                    auto scene = SceneManager::GetInstance().GetCurrentScene();
                    auto targetObject = scene->FindGameObjectByGuid(m_targetObjectGuid);

                    if (!targetObject.HasComponent<ECS::SpriteComponent>())
                    {
                        targetObject.AddComponent<ECS::SpriteComponent>();
                    }
                    auto& spriteComp = targetObject.GetComponent<ECS::SpriteComponent>();

                    spriteComp.textureHandle = handle;

                    AnimFrame& frame = m_currentClip->getAnimationClip().Frames[hoveredFrame];

                    const auto* compInfo = ComponentRegistry::GetInstance().Get("SpriteComponent");
                    if (compInfo)
                    {
                        frame.animationData["SpriteComponent"] = compInfo->serialize(
                            scene->GetRegistry(), targetObject.GetEntityHandle());

                        m_multiSelectedFrames.clear();
                        m_multiSelectedFrames.insert(hoveredFrame);

                        LogInfo("拖放纹理到第 {} 帧，已记录SpriteComponent", hoveredFrame);
                    }
                }
                else
                {
                    LogWarn("无法拖放纹理：没有动画或无效的目标物体。");
                }
            }
        }
        ImGui::EndDragDropTarget();
    }


    ImGui::SetNextItemWidth(120);
    ImGui::SliderFloat("缩放", &m_timelineZoom, 0.1f, 5.0f, "%.1fx");
    ImGui::SameLine();
    if (ImGui::Button("跟随播放头")) { centerTimelineOnCurrentFrame(); }

    const char* fitAllText = "适应所有";
    float fitAllButtonWidth = ImGui::CalcTextSize(fitAllText).x + ImGui::GetStyle().FramePadding.x * 2.0f;
    float spacing = ImGui::GetContentRegionAvail().x - fitAllButtonWidth;
    ImGui::SameLine(0, spacing > 0 ? spacing : 0);

    if (ImGui::Button(fitAllText)) { fitTimelineToAllFrames(canvasSize.x); }
}


void AnimationEditorPanel::updatePlayback(float deltaTime)
{
    if (!m_isPlaying || m_totalFrames == 0)
        return;

    m_currentTime += deltaTime;
    float maxTime = static_cast<float>(m_totalFrames) / m_frameRate;

    if (m_currentTime >= maxTime)
    {
        if (m_isLooping)
        {
            m_currentTime = 0.0f;
        }
        else
        {
            m_currentTime = maxTime;
            m_isPlaying = false;
        }
    }

    m_currentFrame = static_cast<int>(m_currentTime * m_frameRate);
    m_currentFrame = std::clamp(m_currentFrame, 0, m_totalFrames - 1);


    if (hasValidTargetObject() && m_currentClip)
    {
        applyFrameToObject(m_currentFrame);
    }
}


void AnimationEditorPanel::seekToFrame(int frameIndex)
{
    int newFrame = std::clamp(frameIndex, 0, m_totalFrames - 1);


    if (newFrame != m_currentFrame)
    {
        m_currentFrame = newFrame;
        if (m_frameRate > 0)
        {
            m_currentTime = static_cast<float>(m_currentFrame) / m_frameRate;
        }
        else
        {
            m_currentTime = 0.0f;
        }
    }


    if (hasValidTargetObject() && m_currentClip)
    {
        applyFrameToObject(m_currentFrame);
    }
}

void AnimationEditorPanel::addKeyFrame(int frameIndex)
{
    if (!m_currentClip)
    {
        LogWarn("没有打开的动画切片，无法添加关键帧");
        return;
    }
    if (m_currentClip->getAnimationClip().Frames.find(frameIndex) != m_currentClip->getAnimationClip().Frames.end())
    {
        LogWarn("帧 {} 已存在关键帧", frameIndex);
        return;
    }
    AnimFrame newFrame;
    m_currentClip->getAnimationClip().Frames[frameIndex] = newFrame;
    m_multiSelectedFrames.clear();
    m_multiSelectedFrames.insert(frameIndex);

    LogInfo("添加空关键帧: {}", frameIndex);
}

void AnimationEditorPanel::removeKeyFrame(int frameIndex)
{
    if (!m_currentClip) return;
    auto it = m_currentClip->getAnimationClip().Frames.find(frameIndex);
    if (it == m_currentClip->getAnimationClip().Frames.end())
    {
        LogWarn("帧 {} 不存在关键帧", frameIndex);
        return;
    }
    m_currentClip->getAnimationClip().Frames.erase(it);
    m_multiSelectedFrames.erase(frameIndex);
    LogInfo("删除关键帧: {}", frameIndex);
}

void AnimationEditorPanel::copyFrameData(int fromFrame, int toFrame)
{
    if (!m_currentClip)
        return;

    auto fromIt = m_currentClip->getAnimationClip().Frames.find(fromFrame);
    if (fromIt == m_currentClip->getAnimationClip().Frames.end())
    {
        LogWarn("源帧 {} 不存在", fromFrame);
        return;
    }

    m_currentClip->getAnimationClip().Frames[toFrame] = fromIt->second;
    LogInfo("复制帧数据: {} -> {}", fromFrame, toFrame);
}

void AnimationEditorPanel::saveCurrentClip()
{
    if (!m_currentClip)
        return;


    AnimationClip& clipData = m_currentClip->getAnimationClip();


    if (clipData.Name.empty())
    {
        clipData.Name = m_currentClipName.empty() ? "未命名动画" : m_currentClipName;
    }


    if (!clipData.TargetEntityGuid.Valid() && m_targetObjectGuid.Valid())
    {
        clipData.TargetEntityGuid = m_targetObjectGuid;
    }

    auto meta = AssetManager::GetInstance().GetMetadata(m_currentClipGuid);
    if (!meta)
    {
        LogError("无法找到动画切片的元数据，GUID: {}", m_currentClipGuid.ToString());
        return;
    }

    auto path = AssetManager::GetInstance().GetAssetsRootPath() / meta->assetPath.filename();
    std::ofstream fout(path);
    if (!fout.is_open())
    {
        LogError("无法打开文件保存动画切片: {}", path.c_str());
        return;
    }


    std::string content = YAML::Dump(YAML::convert<AnimationClip>::encode(clipData));
    fout << content;
    fout.close();

    LogInfo("保存动画切片: {} (包含 {} 个关键帧)", clipData.Name, clipData.Frames.size());
}

void AnimationEditorPanel::centerTimelineOnCurrentFrame()
{
    float pixelsPerFrame = 20.0f * m_timelineZoom;
    m_timelineScrollX = m_currentFrame * pixelsPerFrame - 200.0f;
    m_timelineScrollX = std::max(0.0f, m_timelineScrollX);
}

void AnimationEditorPanel::fitTimelineToAllFrames(float viewWidth)
{
    if (m_totalFrames <= 0)
        return;

    float neededPixelsPerFrame = viewWidth / static_cast<float>(m_totalFrames);
    m_timelineZoom = neededPixelsPerFrame / 20.0f;
    m_timelineZoom = std::clamp(m_timelineZoom, 0.1f, 5.0f);
    m_timelineScrollX = 0.0f;
}

void AnimationEditorPanel::applyFrameToObject(int frameIndex)
{
    if (!m_currentClip || !hasValidTargetObject())
        return;

    auto frameIt = m_currentClip->getAnimationClip().Frames.find(frameIndex);
    if (frameIt == m_currentClip->getAnimationClip().Frames.end())
        return;

    auto scene = SceneManager::GetInstance().GetCurrentScene();
    if (!scene)
        return;

    auto targetObject = scene->FindGameObjectByGuid(m_targetObjectGuid);
    if (!targetObject.IsValid())
        return;

    const AnimFrame& frame = frameIt->second;
    auto& registry = ComponentRegistry::GetInstance();


    for (const auto& [componentName, componentData] : frame.animationData)
    {
        const auto* componentInfo = registry.Get(componentName);
        if (componentInfo && componentInfo->deserialize)
        {
            try
            {
                componentInfo->deserialize(scene->GetRegistry(), targetObject.GetEntityHandle(), componentData);

                EventBus::GetInstance().Publish(ComponentUpdatedEvent{
                    scene->GetRegistry(), targetObject.GetEntityHandle()
                });
            }
            catch (const std::exception& e)
            {
                LogError("应用组件数据失败 {}: {}", componentName, e.what());
            }
        }
    }
}

void AnimationEditorPanel::updateTargetObject()
{
    if (m_targetObjectGuid.Valid())
    {
        auto scene = SceneManager::GetInstance().GetCurrentScene();
        if (scene)
        {
            auto targetObject = scene->FindGameObjectByGuid(m_targetObjectGuid);
            if (targetObject.IsValid())
            {
                m_targetObjectName = targetObject.GetName();
            }
            else
            {
                m_targetObjectName = "无效物体";
            }
        }
        else
        {
            m_targetObjectName = "没有场景";
        }
    }
    else
    {
        m_targetObjectName.clear();
    }
}

bool AnimationEditorPanel::hasValidTargetObject() const
{
    if (!m_targetObjectGuid.Valid())
        return false;

    auto scene = SceneManager::GetInstance().GetCurrentScene();
    if (!scene)
        return false;

    auto targetObject = scene->FindGameObjectByGuid(m_targetObjectGuid);
    return targetObject.IsValid();
}

void AnimationEditorPanel::addKeyFrameFromCurrentObject(int frameIndex)
{
    if (!m_currentClip || !hasValidTargetObject())
    {
        LogWarn("没有打开的动画切片或没有有效的目标物体，无法添加关键帧");
        return;
    }


    auto scene = SceneManager::GetInstance().GetCurrentScene();
    if (!scene)
        return;

    auto targetObject = scene->FindGameObjectByGuid(m_targetObjectGuid);
    if (!targetObject.IsValid())
    {
        LogError("找不到目标物体: {}", m_targetObjectGuid.ToString());
        return;
    }


    std::vector<std::string> availableComponents;
    const auto& registry = ComponentRegistry::GetInstance();
    auto entityHandle = targetObject.GetEntityHandle();
    auto& sceneRegistry = scene->GetRegistry();

    for (const auto& componentName : registry.GetAllRegisteredNames())
    {
        const auto* componentInfo = registry.Get(componentName);
        if (componentInfo && componentInfo->has(sceneRegistry, entityHandle))
        {
            availableComponents.push_back(componentName);
        }
    }

    if (availableComponents.empty())
    {
        LogWarn("目标物体没有任何组件可以记录");
        return;
    }


    m_componentSelectorOpen = true;
    m_selectedComponents.clear();
    m_availableComponents = availableComponents;
    m_pendingFrameIndex = frameIndex;


    for (const auto& componentName : m_availableComponents)
    {
        if (componentName == "Transform")
        {
            m_selectedComponents.insert(componentName);
        }
    }
}

void AnimationEditorPanel::drawComponentSelector()
{
    if (!ImGui::Begin("选择要记录的组件", &m_componentSelectorOpen, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::End();
        return;
    }

    ImGui::Text("选择要在关键帧中记录的组件:");
    ImGui::Separator();


    if (ImGui::Button("全选"))
    {
        m_selectedComponents.clear();
        for (const auto& componentName : m_availableComponents)
        {
            m_selectedComponents.insert(componentName);
        }
    }

    ImGui::SameLine();
    if (ImGui::Button("全不选"))
    {
        m_selectedComponents.clear();
    }

    ImGui::SameLine();
    if (ImGui::Button("仅Transform"))
    {
        m_selectedComponents.clear();
        for (const auto& componentName : m_availableComponents)
        {
            if (componentName == "Transform")
            {
                m_selectedComponents.insert(componentName);
                break;
            }
        }
    }

    ImGui::Separator();


    for (const auto& componentName : m_availableComponents)
    {
        bool isSelected = m_selectedComponents.find(componentName) != m_selectedComponents.end();
        if (ImGui::Checkbox(componentName.c_str(), &isSelected))
        {
            if (isSelected)
            {
                m_selectedComponents.insert(componentName);
            }
            else
            {
                m_selectedComponents.erase(componentName);
            }
        }
    }

    ImGui::Separator();


    if (ImGui::Button("确认添加关键帧"))
    {
        createKeyFrameWithSelectedComponents();
        m_componentSelectorOpen = false;
    }

    ImGui::SameLine();
    if (ImGui::Button("取消"))
    {
        m_componentSelectorOpen = false;
    }

    ImGui::End();
}

void AnimationEditorPanel::createKeyFrameWithSelectedComponents()
{
    if (m_selectedComponents.empty())
    {
        LogWarn("没有选择任何组件，无法创建关键帧");
        return;
    }
    bool frameExists = m_currentClip->getAnimationClip().Frames.find(m_pendingFrameIndex) != m_currentClip->
        getAnimationClip().Frames.end();
    AnimFrame& frame = m_currentClip->getAnimationClip().Frames[m_pendingFrameIndex];

    if (frameExists)
    {
        for (const auto& componentName : m_selectedComponents)
        {
            frame.animationData.erase(componentName);
        }
    }

    auto scene = SceneManager::GetInstance().GetCurrentScene();
    if (scene)
    {
        auto targetObject = scene->FindGameObjectByGuid(m_targetObjectGuid);
        if (targetObject.IsValid())
        {
            const auto& registry = ComponentRegistry::GetInstance();
            auto entityHandle = targetObject.GetEntityHandle();
            auto& sceneRegistry = scene->GetRegistry();

            for (const auto& componentName : m_selectedComponents)
            {
                const auto* componentInfo = registry.Get(componentName);
                if (componentInfo && componentInfo->has(sceneRegistry, entityHandle))
                {
                    YAML::Node componentNode = componentInfo->serialize(sceneRegistry, entityHandle);
                    frame.animationData[componentName] = componentNode;
                }
            }
            m_multiSelectedFrames.clear();
            m_multiSelectedFrames.insert(m_pendingFrameIndex);
            LogInfo("添加关键帧: {}，包含 {} 个选中的组件", m_pendingFrameIndex, m_selectedComponents.size());
        }
    }
}


void AnimationEditorPanel::drawFrameEditor()
{
    if (!ImGui::Begin("帧编辑器", &m_frameEditWindowOpen))
    {
        ImGui::End();
        return;
    }

    if (m_editingFrameIndex < 0 || !m_currentClip)
    {
        ImGui::Text("无效的编辑帧");
        ImGui::End();
        return;
    }

    auto it = m_currentClip->getAnimationClip().Frames.find(m_editingFrameIndex);
    if (it == m_currentClip->getAnimationClip().Frames.end())
    {
        ImGui::Text("帧数据不存在");
        ImGui::End();
        return;
    }

    ImGui::Text("编辑帧 %d", m_editingFrameIndex);
    ImGui::Separator();

    AnimFrame& frame = it->second;


    if (ImGui::CollapsingHeader("组件数据", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::Text("此帧记录了 %zu 个组件的数据:", frame.animationData.size());
        ImGui::Separator();


        for (auto animDataIt = frame.animationData.begin(); animDataIt != frame.animationData.end();)
        {
            auto& componentName = animDataIt->first;
            auto& componentData = animDataIt->second;

            if (ImGui::TreeNode(componentName.c_str()))
            {
                ImGui::Text("组件类型: %s", componentName.c_str());


                if (componentData.IsMap())
                {
                    ImGui::Text("记录的属性数量: %zu", componentData.size());
                }
                else if (componentData.IsScalar())
                {
                    ImGui::Text("数据: %s", componentData.as<std::string>().c_str());
                }

                if (ImGui::Button("删除组件数据"))
                {
                    animDataIt = frame.animationData.erase(animDataIt);
                    ImGui::TreePop();
                    continue;
                }

                ImGui::TreePop();
            }
            ++animDataIt;
        }

        ImGui::Separator();

        if (ImGui::Button("添加更多组件"))
        {
            if (hasValidTargetObject())
            {
                auto scene = SceneManager::GetInstance().GetCurrentScene();
                if (scene)
                {
                    auto targetObject = scene->FindGameObjectByGuid(m_targetObjectGuid);
                    if (targetObject.IsValid())
                    {
                        std::vector<std::string> unrecordedComponents;
                        const auto& registry = ComponentRegistry::GetInstance();
                        auto entityHandle = targetObject.GetEntityHandle();
                        auto& sceneRegistry = scene->GetRegistry();

                        for (const auto& componentName : registry.GetAllRegisteredNames())
                        {
                            const auto* componentInfo = registry.Get(componentName);
                            if (componentInfo && componentInfo->has(sceneRegistry, entityHandle))
                            {
                                if (frame.animationData.find(componentName) == frame.animationData.end())
                                {
                                    unrecordedComponents.push_back(componentName);
                                }
                            }
                        }

                        if (!unrecordedComponents.empty())
                        {
                            m_componentSelectorOpen = true;
                            m_availableComponents = unrecordedComponents;
                            m_selectedComponents.clear();
                            m_pendingFrameIndex = m_editingFrameIndex;
                            m_isAddingToExistingFrame = true;
                        }
                        else
                        {
                            LogInfo("所有组件都已记录");
                        }
                    }
                }
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("从物体刷新已有组件"))
        {
            if (hasValidTargetObject())
            {
                auto scene = SceneManager::GetInstance().GetCurrentScene();
                if (scene)
                {
                    auto targetObject = scene->FindGameObjectByGuid(m_targetObjectGuid);
                    if (targetObject.IsValid())
                    {
                        const auto& registry = ComponentRegistry::GetInstance();
                        auto entityHandle = targetObject.GetEntityHandle();
                        auto& sceneRegistry = scene->GetRegistry();

                        int refreshedCount = 0;
                        for (auto& [componentName, componentData] : frame.animationData)
                        {
                            const auto* componentInfo = registry.Get(componentName);
                            if (componentInfo && componentInfo->has(sceneRegistry, entityHandle))
                            {
                                componentData = componentInfo->serialize(sceneRegistry, entityHandle);
                                refreshedCount++;
                            }
                        }
                        LogInfo("刷新了 {} 个组件的数据", refreshedCount);
                    }
                }
            }
        }
    }


    if (ImGui::CollapsingHeader("动画事件", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::Text("在此帧触发的事件:");
        ImGui::Separator();

        std::vector<int> indicesToRemove;
        for (size_t i = 0; i < frame.eventTargets.size(); ++i)
        {
            ImGui::PushID(static_cast<int>(i));

            std::string eventLabel = std::format("事件 {} [{}]", i,
                                                 frame.eventTargets[i].targetMethodName.empty()
                                                     ? "未设置"
                                                     : frame.eventTargets[i].targetMethodName);
            if (ImGui::TreeNode(eventLabel.c_str()))
            {
                ECS::SerializableEventTarget& target = frame.eventTargets[i];
                if (ImGui::Button("详细编辑"))
                {
                    m_editingEventIndex = static_cast<int>(i);
                    m_eventEditorOpen = true;
                }

                if (CustomDrawing::WidgetDrawer<Guid>::Draw("目标实体", target.targetEntityGuid, *m_context->uiCallbacks))
                {
                    target.targetComponentName = "ScriptComponent";
                    target.targetMethodName.clear();
                    m_context->uiCallbacks->onValueChanged.Invoke();
                }

                ImGui::Text("组件名称: ScriptComponent");
                target.targetComponentName = "ScriptComponent";

                ImGui::Text("方法名称:");
                ImGui::SameLine();
                auto availableMethods = CustomDrawing::ScriptMetadataHelper::GetAvailableMethods(
                    target.targetEntityGuid, "");
                std::string currentMethodDisplay = target.targetMethodName;
                if (!target.targetMethodName.empty())
                {
                    for (const auto& [methodName, signature] : availableMethods)
                    {
                        if (methodName == target.targetMethodName)
                        {
                            currentMethodDisplay = std::format("{}({})", methodName,
                                                               signature == "void" ? "" : signature);
                            break;
                        }
                    }
                }

                ImGui::SetNextItemWidth(200.0f);
                if (ImGui::BeginCombo("##MethodSelector",
                                      target.targetMethodName.empty() ? "选择方法" : currentMethodDisplay.c_str()))
                {
                    for (const auto& [methodName, signature] : availableMethods)
                    {
                        bool isSelected = (target.targetMethodName == methodName);
                        std::string methodDisplay = std::format("{}({})", methodName,
                                                                signature == "void" ? "" : signature);
                        if (ImGui::Selectable(methodDisplay.c_str(), isSelected))
                        {
                            target.targetMethodName = methodName;
                            m_context->uiCallbacks->onValueChanged.Invoke();
                        }
                        if (isSelected) ImGui::SetItemDefaultFocus();
                    }
                    ImGui::EndCombo();
                }

                if (ImGui::Button("删除事件"))
                {
                    indicesToRemove.push_back(static_cast<int>(i));
                    m_context->uiCallbacks->onValueChanged.Invoke();
                }

                ImGui::TreePop();
            }
            ImGui::PopID();
        }

        for (int i = static_cast<int>(indicesToRemove.size()) - 1; i >= 0; --i)
        {
            frame.eventTargets.erase(frame.eventTargets.begin() + indicesToRemove[i]);
        }

        if (ImGui::Button("添加动画事件"))
        {
            addEventTarget(frame);
        }
    }

    ImGui::Separator();

    if (ImGui::Button("确认"))
    {
        saveCurrentClip();
        m_frameEditWindowOpen = false;
    }
    ImGui::SameLine();
    if (ImGui::Button("关闭"))
    {
        m_frameEditWindowOpen = false;
    }

    ImGui::End();
}

void AnimationEditorPanel::addEventTarget(AnimFrame& frame)
{
    ECS::SerializableEventTarget newTarget;
    newTarget.targetComponentName = "ScriptComponent";

    if (hasValidTargetObject())
    {
        newTarget.targetEntityGuid = m_targetObjectGuid;
    }
    frame.eventTargets.push_back(newTarget);
    m_context->uiCallbacks->onValueChanged.Invoke();
    LogInfo("添加新的动画事件目标");
}

void AnimationEditorPanel::removeEventTarget(AnimFrame& frame, size_t index)
{
    if (index < frame.eventTargets.size())
    {
        frame.eventTargets.erase(frame.eventTargets.begin() + index);
        m_context->uiCallbacks->onValueChanged.Invoke();
        LogInfo("删除动画事件目标");
    }
}

void AnimationEditorPanel::drawEventEditor()
{
    if (!ImGui::Begin("动画事件编辑器", &m_eventEditorOpen))
    {
        ImGui::End();
        return;
    }

    if (!m_currentClip || m_editingEventIndex < 0)
    {
        ImGui::Text("没有选中的事件进行编辑");
        ImGui::End();
        return;
    }


    auto frameIt = m_currentClip->getAnimationClip().Frames.find(m_editingFrameIndex);
    if (frameIt == m_currentClip->getAnimationClip().Frames.end())
    {
        ImGui::Text("无效的帧数据");
        ImGui::End();
        return;
    }

    AnimFrame& frame = frameIt->second;

    if (m_editingEventIndex >= static_cast<int>(frame.eventTargets.size()))
    {
        ImGui::Text("无效的事件索引");
        ImGui::End();
        return;
    }

    ECS::SerializableEventTarget& target = frame.eventTargets[m_editingEventIndex];

    ImGui::Text("编辑帧 %d 的事件 %d", m_editingFrameIndex, m_editingEventIndex);
    ImGui::Separator();


    bool changed = false;


    if (CustomDrawing::WidgetDrawer<Guid>::Draw("目标实体", target.targetEntityGuid, *m_context->uiCallbacks))
    {
        changed = true;

        target.targetComponentName = "ScriptComponent";
        target.targetMethodName.clear();
    }


    ImGui::Text("组件名称:");
    ImGui::SameLine();
    ImGui::TextColored(ImVec4(0.8f, 0.8f, 1.0f, 1.0f), "ScriptComponent");
    target.targetComponentName = "ScriptComponent";


    ImGui::Text("方法名称:");
    ImGui::SameLine();


    auto availableMethods = CustomDrawing::ScriptMetadataHelper::GetAvailableMethods(
        target.targetEntityGuid, "");


    std::string currentMethodDisplay = target.targetMethodName;
    if (!target.targetMethodName.empty())
    {
        for (const auto& [methodName, signature] : availableMethods)
        {
            if (methodName == target.targetMethodName)
            {
                currentMethodDisplay = std::format("{}({})", methodName,
                                                   signature == "void" ? "" : signature);
                break;
            }
        }
    }

    ImGui::SetNextItemWidth(200.0f);
    if (ImGui::BeginCombo("##MethodSelector",
                          target.targetMethodName.empty() ? "选择方法" : currentMethodDisplay.c_str()))
    {
        if (availableMethods.empty())
        {
            ImGui::TextDisabled("无可用方法");
        }
        else
        {
            for (const auto& [methodName, signature] : availableMethods)
            {
                bool isSelected = (target.targetMethodName == methodName);
                std::string methodDisplay = std::format("{}({})", methodName,
                                                        signature == "void" ? "" : signature);

                if (ImGui::Selectable(methodDisplay.c_str(), isSelected))
                {
                    target.targetMethodName = methodName;
                    changed = true;
                }

                if (isSelected)
                {
                    ImGui::SetItemDefaultFocus();
                }


                if (ImGui::IsItemHovered())
                {
                    ImGui::BeginTooltip();
                    ImGui::Text("方法名: %s", methodName.c_str());
                    ImGui::Text("参数: %s", signature == "void" ? "无" : signature.c_str());
                    ImGui::EndTooltip();
                }
            }
        }

        ImGui::EndCombo();
    }


    RuntimeGameObject targetObject = CustomDrawing::ScriptMetadataHelper::GetGameObjectByGuid(
        target.targetEntityGuid);
    if (targetObject.IsValid())
    {
        ImGui::Text("目标对象: %s", targetObject.GetName().c_str());


        ImGui::Separator();
        ImGui::Text("对象详情:");
        ImGui::Text("  GUID: %s", target.targetEntityGuid.ToString().c_str());


        if (targetObject.HasComponent<ECS::ScriptComponent>())
        {
            auto& scriptComp = targetObject.GetComponent<ECS::ScriptComponent>();
            if (scriptComp.metadata)
            {
                ImGui::Text("  脚本类: %s", scriptComp.metadata->name.c_str());
                ImGui::Text("  可用方法数: %zu", scriptComp.metadata->publicMethods.size());
            }
            else
            {
                ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.6f, 1.0f), "  脚本组件无元数据");
            }
        }
        else
        {
            ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.6f, 1.0f), "  对象没有脚本组件");
        }
    }
    else if (target.targetEntityGuid.Valid())
    {
        ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.6f, 1.0f), "目标对象无效或不存在");
        ImGui::Text("GUID: %s", target.targetEntityGuid.ToString().c_str());
    }
    else
    {
        ImGui::TextDisabled("请选择目标实体");
    }

    ImGui::Separator();


    if (ImGui::CollapsingHeader("事件预览"))
    {
        if (!target.targetEntityGuid.Valid())
        {
            ImGui::TextDisabled("请先选择目标实体");
        }
        else if (target.targetMethodName.empty())
        {
            ImGui::TextDisabled("请先选择目标方法");
        }
        else
        {
            ImGui::Text("事件调用:");
            ImGui::Text("  实体: %s", targetObject.IsValid() ? targetObject.GetName().c_str() : "无效对象");
            ImGui::Text("  组件: %s", target.targetComponentName.c_str());
            ImGui::Text("  方法: %s", target.targetMethodName.c_str());
            ImGui::Text("  触发帧: %d", m_editingFrameIndex);
        }
    }

    ImGui::Separator();


    if (ImGui::Button("保存"))
    {
        if (changed)
        {
            m_context->uiCallbacks->onValueChanged.Invoke();
        }
        m_eventEditorOpen = false;
    }

    ImGui::SameLine();
    if (ImGui::Button("取消"))
    {
        m_eventEditorOpen = false;
    }

    ImGui::SameLine();
    if (ImGui::Button("删除此事件"))
    {
        frame.eventTargets.erase(frame.eventTargets.begin() + m_editingEventIndex);
        m_context->uiCallbacks->onValueChanged.Invoke();
        m_eventEditorOpen = false;
        LogInfo("删除动画事件目标");
    }

    ImGui::End();
}

void AnimationEditorPanel::Draw()
{
    if (!m_isVisible) return;
    if (m_requestFocus)
    {
        ImGui::SetNextWindowFocus();
        m_requestFocus = false;
    }

    if (ImGui::Begin(GetPanelName(), &m_isVisible, ImGuiWindowFlags_MenuBar))
    {
        if (ImGui::BeginMenuBar())
        {
            if (ImGui::BeginMenu("文件"))
            {
                if (ImGui::MenuItem("保存", "Ctrl+S", false, m_currentClip != nullptr)) { saveCurrentClip(); }
                if (ImGui::MenuItem("关闭", "Ctrl+W", false, m_currentClip != nullptr)) { CloseCurrentClip(); }
                if (ImGui::MenuItem("新建动画", "Ctrl+N")) { createNewAnimation(); }
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("编辑"))
            {
                bool hasSelection = !m_multiSelectedFrames.empty();
                if (ImGui::MenuItem("添加关键帧", "K", false, hasValidTargetObject()))
                {
                    addKeyFrameFromCurrentObject(m_currentFrame);
                }
                if (ImGui::MenuItem("删除关键帧", "Delete", false, hasSelection))
                {
                    for (int frameIndex : m_multiSelectedFrames) removeKeyFrame(frameIndex);
                    m_multiSelectedFrames.clear();
                }
                if (ImGui::MenuItem("复制帧数据", "Ctrl+C", false, hasSelection))
                {
                    m_copiedFrameIndex = *m_multiSelectedFrames.begin();
                    m_hasFrameDataToCopy = true;
                }
                if (ImGui::MenuItem("粘贴帧数据", "Ctrl+V", false, m_hasFrameDataToCopy && m_currentFrame >= 0))
                {
                    copyFrameData(m_copiedFrameIndex, m_currentFrame);
                }
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("播放"))
            {
                if (ImGui::MenuItem(m_isPlaying ? "暂停" : "播放", "Space")) { m_isPlaying = !m_isPlaying; }
                if (ImGui::MenuItem("停止", "Shift+Space"))
                {
                    m_isPlaying = false;
                    seekToFrame(0);
                }
                if (ImGui::MenuItem("跳到开头", "Home")) { seekToFrame(0); }
                if (ImGui::MenuItem("跳到结尾", "End")) { seekToFrame(m_totalFrames - 1); }
                ImGui::EndMenu();
            }
            ImGui::EndMenuBar();
        }
        drawTargetObjectSelector();
        ImGui::Separator();
        if (!m_currentClip)
        {
            ImGui::Text("当前没有打开的动画切片");
            ImGui::Text("双击资源浏览器中的动画切片文件以开始编辑");
            ImGui::Separator();
            drawControlPanel();
            ImGui::Separator();
            drawTimeline();
        }
        else
        {
            drawControlPanel();
            ImGui::Separator();
            drawTimeline();
            ImGui::Separator();
            drawPropertiesPanel();
        }
    }
    ImGui::End();
    if (m_frameEditWindowOpen) { drawFrameEditor(); }
    if (m_componentSelectorOpen) { drawComponentSelector(); }
    if (m_eventEditorOpen) { drawEventEditor(); }
}

void AnimationEditorPanel::drawPropertiesPanel()
{
    if (!m_currentClip)
    {
        ImGui::Text("没有打开的动画切片");
        return;
    }

    ImGui::Text("属性面板");

    if (m_multiSelectedFrames.size() == 1)
    {
        int selectedFrame = *m_multiSelectedFrames.begin();
        auto it = m_currentClip->getAnimationClip().Frames.find(selectedFrame);
        if (it != m_currentClip->getAnimationClip().Frames.end())
        {
            ImGui::Text("关键帧 %d", selectedFrame);
            if (ImGui::Button("编辑帧数据"))
            {
                m_editingFrameIndex = selectedFrame;
                m_frameEditWindowOpen = true;
            }
            ImGui::SameLine();
            if (ImGui::Button("删除关键帧"))
            {
                removeKeyFrame(selectedFrame);

                return;
            }
            ImGui::SameLine();
            if (ImGui::Button("复制帧"))
            {
                m_copiedFrameIndex = selectedFrame;
                m_hasFrameDataToCopy = true;
            }
            ImGui::SameLine();
            if (ImGui::Button("应用到物体"))
            {
                applyFrameToObject(selectedFrame);
            }

            const AnimFrame& frame = it->second;
            ImGui::Text("记录的组件: %zu 个", frame.animationData.size());
            if (ImGui::BeginChild("ComponentData", ImVec2(0, 150), true))
            {
                for (const auto& [name, data] : frame.animationData)
                {
                    if (ImGui::TreeNodeEx(name.c_str(), ImGuiTreeNodeFlags_Leaf))
                    {
                        ImGui::TreePop();
                    }
                }
                ImGui::EndChild();
            }
            ImGui::Text("事件目标: %zu 个", frame.eventTargets.size());
        }
    }
    else if (m_multiSelectedFrames.size() > 1)
    {
        ImGui::Text("已选择 %zu 个关键帧", m_multiSelectedFrames.size());
        if (ImGui::Button("删除所有选中的关键帧"))
        {
            for (int frameIndex : m_multiSelectedFrames)
            {
                removeKeyFrame(frameIndex);
            }
            m_multiSelectedFrames.clear();
        }
    }
    else
    {
        ImGui::Text("当前帧: %d", m_currentFrame);
        auto it = m_currentClip->getAnimationClip().Frames.find(m_currentFrame);
        if (it != m_currentClip->getAnimationClip().Frames.end())
        {
            ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "这是一个关键帧");
            if (ImGui::Button("编辑此关键帧"))
            {
                m_multiSelectedFrames.insert(m_currentFrame);
            }
        }
        else
        {
            if (hasValidTargetObject())
            {
                if (ImGui::Button("添加关键帧"))
                {
                    addKeyFrameFromCurrentObject(m_currentFrame);
                }
            }
            else
            {
                ImGui::Text("请先设置目标物体");
            }
        }

        if (m_hasFrameDataToCopy)
        {
            if (ImGui::Button("粘贴帧数据"))
            {
                copyFrameData(m_copiedFrameIndex, m_currentFrame);
            }
        }
    }
}
