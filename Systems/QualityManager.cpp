#include "QualityManager.h"
#include "LightingSystem.h"
#include "ShadowRenderer.h"
#include "PostProcessSystem.h"
#include "../Utils/Logger.h"
#include <algorithm>
#include <numeric>
#include <cmath>

namespace Systems
{
    // 静态成员初始化
    QualityManager* QualityManager::s_instance = nullptr;

    QualityManager& QualityManager::GetInstance()
    {
        if (!s_instance)
        {
            s_instance = new QualityManager();
        }
        return *s_instance;
    }

    void QualityManager::DestroyInstance()
    {
        if (s_instance)
        {
            delete s_instance;
            s_instance = nullptr;
        }
    }

    QualityManager::QualityManager()
    {
        // 初始化为 High 质量等级
        m_settings = GetPreset(ECS::QualityLevel::High);
        
        // 初始化时间戳
        m_lastSampleTime = std::chrono::steady_clock::now();
        m_lastAdjustTime = std::chrono::steady_clock::now();
        
        // 预分配帧率采样空间
        m_frameRateSamples.reserve(FRAME_RATE_SAMPLE_COUNT);
        
        LogInfo("QualityManager initialized with High quality preset");
    }

    // ==================== 质量等级管理 ====================

    void QualityManager::SetQualityLevel(ECS::QualityLevel level)
    {
        if (m_settings.level == level)
        {
            return; // 无变化
        }

        ECS::QualityLevel oldLevel = m_settings.level;
        
        // 应用预设配置
        m_settings = GetPreset(level);
        
        // 应用到各系统
        ApplySettingsToSystems();
        
        // 触发回调
        NotifyQualityChange(oldLevel, level);
        
        LogInfo("Quality level changed from {} to {}", 
                static_cast<int>(oldLevel), static_cast<int>(level));
    }

    void QualityManager::ApplyCustomSettings(const ECS::QualitySettingsComponent& settings)
    {
        ECS::QualityLevel oldLevel = m_settings.level;
        
        // 复制设置并标记为自定义
        m_settings = settings;
        m_settings.level = ECS::QualityLevel::Custom;
        
        // 钳制参数到有效范围
        m_settings.ClampValues();
        
        // 应用到各系统
        ApplySettingsToSystems();
        
        // 触发回调
        if (oldLevel != ECS::QualityLevel::Custom)
        {
            NotifyQualityChange(oldLevel, ECS::QualityLevel::Custom);
        }
        
        LogInfo("Custom quality settings applied");
    }

    ECS::QualitySettingsComponent QualityManager::GetPreset(ECS::QualityLevel level)
    {
        // 使用 QualitySettingsComponent 的静态方法获取预设
        return ECS::QualitySettingsComponent::GetPreset(level);
    }

    // ==================== 自动质量调整 ====================

    void QualityManager::SetAutoQualityEnabled(bool enable)
    {
        m_settings.enableAutoQuality = enable;
        
        if (enable)
        {
            // 重置帧率采样
            m_frameRateSamples.clear();
            m_lastSampleTime = std::chrono::steady_clock::now();
            m_lastAdjustTime = std::chrono::steady_clock::now();
        }
        
        LogInfo("Auto quality adjustment {}", enable ? "enabled" : "disabled");
    }

    void QualityManager::SetTargetFrameRate(float targetFps)
    {
        m_settings.targetFrameRate = std::clamp(targetFps, 30.0f, 144.0f);
    }

    void QualityManager::SetQualityAdjustThreshold(float threshold)
    {
        m_settings.qualityAdjustThreshold = std::clamp(threshold, 1.0f, 30.0f);
    }

    void QualityManager::UpdateAutoQuality(float currentFrameRate)
    {
        if (!m_settings.enableAutoQuality)
        {
            return;
        }

        // 添加帧率采样
        AddFrameRateSample(currentFrameRate);

        // 检查是否有足够的采样数据
        if (m_frameRateSamples.size() < FRAME_RATE_SAMPLE_COUNT / 2)
        {
            return; // 等待更多采样
        }

        // 检查冷却时间
        auto now = std::chrono::steady_clock::now();
        float timeSinceLastAdjust = std::chrono::duration<float>(now - m_lastAdjustTime).count();
        if (timeSinceLastAdjust < QUALITY_ADJUST_COOLDOWN)
        {
            return; // 冷却中
        }

        // 计算平均帧率
        float avgFrameRate = GetAverageFrameRate();

        // 检查是否需要调整质量
        if (ShouldDecreaseQuality(avgFrameRate))
        {
            if (DecreaseQualityLevel())
            {
                m_lastAdjustTime = now;
                m_adjustmentCount++;
                LogInfo("Auto quality decreased due to low frame rate ({:.1f} fps)", avgFrameRate);
            }
        }
        else if (ShouldIncreaseQuality(avgFrameRate))
        {
            if (IncreaseQualityLevel())
            {
                m_lastAdjustTime = now;
                m_adjustmentCount++;
                LogInfo("Auto quality increased due to high frame rate ({:.1f} fps)", avgFrameRate);
            }
        }
    }

    void QualityManager::AddFrameRateSample(float frameRate)
    {
        auto now = std::chrono::steady_clock::now();
        float timeSinceLastSample = std::chrono::duration<float>(now - m_lastSampleTime).count();
        
        // 限制采样频率
        if (timeSinceLastSample < MIN_SAMPLE_INTERVAL)
        {
            return;
        }

        // 添加新采样
        FrameRateSample sample;
        sample.frameRate = frameRate;
        sample.timestamp = now;
        m_frameRateSamples.push_back(sample);
        m_lastSampleTime = now;

        // 移除过旧的采样
        while (m_frameRateSamples.size() > FRAME_RATE_SAMPLE_COUNT)
        {
            m_frameRateSamples.erase(m_frameRateSamples.begin());
        }
    }

    float QualityManager::GetAverageFrameRate() const
    {
        if (m_frameRateSamples.empty())
        {
            return m_settings.targetFrameRate;
        }

        float sum = 0.0f;
        for (const auto& sample : m_frameRateSamples)
        {
            sum += sample.frameRate;
        }
        return sum / static_cast<float>(m_frameRateSamples.size());
    }

    float QualityManager::GetFrameRateStability() const
    {
        if (m_frameRateSamples.size() < 2)
        {
            return 0.0f;
        }

        float avg = GetAverageFrameRate();
        float sumSquaredDiff = 0.0f;
        
        for (const auto& sample : m_frameRateSamples)
        {
            float diff = sample.frameRate - avg;
            sumSquaredDiff += diff * diff;
        }
        
        return std::sqrt(sumSquaredDiff / static_cast<float>(m_frameRateSamples.size()));
    }

    bool QualityManager::ShouldDecreaseQuality(float avgFrameRate) const
    {
        // 如果已经是最低质量，不能再降
        if (m_settings.level == ECS::QualityLevel::Low)
        {
            return false;
        }

        // 帧率低于目标值减去阈值时降低质量
        float lowerBound = m_settings.targetFrameRate - m_settings.qualityAdjustThreshold;
        return avgFrameRate < lowerBound;
    }

    bool QualityManager::ShouldIncreaseQuality(float avgFrameRate) const
    {
        // 如果已经是最高质量或自定义，不能再升
        if (m_settings.level == ECS::QualityLevel::Ultra || 
            m_settings.level == ECS::QualityLevel::Custom)
        {
            return false;
        }

        // 帧率高于目标值加上阈值时提高质量
        float upperBound = m_settings.targetFrameRate + m_settings.qualityAdjustThreshold;
        return avgFrameRate > upperBound;
    }

    bool QualityManager::DecreaseQualityLevel()
    {
        ECS::QualityLevel newLevel;
        
        switch (m_settings.level)
        {
            case ECS::QualityLevel::Ultra:
                newLevel = ECS::QualityLevel::High;
                break;
            case ECS::QualityLevel::High:
                newLevel = ECS::QualityLevel::Medium;
                break;
            case ECS::QualityLevel::Medium:
                newLevel = ECS::QualityLevel::Low;
                break;
            case ECS::QualityLevel::Low:
            case ECS::QualityLevel::Custom:
            default:
                return false; // 无法降低
        }

        SetQualityLevel(newLevel);
        return true;
    }

    bool QualityManager::IncreaseQualityLevel()
    {
        ECS::QualityLevel newLevel;
        
        switch (m_settings.level)
        {
            case ECS::QualityLevel::Low:
                newLevel = ECS::QualityLevel::Medium;
                break;
            case ECS::QualityLevel::Medium:
                newLevel = ECS::QualityLevel::High;
                break;
            case ECS::QualityLevel::High:
                newLevel = ECS::QualityLevel::Ultra;
                break;
            case ECS::QualityLevel::Ultra:
            case ECS::QualityLevel::Custom:
            default:
                return false; // 无法提高
        }

        SetQualityLevel(newLevel);
        return true;
    }

    // ==================== 系统集成 ====================

    void QualityManager::SetLightingSystem(LightingSystem* lightingSystem)
    {
        m_lightingSystem = lightingSystem;
        if (m_lightingSystem)
        {
            ApplyToLightingSystem();
        }
    }

    void QualityManager::SetShadowRenderer(ShadowRenderer* shadowRenderer)
    {
        m_shadowRenderer = shadowRenderer;
        if (m_shadowRenderer)
        {
            ApplyToShadowRenderer();
        }
    }

    void QualityManager::SetPostProcessSystem(PostProcessSystem* postProcessSystem)
    {
        m_postProcessSystem = postProcessSystem;
        if (m_postProcessSystem)
        {
            ApplyToPostProcessSystem();
        }
    }

    void QualityManager::ApplySettingsToSystems()
    {
        ApplyToLightingSystem();
        ApplyToShadowRenderer();
        ApplyToPostProcessSystem();
    }

    void QualityManager::ApplyToLightingSystem()
    {
        if (!m_lightingSystem)
        {
            return;
        }

        // 设置每像素最大光源数
        m_lightingSystem->SetMaxLightsPerPixel(m_settings.maxLightsPerPixel);
        
        // 设置阴影方法
        m_lightingSystem->SetShadowMethod(m_settings.shadowMethod);
        
        LogDebug("Applied quality settings to LightingSystem");
    }

    void QualityManager::ApplyToShadowRenderer()
    {
        if (!m_shadowRenderer)
        {
            return;
        }

        // 设置阴影贴图分辨率
        m_shadowRenderer->SetShadowMapResolution(m_settings.shadowMapResolution);
        
        // 设置阴影方法
        m_shadowRenderer->SetShadowMethod(m_settings.shadowMethod);
        
        // 设置阴影缓存
        m_shadowRenderer->SetShadowCacheEnabled(m_settings.enableShadowCache);
        
        LogDebug("Applied quality settings to ShadowRenderer");
    }

    void QualityManager::ApplyToPostProcessSystem()
    {
        if (!m_postProcessSystem)
        {
            return;
        }

        // 获取当前后处理设置并更新
        ECS::PostProcessSettingsComponent postSettings = m_postProcessSystem->GetSettings();
        
        // 根据质量设置启用/禁用效果
        postSettings.enableBloom = m_settings.enableBloom;
        postSettings.enableLightShafts = m_settings.enableLightShafts;
        postSettings.enableFog = m_settings.enableFog;
        postSettings.enableColorGrading = m_settings.enableColorGrading;
        
        // 注意：PostProcessSystem 没有直接的 ApplySettings 方法
        // 设置会在下一帧通过场景组件更新
        
        LogDebug("Applied quality settings to PostProcessSystem");
    }

    // ==================== 回调管理 ====================

    int QualityManager::RegisterQualityChangeCallback(QualityChangeCallback callback)
    {
        int id = m_nextCallbackId++;
        m_callbacks.emplace_back(id, std::move(callback));
        return id;
    }

    void QualityManager::UnregisterQualityChangeCallback(int callbackId)
    {
        m_callbacks.erase(
            std::remove_if(m_callbacks.begin(), m_callbacks.end(),
                [callbackId](const auto& pair) { return pair.first == callbackId; }),
            m_callbacks.end()
        );
    }

    void QualityManager::NotifyQualityChange(ECS::QualityLevel oldLevel, ECS::QualityLevel newLevel)
    {
        for (const auto& [id, callback] : m_callbacks)
        {
            if (callback)
            {
                callback(oldLevel, newLevel);
            }
        }
    }

    // ==================== 调试 ====================

    float QualityManager::GetTimeSinceLastAdjustment() const
    {
        auto now = std::chrono::steady_clock::now();
        return std::chrono::duration<float>(now - m_lastAdjustTime).count();
    }

    void QualityManager::ResetStatistics()
    {
        m_adjustmentCount = 0;
        m_frameRateSamples.clear();
        m_lastSampleTime = std::chrono::steady_clock::now();
        m_lastAdjustTime = std::chrono::steady_clock::now();
    }
}
