#include "RuntimeAnimationController.h"
#include "Logger.h"
#include "SceneManager.h"
#include "Loaders/AnimationClipLoader.h"

int FindLastKeyframeIndex(const AnimationClip& clipData, int currentFrame)
{
    int lastKeyframe = -1;
    for (const auto& [frameIndex, frame] : clipData.Frames)
    {
        if (frameIndex <= currentFrame)
        {
            if (frameIndex > lastKeyframe)
            {
                lastKeyframe = frameIndex;
            }
        }
    }
    return lastKeyframe;
}

void RuntimeAnimationController::playInternal(const sk_sp<RuntimeAnimationClip>& clip, float speed,
                                              float transitionDuration)
{
    if (clip == nullptr)
    {
        LogWarn("尝试播放空动画剪辑");
        return;
    }

    auto& animData = clip->getAnimationClip();
    auto currentScene = SceneManager::GetInstance().GetCurrentScene();
    if (!currentScene) return;

    auto go = currentScene->FindGameObjectByGuid(animData.TargetEntityGuid);
    if (!go.IsValid())
    {
        LogWarn("无法找到目标实体: {}", animData.TargetEntityGuid.ToString());
        return;
    }

    if (transitionDuration > 0.0f && m_isPlaying)
    {
        m_isTransitioning = true;
        m_transitionTime = 0.0f;
        m_transitionDuration = transitionDuration;
        m_fromClip = m_animationClips[m_currentAnimationName];
        m_fromFrameIndex = m_currentFrameIndex;
        m_toClip = clip;
        LogInfo("开始过渡动画，过渡时长: {}秒", transitionDuration);
    }
    else
    {
        m_isTransitioning = false;
    }

    m_currentAnimationGuid = clip->GetSourceGuid();
    m_currentAnimationName = clip->GetName();
    m_currentTime = 0.0f;
    m_currentFrameIndex = 0;
    m_totalFrames = 0;

    if (!animData.Frames.empty())
    {
        for (const auto& [frameIndex, frame] : animData.Frames)
        {
            m_totalFrames = std::max(m_totalFrames, frameIndex + 1);
        }
    }

    m_animationSpeed = speed;
    m_isPlaying = true;


    m_justTransitioned = true;
}

bool RuntimeAnimationController::EvaluateCondition(const std::vector<Condition>& conditions)
{
    if (conditions.empty())
    {
        return true;
    }

    for (const auto& condition : conditions)
    {
        bool result = false;


        std::visit([&](auto&& arg)
        {
            using T = std::decay_t<decltype(arg)>;
            if (!m_variables.contains(arg.VarName))
            {
                LogWarn("动画变量 {} 未定义", arg.VarName);
                result = false;
                return;
            }

            if constexpr (std::is_same_v<T, FloatCondition>)
            {
                auto varValue = std::get<float>(m_variables[arg.VarName]);
                result = (arg.op == FloatCondition::GreaterThan) ? (varValue > arg.Value) : (varValue < arg.Value);
            }
            else if constexpr (std::is_same_v<T, IntCondition>)
            {
                auto varValue = std::get<int>(m_variables[arg.VarName]);
                switch (arg.op)
                {
                case IntCondition::GreaterThan: result = varValue > arg.Value;
                    break;
                case IntCondition::LessThan: result = varValue < arg.Value;
                    break;
                case IntCondition::Equal: result = varValue == arg.Value;
                    break;
                case IntCondition::NotEqual: result = varValue != arg.Value;
                    break;
                }
            }
            else if constexpr (std::is_same_v<T, BoolCondition>)
            {
                auto varValue = std::get<bool>(m_variables[arg.VarName]);
                result = (arg.op == BoolCondition::IsTrue) ? varValue : !varValue;
            }

            else if constexpr (std::is_same_v<T, TriggerCondition>)
            {
                result = std::get<bool>(m_variables[arg.VarName]);
            }
        }, condition);

        if (!result)
        {
            return false;
        }
    }
    return true;
}

void RuntimeAnimationController::SetTrigger(const std::string& name)
{
    if (m_variables.contains(name) && m_variableTypes[name] == VariableType::VariableType_Trigger)
    {
        m_variables[name] = true;
    }
    else
    {
        LogWarn("尝试设置一个无效或不存在的触发器: {}", name);
    }
}

void RuntimeAnimationController::UpdateFrameBasedAnimation(float deltaTime)
{
    if (!m_isPlaying || m_totalFrames <= 0 || m_frameRate <= 0) return;

    m_currentTime += deltaTime * m_animationSpeed;
    float frameDuration = 1.0f / m_frameRate;
    float animationDuration = m_totalFrames * frameDuration;


    if (m_currentTime >= animationDuration)
    {
        m_currentTime = fmod(m_currentTime, animationDuration);
    }

    m_currentFrameIndex = static_cast<int>(m_currentTime / frameDuration);


    auto currentClip = m_animationClips[m_currentAnimationName];
    if (currentClip)
    {
        int keyframeToApply = FindLastKeyframeIndex(currentClip->getAnimationClip(), m_currentFrameIndex);
        if (keyframeToApply != -1)
        {
            ApplyAnimationFrame(currentClip, keyframeToApply);
        }
    }
}

void RuntimeAnimationController::UpdateTransition(float deltaTime)
{
    if (!m_isTransitioning)
        return;

    m_transitionTime += deltaTime;
    float blendFactor = m_transitionTime / m_transitionDuration;

    if (blendFactor >= 1.0f)
    {
        blendFactor = 1.0f;
        m_isTransitioning = false;
        LogInfo("动画过渡完成");
    }


    int fromFrame = m_fromFrameIndex;
    int toFrame = m_currentFrameIndex;


    BlendAnimationFrames(m_fromClip, fromFrame, m_toClip, toFrame, blendFactor);
}

void RuntimeAnimationController::ApplyAnimationFrame(const sk_sp<RuntimeAnimationClip>& clip, int frameIndex,
                                                     float blendWeight)
{
    if (!clip)
        return;

    auto animData = clip->getAnimationClip();
    auto currentScene = SceneManager::GetInstance().GetCurrentScene();
    auto go = currentScene->FindGameObjectByGuid(animData.TargetEntityGuid);

    if (!go.IsValid())
    {
        LogWarn("无法找到目标实体: {}", animData.TargetEntityGuid.ToString());
        return;
    }

    if (!animData.Frames.contains(frameIndex))
    {
        return;
    }

    auto frame = animData.Frames[frameIndex];
    for (auto& comp : frame.animationData)
    {
        auto* reg = ComponentRegistry::GetInstance().Get(comp.first);
        if (reg == nullptr)
        {
            LogWarn("组件注册表中未找到组件: {}", comp.first);
            continue;
        }

        if (reg->has(currentScene->GetRegistry(), go.GetEntityHandle()))
        {
            auto component = reg->get_raw_ptr(currentScene->GetRegistry(), go.GetEntityHandle());
            if (component)
            {
                reg->deserialize(currentScene->GetRegistry(), go.GetEntityHandle(), comp.second);
                EventBus::GetInstance().Publish(ComponentUpdatedEvent{
                    currentScene->GetRegistry(), go.GetEntityHandle()
                });

                for (const auto& target : frame.eventTargets)
                {
                    RuntimeGameObject targetGO = currentScene->FindGameObjectByGuid(target.targetEntityGuid);
                    if (targetGO.IsValid() && targetGO.HasComponent<ECS::ScriptComponent>())
                    {
                        InteractScriptEvent scriptEvent;
                        scriptEvent.type = InteractScriptEvent::CommandType::InvokeMethod;
                        scriptEvent.entityId = static_cast<uint32_t>(targetGO.GetEntityHandle());
                        scriptEvent.methodName = target.targetMethodName;

                        EventBus::GetInstance().Publish(scriptEvent);
                    }
                }
            }
            else
            {
                LogWarn("组件 {} 在实体 {} 上未找到", comp.first, go.GetGuid().ToString());
            }
        }
    }
}

void RuntimeAnimationController::BlendAnimationFrames(const sk_sp<RuntimeAnimationClip>& fromClip, int fromFrame,
                                                      const sk_sp<RuntimeAnimationClip>& toClip, int toFrame,
                                                      float blendFactor)
{
    if (blendFactor < 0.5f)
    {
        ApplyAnimationFrame(fromClip, fromFrame, 1.0f - blendFactor);
    }
    else
    {
        ApplyAnimationFrame(toClip, toFrame, blendFactor);
    }
}

RuntimeAnimationController::RuntimeAnimationController(AnimationControllerData data)
    : m_animationControllerData(std::move(data))
{
    auto loader = AnimationClipLoader();
    for (const auto& clip : m_animationControllerData.Clips)
    {
        if (sk_sp<RuntimeAnimationClip> runtimeClip = loader.LoadAsset(clip.second))
        {
            runtimeClip->SetName(clip.first);
            m_animationClips[clip.first] = runtimeClip;
        }
        else
        {
            LogWarn("加载动画剪辑 {} 失败", clip.first);
        }
    }


    for (auto& var : m_animationControllerData.Variables)
    {
        m_variables[var.Name] = var.Value;
        m_variableTypes[var.Name] = var.Type;

        if (var.Type == VariableType::VariableType_Trigger)
        {
            m_variables[var.Name] = false;
        }
    }
}

void RuntimeAnimationController::PlayAnimation(const Guid& guid, float speed, float transitionDuration)
{
    auto loader = AnimationClipLoader();
    sk_sp<RuntimeAnimationClip> clip = loader.LoadAsset(guid);
    playInternal(clip, speed, transitionDuration);
}

void RuntimeAnimationController::PlayAnimation(const std::string& animationName, float speed, float transitionDuration)
{
    auto it = m_animationClips.find(animationName);
    if (it != m_animationClips.end())
    {
        playInternal(it->second, speed, transitionDuration);
    }
    else
    {
        LogWarn("尝试播放一个不存在的动画: {}", animationName);
    }
}

void RuntimeAnimationController::StopAnimation()
{
    ForceStop = true;
    m_isPlaying = false;
    m_isTransitioning = false;
    m_currentTime = 0.0f;
    m_currentFrameIndex = 0;
}

std::string RuntimeAnimationController::GetCurrentAnimationName() const
{
    return m_currentAnimationName;
}

bool RuntimeAnimationController::IsAnimationPlaying(const std::string& animationName)
{
    return m_isPlaying && m_currentAnimationName == animationName;
}

void RuntimeAnimationController::SetVariable(const std::string& name, std::variant<float, bool, int> value)
{
    if (m_variables.contains(name))
    {
        m_variables[name] = value;
    }
    else
    {
        LogWarn("尝试设置未定义的动画变量: {}", name);
    }
}

void RuntimeAnimationController::SetFrameRate(float frameRate)
{
    if (frameRate > 0.0f)
    {
        m_frameRate = frameRate;
    }
    else
    {
        LogWarn("无效的帧率设置: {}", frameRate);
    }
}

float RuntimeAnimationController::GetFrameRate() const
{
    return m_frameRate;
}

void RuntimeAnimationController::PlayEntryAnimation()
{
    if (!EntryPlayed)
    {
        EntryPlayed = true;

        if (m_animationControllerData.States.count(SpecialStateGuids::Entry()))
        {
            const auto& entryState = m_animationControllerData.States.at(SpecialStateGuids::Entry());
            if (!entryState.Transitions.empty())
            {
                const auto& entryTransition = entryState.Transitions[0];
                PlayAnimation(entryTransition.ToGuid, 1.0f, 0.0f);
                return;
            }
        }
    }
}

const Transition* RuntimeAnimationController::FindBestTransition(bool animationHasFinished)
{
    const Transition* bestCandidate = nullptr;


    auto evaluate = [&](const Transition& t)
    {
        if (EvaluateCondition(t.Conditions))
        {
            if (!bestCandidate || t.priority > bestCandidate->priority)
            {
                bestCandidate = &t;
            }
        }
    };


    if (m_animationControllerData.States.contains(SpecialStateGuids::AnyState()))
    {
        const auto& anyState = m_animationControllerData.States.at(SpecialStateGuids::AnyState());
        for (const auto& transition : anyState.Transitions)
        {
            if (transition.ToGuid != m_currentAnimationGuid)
            {
                evaluate(transition);
            }
        }
    }


    if (m_animationControllerData.States.contains(m_currentAnimationGuid))
    {
        const auto& currentState = m_animationControllerData.States.at(m_currentAnimationGuid);
        for (const auto& transition : currentState.Transitions)
        {
            if (!transition.hasExitTime || animationHasFinished)
            {
                evaluate(transition);
            }
        }
    }

    return bestCandidate;
}

void RuntimeAnimationController::Update(float deltaTime)
{
    if (!SceneManager::GetInstance().GetCurrentScene() || ForceStop)
    {
        ForceStop = false;
        return;
    }

    if (m_isTransitioning)
    {
        UpdateTransition(deltaTime);
        UpdateFrameBasedAnimation(deltaTime);
        return;
    }

    bool isCurrentAnimationFinished = false;
    if (m_isPlaying)
    {
        float animationDuration = (m_totalFrames > 0 && m_frameRate > 0) ? (float)m_totalFrames / m_frameRate : 0.0f;
        if (animationDuration > 0)
        {
            isCurrentAnimationFinished = m_currentTime >= animationDuration;
        }
    }

    if (m_justTransitioned)
    {
        m_justTransitioned = false;
    }
    else
    {
        const Transition* bestTransition = FindBestTransition(isCurrentAnimationFinished);

        if (bestTransition)
        {
            auto loader = AnimationClipLoader();
            sk_sp<RuntimeAnimationClip> nextClip = loader.LoadAsset(bestTransition->ToGuid);

            if (nextClip && nextClip->GetSourceGuid() != m_currentAnimationGuid)
            {
                LogInfo("过渡触发: 从 {} 切换到目标状态", m_currentAnimationName);
                playInternal(nextClip, 1.0f, bestTransition->TransitionDuration);


                for (const auto& condition : bestTransition->Conditions)
                {
                    if (std::holds_alternative<TriggerCondition>(condition))
                    {
                        const auto& triggerCond = std::get<TriggerCondition>(condition);
                        m_variables[triggerCond.VarName] = false;
                    }
                }
            }
        }
    }

    UpdateFrameBasedAnimation(deltaTime);
}
