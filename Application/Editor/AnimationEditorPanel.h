#ifndef ANIMATIONEDITORPANEL_H
#define ANIMATIONEDITORPANEL_H
#include "IEditorPanel.h"
#include <unordered_map>
#include <string>
#include "Loaders/TextureLoader.h"
#include "RuntimeAsset/RuntimeAnimationClip.h"
class AnimationEditorPanel : public IEditorPanel
{
private:
    sk_sp<RuntimeAnimationClip> m_currentClip; 
    Guid m_currentClipGuid; 
    std::string m_currentClipName; 
    Guid m_targetObjectGuid; 
    std::string m_targetObjectName; 
    float m_currentTime = 0.0f; 
    float m_frameRate = 60.0f; 
    int m_currentFrame = 0; 
    int m_totalFrames = 60; 
    bool m_isPlaying = false; 
    bool m_isLooping = true; 
    float m_timelineHeight = 200.0f; 
    float m_timelineZoom = 1.0f; 
    float m_timelineScrollX = 0.0f; 
    int m_selectedFrameIndex = -1; 
    bool m_frameEditWindowOpen = false; 
    int m_editingFrameIndex = -1; 
    int m_copiedFrameIndex = -1; 
    bool m_hasFrameDataToCopy = false; 
    bool m_componentSelectorOpen = false; 
    std::vector<std::string> m_availableComponents; 
    std::set<std::string> m_selectedComponents; 
    int m_pendingFrameIndex = -1; 
    bool m_isAddingToExistingFrame = false; 
    bool m_eventEditorOpen = false; 
    int m_editingEventIndex = -1; 
    std::string m_newEventName; 
    std::vector<std::string> m_availableEventTypes; 
    void openAnimationClipFromContext(const Guid& clipGuid);
    void closeCurrentClipFromContext();
    void createNewAnimation();
    void drawTargetObjectSelector();
    void drawTimeline();
    void drawFrameEditor();
    void drawPropertiesPanel();
    void drawControlPanel();
    void updatePlayback(float deltaTime);
    void seekToFrame(int frameIndex);
    void addKeyFrame(int frameIndex);
    void addKeyFrameFromCurrentObject(int frameIndex);
    void removeKeyFrame(int frameIndex);
    void copyFrameData(int fromFrame, int toFrame);
    void saveCurrentClip();
    void centerTimelineOnCurrentFrame();
    void fitTimelineToAllFrames(float viewWidth);
    void applyFrameToObject(int frameIndex);
    void updateTargetObject();
    bool hasValidTargetObject() const;
    void drawComponentSelector();
    void createKeyFrameWithSelectedComponents();
    void drawEventEditor();
    void addEventTarget(AnimFrame& frame);
    void removeEventTarget(AnimFrame& frame, size_t index);
    void handleShortcutInput();
public:
    AnimationEditorPanel() = default;
    ~AnimationEditorPanel() override = default;
    void Initialize(EditorContext* context) override;
    void Update(float deltaTime) override;
    void Draw() override;
    void Shutdown() override;
    const char* GetPanelName() const override { return "动画编辑器"; }
    void OpenAnimationClip(const Guid& clipGuid);
    void CloseCurrentClip();
    bool HasActiveClip() const { return m_currentClip != nullptr; }
    void Focus() override;
    std::set<int> m_multiSelectedFrames; 
    bool m_isDraggingPlayhead = false; 
    bool m_isDraggingKeyframe = false; 
    int m_dragAnchorFrame = -1; 
    int m_dragFrameDelta = 0; 
    bool m_isBoxSelecting = false; 
    ImVec2 m_boxSelectionStart; 
    std::vector<int> m_dragInitialSelectionState; 
    int m_dragHandleFrame = -1; 
    std::unique_ptr<TextureLoader> m_textureLoader; 
    bool m_requestFocus = false; 
};
#endif
