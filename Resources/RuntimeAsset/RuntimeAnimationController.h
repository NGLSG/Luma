#ifndef RUNTIMEANIMATIONCONTROLLER_H
#define RUNTIMEANIMATIONCONTROLLER_H
#include "AnimationControllerData.h"
#include "IRuntimeAsset.h"
#include "RuntimeAnimationClip.h"
#include "Event/LumaEvent.h"

/**
 * @brief 运行时动画控制器类。
 *
 * 管理和播放动画剪辑，处理动画状态、变量和过渡。
 */
class RuntimeAnimationController : public IRuntimeAsset
{
private:
    AnimationControllerData m_animationControllerData; ///< 动画控制器数据。
    std::unordered_map<std::string, std::variant<float, bool, int>> m_variables; ///< 动画控制器中的变量集合。
    std::unordered_map<std::string, VariableType> m_variableTypes; ///< 动画控制器中变量的类型。
    bool EntryPlayed = false; ///< 标记入口动画是否已播放。
    std::unordered_map<std::string, bool> m_animationPlayingStates; ///< 存储动画的播放状态。
    std::unordered_map<std::string, sk_sp<RuntimeAnimationClip>> m_animationClips; ///< 存储所有运行时动画剪辑。
    std::string m_currentAnimationName; ///< 当前正在播放的动画名称。
    Guid m_currentAnimationGuid; ///< 当前正在播放的动画的全局唯一标识符。

    /**
     * @brief 内部播放动画的实现。
     * @param clip 要播放的动画剪辑。
     * @param speed 动画播放速度。
     * @param transitionDuration 动画过渡持续时间。
     */
    void playInternal(const sk_sp<RuntimeAnimationClip>& clip, float speed = 1.0f, float transitionDuration = 0.0f);
    float m_currentTime = 0.0f; ///< 当前动画播放时间。
    float m_frameRate = 60.f; ///< 动画帧率。
    int m_currentFrameIndex = 0; ///< 当前动画帧索引。
    int m_totalFrames = 0; ///< 当前动画的总帧数。
    float m_animationSpeed = 1.0f; ///< 当前动画播放速度。

    bool m_isTransitioning = false; ///< 标记是否正在进行动画过渡。
    float m_transitionTime = 0.0f; ///< 当前过渡已进行的时间。
    float m_transitionDuration = 0.0f; ///< 过渡的总持续时间。
    sk_sp<RuntimeAnimationClip> m_fromClip; ///< 过渡的起始动画剪辑。
    sk_sp<RuntimeAnimationClip> m_toClip; ///< 过渡的目标动画剪辑。
    int m_fromFrameIndex = 0; ///< 过渡起始动画的帧索引。

    LumaEvent<float, int> m_onAnimationUpdateEvent; ///< 动画更新事件。
    ListenerHandle m_onAnimationUpdateListener; ///< 动画更新事件的监听器句柄。
    bool ForceStop = false; ///< 标记是否强制停止动画。
    bool m_isPlaying = false; ///< 标记动画是否正在播放。
    bool m_justTransitioned = false; ///< 标记是否刚刚完成过渡。

    /**
     * @brief 评估一组条件是否满足。
     * @param conditions 要评估的条件列表。
     * @return 如果所有条件都满足则返回 true，否则返回 false。
     */
    bool EvaluateCondition(const std::vector<Condition>& conditions);
    /**
     * @brief 更新基于帧的动画。
     * @param deltaTime 帧之间的时间差。
     */
    void UpdateFrameBasedAnimation(float deltaTime);
    /**
     * @brief 更新动画过渡状态。
     * @param deltaTime 帧之间的时间差。
     */
    void UpdateTransition(float deltaTime);
    /**
     * @brief 应用动画剪辑的某一帧到模型。
     * @param clip 要应用的动画剪辑。
     * @param frameIndex 要应用的帧索引。
     * @param blendWeight 混合权重。
     */
    void ApplyAnimationFrame(const sk_sp<RuntimeAnimationClip>& clip, int frameIndex, float blendWeight = 1.0f);
    /**
     * @brief 混合两个动画剪辑的帧。
     * @param fromClip 源动画剪辑。
     * @param fromFrame 源动画的帧索引。
     * @param toClip 目标动画剪辑。
     * @param toFrame 目标动画的帧索引。
     * @param blendFactor 混合因子。
     */
    void BlendAnimationFrames(const sk_sp<RuntimeAnimationClip>& fromClip, int fromFrame,
                              const sk_sp<RuntimeAnimationClip>& toClip, int toFrame, float blendFactor);

public:
    /**
     * @brief 获取动画控制器数据。
     * @return 动画控制器数据。
     */
    AnimationControllerData GetAnimationControllerData() const { return m_animationControllerData; }
    /**
     * @brief 构造函数，使用动画控制器数据初始化。
     * @param data 动画控制器数据。
     */
    RuntimeAnimationController(AnimationControllerData data);
    /**
     * @brief 播放指定名称的动画。
     * @param animationName 要播放的动画名称。
     * @param speed 动画播放速度。
     * @param transitionDuration 动画过渡持续时间。
     */
    void PlayAnimation(const std::string& animationName, float speed = 1.0f, float transitionDuration = 0.0f);
    /**
     * @brief 播放指定GUID的动画。
     * @param guid 要播放的动画的GUID。
     * @param speed 动画播放速度。
     * @param transitionDuration 动画过渡持续时间。
     */
    void PlayAnimation(const Guid& guid, float speed = 1.0f, float transitionDuration = 0.0f);
    /**
     * @brief 停止当前正在播放的动画。
     */
    void StopAnimation();
    /**
     * @brief 获取当前正在播放的动画名称。
     * @return 当前动画的名称。
     */
    std::string GetCurrentAnimationName() const;
    /**
     * @brief 检查指定名称的动画是否正在播放。
     * @param animationName 要检查的动画名称。
     * @return 如果动画正在播放则返回 true，否则返回 false。
     */
    bool IsAnimationPlaying(const std::string& animationName);

    /**
     * @brief 设置动画控制器中的变量值。
     * @param name 变量名称。
     * @param value 变量值（可以是浮点数、布尔值或整数）。
     */
    void SetVariable(const std::string& name, std::variant<float, bool, int> value);
    /**
     * @brief 触发动画控制器中的一个触发器。
     * @param name 触发器名称。
     */
    void SetTrigger(const std::string& name);
    /**
     * @brief 设置动画的帧率。
     * @param frameRate 要设置的帧率。
     */
    void SetFrameRate(float frameRate);
    /**
     * @brief 获取动画的帧率。
     * @return 当前动画的帧率。
     */
    float GetFrameRate() const;
    /**
     * @brief 播放入口动画。
     */
    void PlayEntryAnimation();
    /**
     * @brief 查找最佳的动画过渡。
     * @param animationHasFinished 标记当前动画是否已播放完毕。
     * @return 指向最佳过渡的指针，如果没有找到则为 nullptr。
     */
    const Transition* FindBestTransition(bool animationHasFinished);

    /**
     * @brief 更新动画控制器的状态。
     * @param deltaTime 帧之间的时间差。
     */
    void Update(float deltaTime);
};

#endif