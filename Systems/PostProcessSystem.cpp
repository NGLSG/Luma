#include "PostProcessSystem.h"
#include "QualityManager.h"
#include "RuntimeAsset/RuntimeScene.h"
#include "RuntimeAsset/RuntimeGameObject.h"
#include "../Renderer/GraphicsBackend.h"
#include "../Renderer/Nut/NutContext.h"
#include "../Renderer/Camera.h"
#include "Logger.h"
#include <algorithm>
#include <cmath>

namespace Systems
{
    PostProcessSystem::PostProcessSystem()
        : m_settings()
        , m_bufferConfig()
        , m_initialized(false)
        , m_debugMode(false)
        , m_settingsDirty(true)
        , m_currentWidth(0)
        , m_currentHeight(0)
    {
    }

    void PostProcessSystem::OnCreate(RuntimeScene* scene, EngineContext& engineCtx)
    {
        // 获取 NutContext
        auto* graphicsBackend = engineCtx.graphicsBackend;
        if (!graphicsBackend)
        {
            LogError("PostProcessSystem: GraphicsBackend not available");
            return;
        }

        m_nutContext = graphicsBackend->GetNutContext();
        if (!m_nutContext)
        {
            LogError("PostProcessSystem: NutContext not available");
            return;
        }

        // 从场景获取后处理设置
        UpdateSettingsFromScene(scene);

        // 创建 GPU 缓冲区
        CreateBuffers(engineCtx);

        // 注册到 QualityManager
        QualityManager::GetInstance().SetPostProcessSystem(this);

        m_initialized = true;
        LogInfo("PostProcessSystem initialized");
    }

    void PostProcessSystem::OnUpdate(RuntimeScene* scene, float deltaTime, EngineContext& engineCtx)
    {
        if (!m_initialized)
            return;

        // 从场景更新后处理设置
        UpdateSettingsFromScene(scene);

        // 如果设置发生变化，更新 GPU 缓冲区
        if (m_settingsDirty)
        {
            UpdateGlobalBuffer();
            m_settingsDirty = false;
        }
    }

    void PostProcessSystem::OnDestroy(RuntimeScene* scene)
    {
        // 从 QualityManager 注销
        QualityManager::GetInstance().SetPostProcessSystem(nullptr);

        // 清理渲染缓冲区
        m_emissionBuffer.reset();
        m_bloomBuffer.reset();
        m_tempBuffer.reset();
        m_bloomMipChain.clear();
        m_intermediateBuffers.clear();

        // 清理 GPU 缓冲区
        m_globalBuffer.reset();

        // 重置状态
        m_nutContext.reset();
        m_initialized = false;
        m_settingsDirty = true;
        m_currentWidth = 0;
        m_currentHeight = 0;

        LogInfo("PostProcessSystem destroyed");
    }

    void PostProcessSystem::Execute(
        const ECS::PostProcessSettingsComponent& settings,
        std::shared_ptr<Nut::RenderTarget> input,
        std::shared_ptr<Nut::RenderTarget> output)
    {
        if (!m_initialized || !input || !output)
            return;

        // 验证设置
        ECS::PostProcessSettingsComponent validatedSettings = settings;
        ValidateSettings(validatedSettings);

        // 执行后处理管线
        // 注意：实际的渲染 Pass 将在后续任务中实现
        // 这里只是管线框架

        // 1. Bloom 效果
        if (validatedSettings.enableBloom)
        {
            ExecuteBloom(validatedSettings);
        }

        // 2. 光束效果
        if (validatedSettings.enableLightShafts)
        {
            ExecuteLightShafts(validatedSettings);
        }

        // 3. 雾效
        if (validatedSettings.enableFog)
        {
            ExecuteFog(validatedSettings);
        }

        // 4. 色调映射
        if (validatedSettings.toneMappingMode != ECS::ToneMappingMode::None)
        {
            ExecuteToneMapping(validatedSettings);
        }

        // 5. 颜色分级
        if (validatedSettings.enableColorGrading)
        {
            ExecuteColorGrading(validatedSettings);
        }

        if (m_debugMode)
        {
            LogInfo("PostProcessSystem: Execute complete");
        }
    }

    void PostProcessSystem::ExecuteBloom(const ECS::PostProcessSettingsComponent& settings)
    {
        if (!m_initialized)
            return;

        // 使用内部缓冲区执行 Bloom
        ExecuteBloom(settings, m_emissionBuffer, m_emissionBuffer, m_bloomBuffer);
    }

    void PostProcessSystem::ExecuteBloom(
        const ECS::PostProcessSettingsComponent& settings,
        std::shared_ptr<Nut::RenderTarget> sceneInput,
        std::shared_ptr<Nut::RenderTarget> emissionInput,
        std::shared_ptr<Nut::RenderTarget> output)
    {
        if (!m_initialized || !sceneInput)
            return;

        // 验证设置
        ECS::PostProcessSettingsComponent validatedSettings = settings;
        ValidateSettings(validatedSettings);

        // 确保 Bloom 缓冲区存在
        if (!m_bloomBuffer || m_bloomMipChain.empty())
        {
            if (m_debugMode)
            {
                LogWarn("PostProcessSystem: Bloom buffers not available");
            }
            return;
        }

        // 1. Bloom 提取 Pass - 从场景和 Emission 提取高亮区域
        ExecuteBloomExtract(sceneInput, emissionInput, m_bloomBuffer, validatedSettings);

        // 2. Bloom 降采样模糊 Pass - 多级降采样
        int iterations = std::min(validatedSettings.bloomIterations, 
                                  static_cast<int>(m_bloomMipChain.size()));
        ExecuteBloomDownsample(iterations);

        // 3. Bloom 升采样合并 Pass - 合并各级模糊结果
        ExecuteBloomUpsample(iterations);

        // 4. Bloom 合成 Pass - 叠加到输出
        if (output)
        {
            ExecuteBloomComposite(sceneInput, m_bloomBuffer, output, validatedSettings);
        }

        if (m_debugMode)
        {
            LogInfo("PostProcessSystem: ExecuteBloom complete with {} iterations", iterations);
        }
    }

    void PostProcessSystem::ExecuteLightShafts(const ECS::PostProcessSettingsComponent& settings)
    {
        if (!m_initialized)
            return;

        // 光束效果需要光源信息，这里使用默认值
        // 实际使用时应该从场景中获取光源信息
        glm::vec2 defaultLightPos(0.0f, 0.0f);
        ECS::Color defaultColor(1.0f, 1.0f, 1.0f, 1.0f);
        
        // 使用阴影缓冲区进行遮挡（如果可用）
        ExecuteLightShafts(settings, defaultLightPos, defaultColor, 1.0f, 
                          m_emissionBuffer, m_shadowBuffer, m_lightShaftBuffer);
    }

    void PostProcessSystem::ExecuteLightShafts(
        const ECS::PostProcessSettingsComponent& settings,
        const glm::vec2& lightWorldPos,
        const ECS::Color& lightColor,
        float lightIntensity,
        std::shared_ptr<Nut::RenderTarget> sceneInput,
        std::shared_ptr<Nut::RenderTarget> shadowInput,
        std::shared_ptr<Nut::RenderTarget> output)
    {
        if (!m_initialized || !sceneInput)
            return;

        // 验证设置
        ECS::PostProcessSettingsComponent validatedSettings = settings;
        ValidateSettings(validatedSettings);

        // 确保光束缓冲区存在
        if (!m_lightShaftBuffer)
        {
            if (m_currentWidth > 0 && m_currentHeight > 0)
            {
                m_lightShaftBuffer = CreateRenderTarget(
                    m_currentWidth, m_currentHeight,
                    m_bufferConfig.lightBufferFormat
                );
            }
            
            if (!m_lightShaftBuffer)
            {
                if (m_debugMode)
                {
                    LogWarn("PostProcessSystem: Light shaft buffer not available");
                }
                return;
            }
        }

        // 确保光束参数缓冲区存在
        if (!m_lightShaftParamsBuffer && m_nutContext)
        {
            Nut::BufferLayout paramsLayout;
            paramsLayout.usage = Nut::BufferBuilder::GetCommonUniformUsage();
            paramsLayout.size = sizeof(ECS::LightShaftParams);
            paramsLayout.mapped = false;
            m_lightShaftParamsBuffer = std::make_shared<Nut::Buffer>(paramsLayout, m_nutContext);
        }

        if (!m_lightShaftParamsBuffer)
            return;

        // 将世界坐标转换为屏幕空间 UV
        glm::vec2 screenUV = WorldToScreenUV(lightWorldPos);

        // 构建光束参数
        ECS::LightShaftParams params;
        params.lightScreenPos = screenUV;
        params.lightWorldPos = lightWorldPos;
        params.lightColor = glm::vec4(lightColor.r, lightColor.g, lightColor.b, lightColor.a);
        params.density = validatedSettings.lightShaftDensity;
        params.decay = validatedSettings.lightShaftDecay;
        params.weight = validatedSettings.lightShaftWeight;
        params.exposure = validatedSettings.lightShaftExposure;
        params.numSamples = 64; // 默认采样次数
        params.lightRadius = 1.0f;
        params.lightIntensity = lightIntensity;
        params.enableOcclusion = (shadowInput != nullptr) ? 1u : 0u;

        // 更新光束参数缓冲区
        m_lightShaftParamsBuffer->WriteBuffer(&params, sizeof(ECS::LightShaftParams));

        // 获取设备
        wgpu::Device device = m_nutContext->GetWGPUDevice();
        if (!device)
            return;

        // 确定输出目标
        std::shared_ptr<Nut::RenderTarget> renderOutput = output ? output : m_lightShaftBuffer;

        // 创建命令编码器
        wgpu::CommandEncoderDescriptor encoderDesc{};
        encoderDesc.label = "LightShaftEncoder";
        wgpu::CommandEncoder encoder = device.CreateCommandEncoder(&encoderDesc);

        // 设置渲染 Pass
        wgpu::RenderPassColorAttachment colorAttachment{};
        colorAttachment.view = renderOutput->GetView();
        colorAttachment.loadOp = wgpu::LoadOp::Clear;
        colorAttachment.storeOp = wgpu::StoreOp::Store;
        colorAttachment.clearValue = {0.0f, 0.0f, 0.0f, 0.0f};

        wgpu::RenderPassDescriptor passDesc{};
        passDesc.label = "LightShaftPass";
        passDesc.colorAttachmentCount = 1;
        passDesc.colorAttachments = &colorAttachment;

        wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&passDesc);

        // 注意：实际的管线绑定和绘制需要在管线创建后实现
        // 1. 绑定光束渲染管线（根据是否有遮挡选择）
        // 2. 绑定光束参数缓冲区
        // 3. 绑定场景纹理
        // 4. 绑定阴影纹理（如果有）
        // 5. 绘制全屏三角形

        pass.End();

        // 提交命令
        wgpu::CommandBuffer commands = encoder.Finish();
        m_nutContext->GetWGPUQueue().Submit(1, &commands);

        if (m_debugMode)
        {
            LogInfo("PostProcessSystem: Light shaft pass executed (light at screen UV: {}, {})", 
                    screenUV.x, screenUV.y);
        }
    }

    glm::vec2 PostProcessSystem::WorldToScreenUV(const glm::vec2& worldPos) const
    {
        // 获取相机信息进行坐标转换
        Camera& camera = CameraManager::GetInstance().GetActiveCamera();
        CameraProperties props = camera.GetProperties();

        // 将世界坐标转换为相机空间
        glm::vec2 cameraPos(props.position.x(), props.position.y());
        glm::vec2 relativePos = worldPos - cameraPos;

        // 应用相机旋转
        float sinR = std::sin(-props.rotation);
        float cosR = std::cos(-props.rotation);
        glm::vec2 rotatedPos(
            relativePos.x * cosR - relativePos.y * sinR,
            relativePos.x * sinR + relativePos.y * cosR
        );

        // 应用相机缩放
        glm::vec2 effectiveZoom(props.GetEffectiveZoom().x(), props.GetEffectiveZoom().y());
        glm::vec2 scaledPos = rotatedPos * effectiveZoom;

        // 获取视口尺寸
        float viewportWidth = props.viewport.width();
        float viewportHeight = props.viewport.height();

        // 转换为屏幕空间 UV [0, 1]
        // 屏幕中心是 (0.5, 0.5)
        glm::vec2 screenUV;
        screenUV.x = 0.5f + scaledPos.x / viewportWidth;
        screenUV.y = 0.5f - scaledPos.y / viewportHeight; // Y 轴翻转

        return screenUV;
    }

    void PostProcessSystem::SetShadowBuffer(std::shared_ptr<Nut::RenderTarget> shadowBuffer)
    {
        m_shadowBuffer = shadowBuffer;
        
        if (m_debugMode)
        {
            if (shadowBuffer)
            {
                LogInfo("PostProcessSystem: Shadow buffer set ({}x{})", 
                        shadowBuffer->GetWidth(), shadowBuffer->GetHeight());
            }
            else
            {
                LogInfo("PostProcessSystem: Shadow buffer cleared");
            }
        }
    }

    void PostProcessSystem::ExecuteFog(const ECS::PostProcessSettingsComponent& settings)
    {
        if (!m_initialized)
            return;

        // 验证设置
        ECS::PostProcessSettingsComponent validatedSettings = settings;
        ValidateSettings(validatedSettings);

        // 如果雾效未启用，直接返回
        if (!validatedSettings.enableFog)
        {
            if (m_debugMode)
            {
                LogInfo("PostProcessSystem: Fog disabled, skipping");
            }
            return;
        }

        // 确保雾效参数缓冲区存在
        if (!m_fogParamsBuffer && m_nutContext)
        {
            Nut::BufferLayout paramsLayout;
            paramsLayout.usage = Nut::BufferBuilder::GetCommonUniformUsage();
            paramsLayout.size = sizeof(ECS::FogParams);
            paramsLayout.mapped = false;
            m_fogParamsBuffer = std::make_shared<Nut::Buffer>(paramsLayout, m_nutContext);
        }

        if (!m_fogParamsBuffer)
            return;

        // 获取相机信息
        Camera& camera = CameraManager::GetInstance().GetActiveCamera();
        CameraProperties props = camera.GetProperties();

        // 构建雾效参数
        ECS::FogParams fogParams;
        fogParams.fogColor = glm::vec4(
            validatedSettings.fogColor.r,
            validatedSettings.fogColor.g,
            validatedSettings.fogColor.b,
            validatedSettings.fogColor.a
        );
        fogParams.fogDensity = validatedSettings.fogDensity;
        fogParams.fogStart = validatedSettings.fogStart;
        fogParams.fogEnd = validatedSettings.fogEnd;
        fogParams.fogMode = static_cast<uint32_t>(validatedSettings.fogMode);
        fogParams.heightFogBase = validatedSettings.heightFogBase;
        fogParams.heightFogDensity = validatedSettings.heightFogDensity;
        fogParams.enableHeightFog = validatedSettings.enableHeightFog ? 1u : 0u;
        fogParams.enableFog = 1u;
        fogParams.cameraPosition = glm::vec2(props.position.x(), props.position.y());
        fogParams.cameraZoom = props.GetEffectiveZoom().x();

        // 更新雾效参数缓冲区
        m_fogParamsBuffer->WriteBuffer(&fogParams, sizeof(ECS::FogParams));

        // 获取设备
        wgpu::Device device = m_nutContext->GetWGPUDevice();
        if (!device)
            return;

        // 确保雾效缓冲区存在
        if (!m_fogBuffer && m_currentWidth > 0 && m_currentHeight > 0)
        {
            m_fogBuffer = CreateRenderTarget(
                m_currentWidth, m_currentHeight,
                m_bufferConfig.lightBufferFormat
            );
        }

        if (!m_fogBuffer)
        {
            if (m_debugMode)
            {
                LogWarn("PostProcessSystem: Fog buffer not available");
            }
            return;
        }

        // 创建命令编码器
        wgpu::CommandEncoderDescriptor encoderDesc{};
        encoderDesc.label = "FogEncoder";
        wgpu::CommandEncoder encoder = device.CreateCommandEncoder(&encoderDesc);

        // 设置渲染 Pass
        wgpu::RenderPassColorAttachment colorAttachment{};
        colorAttachment.view = m_fogBuffer->GetView();
        colorAttachment.loadOp = wgpu::LoadOp::Clear;
        colorAttachment.storeOp = wgpu::StoreOp::Store;
        colorAttachment.clearValue = {0.0f, 0.0f, 0.0f, 0.0f};

        wgpu::RenderPassDescriptor passDesc{};
        passDesc.label = "FogPass";
        passDesc.colorAttachmentCount = 1;
        passDesc.colorAttachments = &colorAttachment;

        wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&passDesc);

        // 注意：实际的管线绑定和绘制需要在管线创建后实现
        // 1. 绑定雾效渲染管线
        // 2. 绑定雾效参数缓冲区
        // 3. 绑定场景纹理
        // 4. 绘制全屏三角形

        pass.End();

        // 提交命令
        wgpu::CommandBuffer commands = encoder.Finish();
        m_nutContext->GetWGPUQueue().Submit(1, &commands);

        if (m_debugMode)
        {
            LogInfo("PostProcessSystem: Fog pass executed (mode: {}, density: {}, start: {}, end: {})",
                    static_cast<int>(validatedSettings.fogMode),
                    validatedSettings.fogDensity,
                    validatedSettings.fogStart,
                    validatedSettings.fogEnd);
            if (validatedSettings.enableHeightFog)
            {
                LogInfo("PostProcessSystem: Height fog enabled (base: {}, density: {})",
                        validatedSettings.heightFogBase,
                        validatedSettings.heightFogDensity);
            }
        }
    }

    void PostProcessSystem::SetFogLights(const std::vector<ECS::FogLightData>& lights, float maxPenetration)
    {
        if (!m_initialized || !m_nutContext)
            return;

        // 限制光源数量
        const size_t maxLights = 8;
        size_t lightCount = std::min(lights.size(), maxLights);

        // 创建或更新光源穿透参数缓冲区
        if (!m_fogLightParamsBuffer)
        {
            Nut::BufferLayout paramsLayout;
            paramsLayout.usage = Nut::BufferBuilder::GetCommonUniformUsage();
            paramsLayout.size = sizeof(ECS::FogLightParams);
            paramsLayout.mapped = false;
            m_fogLightParamsBuffer = std::make_shared<Nut::Buffer>(paramsLayout, m_nutContext);
        }

        // 创建或更新光源数据缓冲区
        if (!m_fogLightsBuffer || lightCount > 0)
        {
            size_t bufferSize = std::max(lightCount, size_t(1)) * sizeof(ECS::FogLightData);
            Nut::BufferLayout lightsLayout;
            lightsLayout.usage = wgpu::BufferUsage::Storage | wgpu::BufferUsage::CopyDst;
            lightsLayout.size = bufferSize;
            lightsLayout.mapped = false;
            m_fogLightsBuffer = std::make_shared<Nut::Buffer>(lightsLayout, m_nutContext);
        }

        // 更新光源穿透参数
        ECS::FogLightParams params;
        params.lightCount = static_cast<uint32_t>(lightCount);
        params.enableLightPenetration = lightCount > 0 ? 1u : 0u;
        params.maxPenetration = std::clamp(maxPenetration, 0.0f, 1.0f);
        m_fogLightParamsBuffer->WriteBuffer(&params, sizeof(ECS::FogLightParams));

        // 更新光源数据
        if (lightCount > 0)
        {
            m_fogLightsBuffer->WriteBuffer(lights.data(), lightCount * sizeof(ECS::FogLightData));
        }

        if (m_debugMode)
        {
            LogInfo("PostProcessSystem: Set {} fog lights with max penetration {}", 
                    lightCount, maxPenetration);
        }
    }

    void PostProcessSystem::ClearFogLights()
    {
        if (!m_initialized || !m_fogLightParamsBuffer)
            return;

        // 禁用光源穿透
        ECS::FogLightParams params;
        params.lightCount = 0;
        params.enableLightPenetration = 0;
        params.maxPenetration = 0.0f;
        m_fogLightParamsBuffer->WriteBuffer(&params, sizeof(ECS::FogLightParams));

        if (m_debugMode)
        {
            LogInfo("PostProcessSystem: Cleared fog lights");
        }
    }

    void PostProcessSystem::ExecuteToneMapping(const ECS::PostProcessSettingsComponent& settings)
    {
        if (!m_initialized)
            return;

        // 验证设置
        ECS::PostProcessSettingsComponent validatedSettings = settings;
        ValidateSettings(validatedSettings);

        // 如果色调映射模式为 None，直接返回
        if (validatedSettings.toneMappingMode == ECS::ToneMappingMode::None)
        {
            if (m_debugMode)
            {
                LogInfo("PostProcessSystem: Tone mapping disabled, skipping");
            }
            return;
        }

        // 确保色调映射参数缓冲区存在
        if (!m_toneMappingParamsBuffer && m_nutContext)
        {
            Nut::BufferLayout paramsLayout;
            paramsLayout.usage = Nut::BufferBuilder::GetCommonUniformUsage();
            paramsLayout.size = sizeof(ECS::ToneMappingParams);
            paramsLayout.mapped = false;
            m_toneMappingParamsBuffer = std::make_shared<Nut::Buffer>(paramsLayout, m_nutContext);
        }

        if (!m_toneMappingParamsBuffer)
            return;

        // 构建色调映射参数
        ECS::ToneMappingParams params;
        params.exposure = validatedSettings.exposure;
        params.contrast = validatedSettings.contrast;
        params.saturation = validatedSettings.saturation;
        params.gamma = validatedSettings.gamma;
        params.toneMappingMode = static_cast<uint32_t>(validatedSettings.toneMappingMode);
        params.enableToneMapping = 1u;
        params.enableColorGrading = validatedSettings.enableColorGrading ? 1u : 0u;
        params.enableLUT = (!validatedSettings.lutTexturePath.empty() && validatedSettings.lutIntensity > 0.0f) ? 1u : 0u;
        params.lutIntensity = validatedSettings.lutIntensity;
        params.lutSize = 32.0f; // 标准 LUT 尺寸
        params.whitePoint = 4.0f; // Reinhard 扩展白点
        params.colorBalance = glm::vec3(1.0f, 1.0f, 1.0f); // 默认颜色平衡

        // 更新色调映射参数缓冲区
        m_toneMappingParamsBuffer->WriteBuffer(&params, sizeof(ECS::ToneMappingParams));

        // 获取设备
        wgpu::Device device = m_nutContext->GetWGPUDevice();
        if (!device)
            return;

        // 确保色调映射缓冲区存在
        if (!m_toneMappingBuffer && m_currentWidth > 0 && m_currentHeight > 0)
        {
            m_toneMappingBuffer = CreateRenderTarget(
                m_currentWidth, m_currentHeight,
                wgpu::TextureFormat::RGBA8Unorm // LDR 输出
            );
        }

        if (!m_toneMappingBuffer)
        {
            if (m_debugMode)
            {
                LogWarn("PostProcessSystem: Tone mapping buffer not available");
            }
            return;
        }

        // 创建命令编码器
        wgpu::CommandEncoderDescriptor encoderDesc{};
        encoderDesc.label = "ToneMappingEncoder";
        wgpu::CommandEncoder encoder = device.CreateCommandEncoder(&encoderDesc);

        // 设置渲染 Pass
        wgpu::RenderPassColorAttachment colorAttachment{};
        colorAttachment.view = m_toneMappingBuffer->GetView();
        colorAttachment.loadOp = wgpu::LoadOp::Clear;
        colorAttachment.storeOp = wgpu::StoreOp::Store;
        colorAttachment.clearValue = {0.0f, 0.0f, 0.0f, 1.0f};

        wgpu::RenderPassDescriptor passDesc{};
        passDesc.label = "ToneMappingPass";
        passDesc.colorAttachmentCount = 1;
        passDesc.colorAttachments = &colorAttachment;

        wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&passDesc);

        // 注意：实际的管线绑定和绘制需要在管线创建后实现
        // 1. 绑定色调映射渲染管线
        // 2. 绑定色调映射参数缓冲区
        // 3. 绑定 HDR 场景纹理
        // 4. 绘制全屏三角形

        pass.End();

        // 提交命令
        wgpu::CommandBuffer commands = encoder.Finish();
        m_nutContext->GetWGPUQueue().Submit(1, &commands);

        if (m_debugMode)
        {
            const char* modeNames[] = {"None", "Reinhard", "ACES", "Filmic"};
            int modeIndex = static_cast<int>(validatedSettings.toneMappingMode);
            LogInfo("PostProcessSystem: Tone mapping pass executed (mode: {}, exposure: {}, gamma: {})",
                    modeNames[modeIndex],
                    validatedSettings.exposure,
                    validatedSettings.gamma);
        }
    }

    void PostProcessSystem::ExecuteColorGrading(const ECS::PostProcessSettingsComponent& settings)
    {
        if (!m_initialized)
            return;

        // 验证设置
        ECS::PostProcessSettingsComponent validatedSettings = settings;
        ValidateSettings(validatedSettings);

        // 如果颜色分级未启用且没有 LUT，直接返回
        if (!validatedSettings.enableColorGrading && validatedSettings.lutTexturePath.empty())
        {
            if (m_debugMode)
            {
                LogInfo("PostProcessSystem: Color grading disabled, skipping");
            }
            return;
        }

        // 确保色调映射参数缓冲区存在（颜色分级使用相同的参数结构）
        if (!m_toneMappingParamsBuffer && m_nutContext)
        {
            Nut::BufferLayout paramsLayout;
            paramsLayout.usage = Nut::BufferBuilder::GetCommonUniformUsage();
            paramsLayout.size = sizeof(ECS::ToneMappingParams);
            paramsLayout.mapped = false;
            m_toneMappingParamsBuffer = std::make_shared<Nut::Buffer>(paramsLayout, m_nutContext);
        }

        if (!m_toneMappingParamsBuffer)
            return;

        // 检查是否需要加载新的 LUT 纹理
        bool hasLUT = !validatedSettings.lutTexturePath.empty();
        if (hasLUT && validatedSettings.lutTexturePath != m_currentLutPath)
        {
            // LUT 纹理加载将在实际集成时实现
            // 这里只是标记需要加载
            m_currentLutPath = validatedSettings.lutTexturePath;
            if (m_debugMode)
            {
                LogInfo("PostProcessSystem: LUT texture path changed to: {}", m_currentLutPath);
            }
        }

        // 构建颜色分级参数
        ECS::ToneMappingParams params;
        params.exposure = validatedSettings.exposure;
        params.contrast = validatedSettings.contrast;
        params.saturation = validatedSettings.saturation;
        params.gamma = validatedSettings.gamma;
        params.toneMappingMode = static_cast<uint32_t>(ECS::ToneMappingMode::None); // 颜色分级不需要色调映射
        params.enableToneMapping = 0u;
        params.enableColorGrading = validatedSettings.enableColorGrading ? 1u : 0u;
        params.enableLUT = (hasLUT && validatedSettings.lutIntensity > 0.0f) ? 1u : 0u;
        params.lutIntensity = validatedSettings.lutIntensity;
        params.lutSize = 32.0f;
        params.whitePoint = 4.0f;
        params.colorBalance = glm::vec3(1.0f, 1.0f, 1.0f);

        // 更新参数缓冲区
        m_toneMappingParamsBuffer->WriteBuffer(&params, sizeof(ECS::ToneMappingParams));

        // 获取设备
        wgpu::Device device = m_nutContext->GetWGPUDevice();
        if (!device)
            return;

        // 确保输出缓冲区存在
        if (!m_toneMappingBuffer && m_currentWidth > 0 && m_currentHeight > 0)
        {
            m_toneMappingBuffer = CreateRenderTarget(
                m_currentWidth, m_currentHeight,
                wgpu::TextureFormat::RGBA8Unorm
            );
        }

        if (!m_toneMappingBuffer)
        {
            if (m_debugMode)
            {
                LogWarn("PostProcessSystem: Color grading buffer not available");
            }
            return;
        }

        // 创建命令编码器
        wgpu::CommandEncoderDescriptor encoderDesc{};
        encoderDesc.label = "ColorGradingEncoder";
        wgpu::CommandEncoder encoder = device.CreateCommandEncoder(&encoderDesc);

        // 设置渲染 Pass
        wgpu::RenderPassColorAttachment colorAttachment{};
        colorAttachment.view = m_toneMappingBuffer->GetView();
        colorAttachment.loadOp = wgpu::LoadOp::Clear;
        colorAttachment.storeOp = wgpu::StoreOp::Store;
        colorAttachment.clearValue = {0.0f, 0.0f, 0.0f, 1.0f};

        wgpu::RenderPassDescriptor passDesc{};
        passDesc.label = "ColorGradingPass";
        passDesc.colorAttachmentCount = 1;
        passDesc.colorAttachments = &colorAttachment;

        wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&passDesc);

        // 注意：实际的管线绑定和绘制需要在管线创建后实现
        // 1. 绑定颜色分级渲染管线（或 LUT 管线）
        // 2. 绑定参数缓冲区
        // 3. 绑定场景纹理
        // 4. 绑定 LUT 纹理（如果有）
        // 5. 绘制全屏三角形

        pass.End();

        // 提交命令
        wgpu::CommandBuffer commands = encoder.Finish();
        m_nutContext->GetWGPUQueue().Submit(1, &commands);

        if (m_debugMode)
        {
            LogInfo("PostProcessSystem: Color grading pass executed (contrast: {}, saturation: {}, LUT: {})",
                    validatedSettings.contrast,
                    validatedSettings.saturation,
                    hasLUT ? "enabled" : "disabled");
        }
    }


    void PostProcessSystem::SetBufferConfig(const RenderBufferConfig& config)
    {
        m_bufferConfig = config;
        
        // 如果已初始化且尺寸已知，重新创建缓冲区
        if (m_initialized && m_currentWidth > 0 && m_currentHeight > 0)
        {
            ResizeBuffers(m_currentWidth, m_currentHeight);
        }
    }

    void PostProcessSystem::ResizeBuffers(uint16_t width, uint16_t height)
    {
        if (width == 0 || height == 0)
            return;

        // 如果尺寸没有变化，不需要重新创建
        if (width == m_currentWidth && height == m_currentHeight)
            return;

        m_currentWidth = width;
        m_currentHeight = height;

        if (!m_nutContext)
            return;

        // 重新创建渲染缓冲区
        // Emission 缓冲区（全分辨率）
        m_emissionBuffer = CreateRenderTarget(
            width, height,
            m_bufferConfig.emissionFormat
        );

        // Bloom 缓冲区（可能降采样）
        uint16_t bloomWidth = static_cast<uint16_t>(width * m_bufferConfig.bloomBufferScale);
        uint16_t bloomHeight = static_cast<uint16_t>(height * m_bufferConfig.bloomBufferScale);
        bloomWidth = std::max(bloomWidth, static_cast<uint16_t>(1));
        bloomHeight = std::max(bloomHeight, static_cast<uint16_t>(1));

        m_bloomBuffer = CreateRenderTarget(
            bloomWidth, bloomHeight,
            m_bufferConfig.bloomFormat
        );

        // 临时缓冲区（用于 ping-pong 渲染）
        m_tempBuffer = CreateRenderTarget(
            bloomWidth, bloomHeight,
            m_bufferConfig.bloomFormat
        );

        // 创建 Bloom 降采样链
        CreateBloomMipChain(bloomWidth, bloomHeight);

        LogInfo("PostProcessSystem: Buffers resized to {}x{}", width, height);
    }

    std::shared_ptr<Nut::RenderTarget> PostProcessSystem::GetIntermediateBuffer(PostProcessPassType passType) const
    {
        auto it = m_intermediateBuffers.find(passType);
        if (it != m_intermediateBuffers.end())
        {
            return it->second;
        }
        return nullptr;
    }

    void PostProcessSystem::CreateBuffers(EngineContext& engineCtx)
    {
        if (!m_nutContext)
            return;

        // 创建后处理全局数据缓冲区
        Nut::BufferLayout globalLayout;
        globalLayout.usage = Nut::BufferBuilder::GetCommonUniformUsage();
        globalLayout.size = sizeof(ECS::PostProcessGlobalData);
        globalLayout.mapped = false;

        m_globalBuffer = std::make_shared<Nut::Buffer>(globalLayout, m_nutContext);

        // 写入初始数据
        ECS::PostProcessGlobalData initialData = m_settings.ToPostProcessGlobalData();
        m_globalBuffer->WriteBuffer(&initialData, sizeof(ECS::PostProcessGlobalData));

        LogInfo("PostProcessSystem: GPU buffers created");
    }

    std::shared_ptr<Nut::RenderTarget> PostProcessSystem::CreateRenderTarget(
        uint16_t width,
        uint16_t height,
        wgpu::TextureFormat format)
    {
        if (!m_nutContext || width == 0 || height == 0)
            return nullptr;

        wgpu::Device device = m_nutContext->GetWGPUDevice();
        if (!device)
            return nullptr;

        // 创建纹理描述符
        wgpu::TextureDescriptor textureDesc{};
        textureDesc.size.width = width;
        textureDesc.size.height = height;
        textureDesc.size.depthOrArrayLayers = 1;
        textureDesc.mipLevelCount = 1;
        textureDesc.sampleCount = 1;
        textureDesc.dimension = wgpu::TextureDimension::e2D;
        textureDesc.format = format;
        textureDesc.usage = wgpu::TextureUsage::RenderAttachment |
                           wgpu::TextureUsage::TextureBinding |
                           wgpu::TextureUsage::CopySrc |
                           wgpu::TextureUsage::CopyDst;

        wgpu::Texture texture = device.CreateTexture(&textureDesc);
        if (!texture)
        {
            LogError("PostProcessSystem: Failed to create render target texture");
            return nullptr;
        }

        return std::make_shared<Nut::RenderTarget>(texture, width, height);
    }

    void PostProcessSystem::CreateBloomMipChain(uint16_t baseWidth, uint16_t baseHeight)
    {
        m_bloomMipChain.clear();

        uint16_t width = baseWidth;
        uint16_t height = baseHeight;

        // 创建降采样链
        for (int i = 0; i < MAX_BLOOM_MIP_LEVELS; ++i)
        {
            // 每级降采样一半
            width = std::max(static_cast<uint16_t>(width / 2), static_cast<uint16_t>(1));
            height = std::max(static_cast<uint16_t>(height / 2), static_cast<uint16_t>(1));

            auto mipTarget = CreateRenderTarget(width, height, m_bufferConfig.bloomFormat);
            if (mipTarget)
            {
                m_bloomMipChain.push_back(mipTarget);
            }

            // 如果已经是 1x1，停止创建
            if (width == 1 && height == 1)
                break;
        }

        if (m_debugMode)
        {
            LogInfo("PostProcessSystem: Created {} bloom mip levels", m_bloomMipChain.size());
        }
    }

    void PostProcessSystem::UpdateSettingsFromScene(RuntimeScene* scene)
    {
        if (!scene)
            return;

        auto& registry = scene->GetRegistry();
        auto view = registry.view<ECS::PostProcessSettingsComponent>();

        for (auto entity : view)
        {
            auto& settings = view.get<ECS::PostProcessSettingsComponent>(entity);
            if (settings.Enable)
            {
                // 检查设置是否发生变化
                if (settings != m_settings)
                {
                    m_settings = settings;
                    m_settingsDirty = true;
                }
                return;
            }
        }

        // 如果没有找到后处理设置组件，使用默认值
        ECS::PostProcessSettingsComponent defaultSettings;
        if (defaultSettings != m_settings)
        {
            m_settings = defaultSettings;
            m_settingsDirty = true;
        }
    }

    void PostProcessSystem::UpdateGlobalBuffer()
    {
        if (!m_globalBuffer)
            return;

        // 验证设置
        ECS::PostProcessSettingsComponent validatedSettings = m_settings;
        ValidateSettings(validatedSettings);

        // 转换为 GPU 数据格式
        ECS::PostProcessGlobalData globalData = validatedSettings.ToPostProcessGlobalData();

        // 写入缓冲区
        m_globalBuffer->WriteBuffer(&globalData, sizeof(ECS::PostProcessGlobalData));

        if (m_debugMode)
        {
            LogInfo("PostProcessSystem: Global buffer updated");
        }
    }

    void PostProcessSystem::ValidateSettings(ECS::PostProcessSettingsComponent& settings)
    {
        // Bloom 参数验证
        settings.bloomThreshold = std::max(0.0f, settings.bloomThreshold);
        settings.bloomIntensity = std::max(0.0f, settings.bloomIntensity);
        settings.bloomRadius = std::max(0.0f, settings.bloomRadius);
        settings.bloomIterations = std::clamp(settings.bloomIterations, 1, MAX_BLOOM_ITERATIONS);

        // 光束参数验证
        settings.lightShaftDensity = std::clamp(settings.lightShaftDensity, 0.0f, 1.0f);
        settings.lightShaftDecay = std::clamp(settings.lightShaftDecay, 0.0f, 1.0f);
        settings.lightShaftWeight = std::clamp(settings.lightShaftWeight, 0.0f, 1.0f);
        settings.lightShaftExposure = std::max(0.0f, settings.lightShaftExposure);

        // 雾效参数验证
        settings.fogDensity = std::max(0.0f, settings.fogDensity);
        settings.fogStart = std::max(0.0f, settings.fogStart);
        settings.fogEnd = std::max(settings.fogStart + 0.001f, settings.fogEnd);
        settings.heightFogDensity = std::max(0.0f, settings.heightFogDensity);

        // 色调映射参数验证
        settings.exposure = std::max(0.001f, settings.exposure);
        settings.contrast = std::max(0.0f, settings.contrast);
        settings.saturation = std::max(0.0f, settings.saturation);
        settings.gamma = std::clamp(settings.gamma, 0.1f, 10.0f);

        // LUT 参数验证
        settings.lutIntensity = std::clamp(settings.lutIntensity, 0.0f, 1.0f);
    }

    // ============================================================================
    // Bloom 实现方法
    // Feature: 2d-lighting-enhancement
    // Requirements: 5.1, 5.3, 5.4, 5.5
    // ============================================================================

    void PostProcessSystem::CreateBloomPipelines()
    {
        // Bloom 管线将在需要时延迟创建
        // 这里只是占位符，实际管线创建需要着色器模块
        if (m_debugMode)
        {
            LogInfo("PostProcessSystem: Bloom pipelines creation placeholder");
        }
    }

    void PostProcessSystem::ExecuteBloomExtract(
        std::shared_ptr<Nut::RenderTarget> sceneInput,
        std::shared_ptr<Nut::RenderTarget> emissionInput,
        std::shared_ptr<Nut::RenderTarget> output,
        const ECS::PostProcessSettingsComponent& settings)
    {
        if (!m_nutContext || !sceneInput || !output)
            return;

        // 获取设备和命令编码器
        wgpu::Device device = m_nutContext->GetWGPUDevice();
        if (!device)
            return;

        // 创建命令编码器
        wgpu::CommandEncoderDescriptor encoderDesc{};
        encoderDesc.label = "BloomExtractEncoder";
        wgpu::CommandEncoder encoder = device.CreateCommandEncoder(&encoderDesc);

        // 设置渲染 Pass
        wgpu::RenderPassColorAttachment colorAttachment{};
        colorAttachment.view = output->GetView();
        colorAttachment.loadOp = wgpu::LoadOp::Clear;
        colorAttachment.storeOp = wgpu::StoreOp::Store;
        colorAttachment.clearValue = {0.0f, 0.0f, 0.0f, 0.0f};

        wgpu::RenderPassDescriptor passDesc{};
        passDesc.label = "BloomExtractPass";
        passDesc.colorAttachmentCount = 1;
        passDesc.colorAttachments = &colorAttachment;

        wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&passDesc);

        // 注意：实际的管线绑定和绘制需要在管线创建后实现
        // 这里只是框架，展示 Bloom 提取的逻辑流程
        
        // 1. 绑定 Bloom 提取管线
        // 2. 绑定场景纹理和采样器
        // 3. 绑定 Emission 纹理和采样器（如果有）
        // 4. 绑定后处理全局数据
        // 5. 绘制全屏三角形

        pass.End();

        // 提交命令
        wgpu::CommandBuffer commands = encoder.Finish();
        m_nutContext->GetWGPUQueue().Submit(1, &commands);

        if (m_debugMode)
        {
            LogInfo("PostProcessSystem: Bloom extract pass executed");
        }
    }

    void PostProcessSystem::ExecuteBloomDownsample(int iterations)
    {
        if (!m_nutContext || m_bloomMipChain.empty())
            return;

        wgpu::Device device = m_nutContext->GetWGPUDevice();
        if (!device)
            return;

        // 从 Bloom 缓冲区开始，逐级降采样到 mip chain
        std::shared_ptr<Nut::RenderTarget> source = m_bloomBuffer;

        for (int i = 0; i < iterations && i < static_cast<int>(m_bloomMipChain.size()); ++i)
        {
            std::shared_ptr<Nut::RenderTarget> dest = m_bloomMipChain[i];
            if (!dest)
                continue;

            // 创建命令编码器
            wgpu::CommandEncoderDescriptor encoderDesc{};
            encoderDesc.label = "BloomDownsampleEncoder";
            wgpu::CommandEncoder encoder = device.CreateCommandEncoder(&encoderDesc);

            // 设置渲染 Pass
            wgpu::RenderPassColorAttachment colorAttachment{};
            colorAttachment.view = dest->GetView();
            colorAttachment.loadOp = wgpu::LoadOp::Clear;
            colorAttachment.storeOp = wgpu::StoreOp::Store;
            colorAttachment.clearValue = {0.0f, 0.0f, 0.0f, 0.0f};

            wgpu::RenderPassDescriptor passDesc{};
            passDesc.label = "BloomDownsamplePass";
            passDesc.colorAttachmentCount = 1;
            passDesc.colorAttachments = &colorAttachment;

            wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&passDesc);

            // 注意：实际的管线绑定和绘制需要在管线创建后实现
            // 1. 绑定降采样管线
            // 2. 绑定源纹理
            // 3. 更新 texelSize uniform
            // 4. 绘制全屏三角形

            pass.End();

            // 提交命令
            wgpu::CommandBuffer commands = encoder.Finish();
            m_nutContext->GetWGPUQueue().Submit(1, &commands);

            // 下一级的源是当前级的目标
            source = dest;
        }

        if (m_debugMode)
        {
            LogInfo("PostProcessSystem: Bloom downsample {} iterations executed", iterations);
        }
    }

    void PostProcessSystem::ExecuteBloomUpsample(int iterations)
    {
        if (!m_nutContext || m_bloomMipChain.empty())
            return;

        wgpu::Device device = m_nutContext->GetWGPUDevice();
        if (!device)
            return;

        // 从最小的 mip 开始，逐级升采样并累加
        int startLevel = std::min(iterations - 1, static_cast<int>(m_bloomMipChain.size()) - 1);
        
        for (int i = startLevel; i >= 0; --i)
        {
            std::shared_ptr<Nut::RenderTarget> source = m_bloomMipChain[i];
            std::shared_ptr<Nut::RenderTarget> dest = (i > 0) ? m_bloomMipChain[i - 1] : m_bloomBuffer;
            
            if (!source || !dest)
                continue;

            // 创建命令编码器
            wgpu::CommandEncoderDescriptor encoderDesc{};
            encoderDesc.label = "BloomUpsampleEncoder";
            wgpu::CommandEncoder encoder = device.CreateCommandEncoder(&encoderDesc);

            // 设置渲染 Pass（使用加法混合）
            wgpu::RenderPassColorAttachment colorAttachment{};
            colorAttachment.view = dest->GetView();
            // 使用 Load 而不是 Clear，以便累加
            colorAttachment.loadOp = (i == startLevel) ? wgpu::LoadOp::Clear : wgpu::LoadOp::Load;
            colorAttachment.storeOp = wgpu::StoreOp::Store;
            colorAttachment.clearValue = {0.0f, 0.0f, 0.0f, 0.0f};

            wgpu::RenderPassDescriptor passDesc{};
            passDesc.label = "BloomUpsamplePass";
            passDesc.colorAttachmentCount = 1;
            passDesc.colorAttachments = &colorAttachment;

            wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&passDesc);

            // 注意：实际的管线绑定和绘制需要在管线创建后实现
            // 1. 绑定升采样管线（带加法混合）
            // 2. 绑定源纹理
            // 3. 更新 texelSize uniform
            // 4. 绘制全屏三角形

            pass.End();

            // 提交命令
            wgpu::CommandBuffer commands = encoder.Finish();
            m_nutContext->GetWGPUQueue().Submit(1, &commands);
        }

        if (m_debugMode)
        {
            LogInfo("PostProcessSystem: Bloom upsample {} iterations executed", iterations);
        }
    }

    void PostProcessSystem::ExecuteBloomComposite(
        std::shared_ptr<Nut::RenderTarget> sceneInput,
        std::shared_ptr<Nut::RenderTarget> bloomInput,
        std::shared_ptr<Nut::RenderTarget> output,
        const ECS::PostProcessSettingsComponent& settings)
    {
        if (!m_nutContext || !sceneInput || !bloomInput || !output)
            return;

        wgpu::Device device = m_nutContext->GetWGPUDevice();
        if (!device)
            return;

        // 创建命令编码器
        wgpu::CommandEncoderDescriptor encoderDesc{};
        encoderDesc.label = "BloomCompositeEncoder";
        wgpu::CommandEncoder encoder = device.CreateCommandEncoder(&encoderDesc);

        // 设置渲染 Pass
        wgpu::RenderPassColorAttachment colorAttachment{};
        colorAttachment.view = output->GetView();
        colorAttachment.loadOp = wgpu::LoadOp::Clear;
        colorAttachment.storeOp = wgpu::StoreOp::Store;
        colorAttachment.clearValue = {0.0f, 0.0f, 0.0f, 1.0f};

        wgpu::RenderPassDescriptor passDesc{};
        passDesc.label = "BloomCompositePass";
        passDesc.colorAttachmentCount = 1;
        passDesc.colorAttachments = &colorAttachment;

        wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&passDesc);

        // 注意：实际的管线绑定和绘制需要在管线创建后实现
        // 1. 绑定合成管线
        // 2. 绑定场景纹理
        // 3. 绑定 Bloom 纹理
        // 4. 绑定后处理全局数据（包含 bloomIntensity）
        // 5. 绘制全屏三角形

        pass.End();

        // 提交命令
        wgpu::CommandBuffer commands = encoder.Finish();
        m_nutContext->GetWGPUQueue().Submit(1, &commands);

        if (m_debugMode)
        {
            LogInfo("PostProcessSystem: Bloom composite pass executed");
        }
    }

    void PostProcessSystem::ExecuteGaussianBlurPass(
        std::shared_ptr<Nut::RenderTarget> input,
        std::shared_ptr<Nut::RenderTarget> output,
        bool horizontal)
    {
        if (!m_nutContext || !input || !output)
            return;

        wgpu::Device device = m_nutContext->GetWGPUDevice();
        if (!device)
            return;

        // 创建命令编码器
        wgpu::CommandEncoderDescriptor encoderDesc{};
        encoderDesc.label = horizontal ? "BloomBlurHEncoder" : "BloomBlurVEncoder";
        wgpu::CommandEncoder encoder = device.CreateCommandEncoder(&encoderDesc);

        // 设置渲染 Pass
        wgpu::RenderPassColorAttachment colorAttachment{};
        colorAttachment.view = output->GetView();
        colorAttachment.loadOp = wgpu::LoadOp::Clear;
        colorAttachment.storeOp = wgpu::StoreOp::Store;
        colorAttachment.clearValue = {0.0f, 0.0f, 0.0f, 0.0f};

        wgpu::RenderPassDescriptor passDesc{};
        passDesc.label = horizontal ? "BloomBlurHPass" : "BloomBlurVPass";
        passDesc.colorAttachmentCount = 1;
        passDesc.colorAttachments = &colorAttachment;

        wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&passDesc);

        // 更新模糊参数
        BloomBlurParams params;
        params.texelSizeX = 1.0f / static_cast<float>(input->GetWidth());
        params.texelSizeY = 1.0f / static_cast<float>(input->GetHeight());
        params.directionX = horizontal ? 1.0f : 0.0f;
        params.directionY = horizontal ? 0.0f : 1.0f;

        // 注意：实际的管线绑定和绘制需要在管线创建后实现
        // 1. 绑定模糊管线（水平或垂直）
        // 2. 绑定输入纹理
        // 3. 更新模糊参数 uniform
        // 4. 绘制全屏三角形

        pass.End();

        // 提交命令
        wgpu::CommandBuffer commands = encoder.Finish();
        m_nutContext->GetWGPUQueue().Submit(1, &commands);

        if (m_debugMode)
        {
            LogInfo("PostProcessSystem: Gaussian blur {} pass executed", 
                    horizontal ? "horizontal" : "vertical");
        }
    }
}
