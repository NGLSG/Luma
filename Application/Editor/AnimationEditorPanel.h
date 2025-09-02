#ifndef ANIMATIONEDITORPANEL_H
#define ANIMATIONEDITORPANEL_H

#include "IEditorPanel.h"

#include <unordered_map>
#include <string>

#include "Loaders/TextureLoader.h"
#include "RuntimeAsset/RuntimeAnimationClip.h"

/**
 * @brief 动画编辑器面板类。
 *
 * 负责在编辑器中显示和编辑动画剪辑。
 */
class AnimationEditorPanel : public IEditorPanel
{
private:
    sk_sp<RuntimeAnimationClip> m_currentClip; ///< 当前正在编辑的动画剪辑。
    Guid m_currentClipGuid; ///< 当前动画剪辑的全局唯一标识符。
    std::string m_currentClipName; ///< 当前动画剪辑的名称。

    Guid m_targetObjectGuid; ///< 目标对象的全局唯一标识符，动画将应用于此对象。
    std::string m_targetObjectName; ///< 目标对象的名称。

    float m_currentTime = 0.0f; ///< 当前播放时间。
    float m_frameRate = 60.0f; ///< 动画的帧率。
    int m_currentFrame = 0; ///< 当前播放的帧索引。
    int m_totalFrames = 60; ///< 动画的总帧数。
    bool m_isPlaying = false; ///< 动画是否正在播放。
    bool m_isLooping = true; ///< 动画是否循环播放。

    float m_timelineHeight = 200.0f; ///< 时间轴的高度。
    float m_timelineZoom = 1.0f; ///< 时间轴的缩放级别。
    float m_timelineScrollX = 0.0f; ///< 时间轴的水平滚动位置。
    int m_selectedFrameIndex = -1; ///< 当前选中的关键帧索引。

    bool m_frameEditWindowOpen = false; ///< 帧编辑窗口是否打开。
    int m_editingFrameIndex = -1; ///< 正在编辑的帧索引。

    int m_copiedFrameIndex = -1; ///< 复制的帧索引。
    bool m_hasFrameDataToCopy = false; ///< 是否有可复制的帧数据。

    bool m_componentSelectorOpen = false; ///< 组件选择器是否打开。
    std::vector<std::string> m_availableComponents; ///< 可用的组件列表。
    std::set<std::string> m_selectedComponents; ///< 已选择的组件列表。
    int m_pendingFrameIndex = -1; ///< 待处理的帧索引。
    bool m_isAddingToExistingFrame = false; ///< 是否正在向现有帧添加数据。

    bool m_eventEditorOpen = false; ///< 事件编辑器是否打开。
    int m_editingEventIndex = -1; ///< 正在编辑的事件索引。
    std::string m_newEventName; ///< 新事件的名称。
    std::vector<std::string> m_availableEventTypes; ///< 可用的事件类型列表。

    /**
     * @brief 从上下文打开一个动画剪辑。
     * @param clipGuid 动画剪辑的全局唯一标识符。
     */
    void openAnimationClipFromContext(const Guid& clipGuid);
    /**
     * @brief 从上下文关闭当前动画剪辑。
     */
    void closeCurrentClipFromContext();
    /**
     * @brief 创建一个新的动画。
     */
    void createNewAnimation();
    /**
     * @brief 绘制目标对象选择器。
     */
    void drawTargetObjectSelector();
    /**
     * @brief 绘制时间轴。
     */
    void drawTimeline();
    /**
     * @brief 绘制帧编辑器。
     */
    void drawFrameEditor();
    /**
     * @brief 绘制属性面板。
     */
    void drawPropertiesPanel();
    /**
     * @brief 绘制控制面板。
     */
    void drawControlPanel();
    /**
     * @brief 更新动画播放状态。
     * @param deltaTime 距离上一帧的时间间隔。
     */
    void updatePlayback(float deltaTime);
    /**
     * @brief 跳转到指定帧。
     * @param frameIndex 要跳转到的帧索引。
     */
    void seekToFrame(int frameIndex);
    /**
     * @brief 添加一个关键帧。
     * @param frameIndex 要添加关键帧的索引。
     */
    void addKeyFrame(int frameIndex);
    /**
     * @brief 从当前目标对象添加一个关键帧。
     * @param frameIndex 要添加关键帧的索引。
     */
    void addKeyFrameFromCurrentObject(int frameIndex);
    /**
     * @brief 移除一个关键帧。
     * @param frameIndex 要移除关键帧的索引。
     */
    void removeKeyFrame(int frameIndex);
    /**
     * @brief 复制帧数据。
     * @param fromFrame 源帧索引。
     * @param toFrame 目标帧索引。
     */
    void copyFrameData(int fromFrame, int toFrame);
    /**
     * @brief 保存当前动画剪辑。
     */
    void saveCurrentClip();
    /**
     * @brief 将时间轴居中显示当前帧。
     */
    void centerTimelineOnCurrentFrame();
    /**
     * @brief 根据视图宽度调整时间轴以显示所有帧。
     * @param viewWidth 视图的宽度。
     */
    void fitTimelineToAllFrames(float viewWidth);
    /**
     * @brief 将指定帧的数据应用到目标对象。
     * @param frameIndex 要应用的帧索引。
     */
    void applyFrameToObject(int frameIndex);
    /**
     * @brief 更新目标对象。
     */
    void updateTargetObject();
    /**
     * @brief 检查是否存在有效的目标对象。
     * @return 如果存在有效的目标对象，则返回 true；否则返回 false。
     */
    bool hasValidTargetObject() const;
    /**
     * @brief 绘制组件选择器。
     */
    void drawComponentSelector();
    /**
     * @brief 使用选定的组件创建关键帧。
     */
    void createKeyFrameWithSelectedComponents();
    /**
     * @brief 绘制事件编辑器。
     */
    void drawEventEditor();
    /**
     * @brief 为动画帧添加一个事件目标。
     * @param frame 动画帧引用。
     */
    void addEventTarget(AnimFrame& frame);
    /**
     * @brief 从动画帧中移除一个事件目标。
     * @param frame 动画帧引用。
     * @param index 要移除的事件目标的索引。
     */
    void removeEventTarget(AnimFrame& frame, size_t index);

public:
    /**
     * @brief 构造函数。
     */
    AnimationEditorPanel() = default;
    /**
     * @brief 析构函数。
     */
    ~AnimationEditorPanel() override = default;

    /**
     * @brief 初始化编辑器面板。
     * @param context 编辑器上下文指针。
     */
    void Initialize(EditorContext* context) override;
    /**
     * @brief 更新编辑器面板。
     * @param deltaTime 距离上一帧的时间间隔。
     */
    void Update(float deltaTime) override;
    /**
     * @brief 绘制编辑器面板。
     */
    void Draw() override;
    /**
     * @brief 关闭编辑器面板。
     */
    void Shutdown() override;
    /**
     * @brief 获取面板名称。
     * @return 面板的名称字符串。
     */
    const char* GetPanelName() const override { return "动画编辑器"; }

    /**
     * @brief 打开一个动画剪辑。
     * @param clipGuid 动画剪辑的全局唯一标识符。
     */
    void OpenAnimationClip(const Guid& clipGuid);

    /**
     * @brief 关闭当前动画剪辑。
     */
    void CloseCurrentClip();

    /**
     * @brief 检查是否有活动的动画剪辑。
     * @return 如果有活动的动画剪辑，则返回 true；否则返回 false。
     */
    bool HasActiveClip() const { return m_currentClip != nullptr; }
    /**
     * @brief 使面板获得焦点。
     */
    void Focus() override;

    std::set<int> m_multiSelectedFrames; ///< 多选的关键帧索引集合。
    bool m_isDraggingPlayhead = false; ///< 播放头是否正在被拖动。
    bool m_isDraggingKeyframe = false; ///< 关键帧是否正在被拖动。
    int m_dragAnchorFrame = -1; ///< 拖动操作的起始帧。
    int m_dragFrameDelta = 0; ///< 拖动帧的偏移量。

    bool m_isBoxSelecting = false; ///< 是否正在进行框选。
    ImVec2 m_boxSelectionStart; ///< 框选的起始位置。
    std::vector<int> m_dragInitialSelectionState; ///< 拖动操作开始时的初始选择状态。
    int m_dragHandleFrame = -1; ///< 拖动句柄所在的帧。
    std::unique_ptr<TextureLoader> m_textureLoader; ///< 纹理加载器。
    bool m_requestFocus = false; ///< 是否请求面板获得焦点。
};

#endif