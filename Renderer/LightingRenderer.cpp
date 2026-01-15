#include "LightingRenderer.h"
#include "../Systems/LightingSystem.h"
#include "../Systems/ShadowRenderer.h"
#include "../Systems/IndirectLightingSystem.h"
#include "../Systems/AreaLightSystem.h"
#include "../Systems/AmbientZoneSystem.h"
#include "Nut/NutContext.h"
#include "Logger.h"

LightingRenderer* LightingRenderer::s_instance = nullptr;

LightingRenderer::LightingRenderer()
{
    // 初始化默认光照数据（仅环境光）
    m_defaultGlobalData.ambientColor = {0.3f, 0.3f, 0.3f, 1.0f};
    m_defaultGlobalData.ambientIntensity = 1.0f;
    m_defaultGlobalData.lightCount = 0;
    m_defaultGlobalData.maxLightsPerPixel = 8;
    m_defaultGlobalData.enableShadows = 0;
}

LightingRenderer::~LightingRenderer()
{
    Shutdown();
    if (s_instance == this)
    {
        s_instance = nullptr;
    }
}

LightingRenderer& LightingRenderer::GetInstance()
{
    if (!s_instance)
    {
        static LightingRenderer instance;
        s_instance = &instance;
    }
    return *s_instance;
}

bool LightingRenderer::Initialize(const std::shared_ptr<Nut::NutContext>& context)
{
    if (m_initialized)
        return true;

    if (!context)
    {
        LogError("LightingRenderer::Initialize - NutContext is null");
        return false;
    }

    m_context = context;
    CreateDefaultBuffers();
    CreateDefaultShadowBuffers();
    CreateDefaultIndirectBuffers();
    CreateDefaultAreaLightBuffers();
    CreateDefaultAmbientZoneBuffers();
    CreateEmissionGlobalBuffer();
    m_initialized = true;

    LogInfo("LightingRenderer initialized");
    return true;
}

void LightingRenderer::Shutdown()
{
    m_defaultGlobalBuffer.reset();
    m_defaultLightBuffer.reset();
    m_defaultShadowParamsBuffer.reset();
    m_defaultShadowEdgeBuffer.reset();
    m_defaultIndirectGlobalBuffer.reset();
    m_defaultReflectorBuffer.reset();
    m_defaultAreaLightGlobalBuffer.reset();
    m_defaultAreaLightBuffer.reset();
    m_defaultAmbientZoneGlobalBuffer.reset();
    m_defaultAmbientZoneBuffer.reset();
    m_emissionBuffer.reset();
    m_emissionGlobalBuffer.reset();
    m_emissionBufferWidth = 0;
    m_emissionBufferHeight = 0;
    m_lightingSystem = nullptr;
    m_context.reset();
    m_initialized = false;
}

void LightingRenderer::SetLightingSystem(Systems::LightingSystem* lightingSystem)
{
    m_lightingSystem = lightingSystem;
}

void LightingRenderer::SetAmbientZoneSystem(Systems::AmbientZoneSystem* ambientZoneSystem)
{
    m_ambientZoneSystem = ambientZoneSystem;
}

void LightingRenderer::CreateDefaultBuffers()
{
    if (!m_context)
        return;

    // 创建默认全局光照数据缓冲区
    Nut::BufferLayout globalLayout;
    globalLayout.usage = Nut::BufferBuilder::GetCommonUniformUsage();
    globalLayout.size = sizeof(ECS::LightingGlobalData);
    globalLayout.mapped = false;

    m_defaultGlobalBuffer = std::make_shared<Nut::Buffer>(globalLayout, m_context);

    // 创建默认光源数据缓冲区（至少1个光源大小，避免空缓冲区）
    Nut::BufferLayout lightLayout;
    lightLayout.usage = Nut::BufferBuilder::GetCommonStorageUsage();
    lightLayout.size = sizeof(ECS::LightData);
    lightLayout.mapped = false;

    m_defaultLightBuffer = std::make_shared<Nut::Buffer>(lightLayout, m_context);

    // 写入默认数据
    UpdateDefaultLightingData();
}

void LightingRenderer::CreateDefaultShadowBuffers()
{
    if (!m_context)
        return;

    // 创建默认阴影参数缓冲区
    Nut::BufferLayout paramsLayout;
    paramsLayout.usage = Nut::BufferBuilder::GetCommonUniformUsage();
    paramsLayout.size = sizeof(Systems::ShadowParams);
    paramsLayout.mapped = false;

    m_defaultShadowParamsBuffer = std::make_shared<Nut::Buffer>(paramsLayout, m_context);

    // 创建默认阴影边缘缓冲区（至少1个边缘大小）
    Nut::BufferLayout edgeLayout;
    edgeLayout.usage = Nut::BufferBuilder::GetCommonStorageUsage();
    edgeLayout.size = sizeof(Systems::GPUShadowEdge);
    edgeLayout.mapped = false;

    m_defaultShadowEdgeBuffer = std::make_shared<Nut::Buffer>(edgeLayout, m_context);

    // 写入默认数据
    Systems::ShadowParams defaultParams{};
    defaultParams.edgeCount = 0;
    defaultParams.shadowSoftness = 1.0f;
    defaultParams.shadowBias = 0.005f;
    m_defaultShadowParamsBuffer->WriteBuffer(&defaultParams, sizeof(Systems::ShadowParams));

    Systems::GPUShadowEdge emptyEdge{};
    m_defaultShadowEdgeBuffer->WriteBuffer(&emptyEdge, sizeof(Systems::GPUShadowEdge));
}

void LightingRenderer::UpdateDefaultLightingData()
{
    if (m_defaultGlobalBuffer)
    {
        m_defaultGlobalBuffer->WriteBuffer(&m_defaultGlobalData, sizeof(ECS::LightingGlobalData));
    }

    // 写入一个空的光源数据（占位）
    if (m_defaultLightBuffer)
    {
        ECS::LightData emptyLight{};
        m_defaultLightBuffer->WriteBuffer(&emptyLight, sizeof(ECS::LightData));
    }
}

std::shared_ptr<Nut::Buffer> LightingRenderer::GetGlobalBuffer() const
{
    if (m_lightingSystem)
    {
        auto buffer = m_lightingSystem->GetGlobalBuffer();
        if (buffer)
            return buffer;
    }
    return m_defaultGlobalBuffer;
}

std::shared_ptr<Nut::Buffer> LightingRenderer::GetLightBuffer() const
{
    if (m_lightingSystem)
    {
        auto buffer = m_lightingSystem->GetLightBuffer();
        if (buffer)
            return buffer;
    }
    return m_defaultLightBuffer;
}

std::shared_ptr<Nut::Buffer> LightingRenderer::GetShadowParamsBuffer() const
{
    auto* shadowRenderer = Systems::ShadowRenderer::GetInstance();
    if (shadowRenderer)
    {
        auto buffer = shadowRenderer->GetParamsBuffer();
        if (buffer)
            return buffer;
    }
    return m_defaultShadowParamsBuffer;
}

std::shared_ptr<Nut::Buffer> LightingRenderer::GetShadowEdgeBuffer() const
{
    auto* shadowRenderer = Systems::ShadowRenderer::GetInstance();
    if (shadowRenderer)
    {
        auto buffer = shadowRenderer->GetEdgeBuffer();
        if (buffer)
            return buffer;
    }
    return m_defaultShadowEdgeBuffer;
}

uint32_t LightingRenderer::GetLightCount() const
{
    if (m_lightingSystem)
    {
        return m_lightingSystem->GetLightCount();
    }
    return 0;
}

uint32_t LightingRenderer::GetShadowEdgeCount() const
{
    auto* shadowRenderer = Systems::ShadowRenderer::GetInstance();
    if (shadowRenderer)
    {
        return shadowRenderer->GetEdgeCount();
    }
    return 0;
}

bool LightingRenderer::IsShadowEnabled() const
{
    auto* shadowRenderer = Systems::ShadowRenderer::GetInstance();
    return shadowRenderer != nullptr && shadowRenderer->IsEnabled() && shadowRenderer->GetEdgeCount() > 0;
}

void LightingRenderer::BindLightingData(Nut::RenderPipeline* pipeline, uint32_t groupIndex)
{
    if (!pipeline || !m_initialized)
        return;

    auto globalBuffer = GetGlobalBuffer();
    auto lightBuffer = GetLightBuffer();

    if (globalBuffer)
    {
        pipeline->SetBinding(groupIndex, 0, *globalBuffer);
    }

    if (lightBuffer)
    {
        pipeline->SetBinding(groupIndex, 1, *lightBuffer);
    }

    // 绑定面光源数据到 Group 1, Binding 2 和 3
    // Feature: 2d-lighting-enhancement
    auto areaLightGlobalBuffer = GetAreaLightGlobalBuffer();
    auto areaLightBuffer = GetAreaLightBuffer();

    if (areaLightGlobalBuffer)
    {
        pipeline->SetBinding(groupIndex, 2, *areaLightGlobalBuffer);
    }

    if (areaLightBuffer)
    {
        pipeline->SetBinding(groupIndex, 3, *areaLightBuffer);
    }

    // 重新构建绑定组以应用更改
    if (m_context)
    {
        pipeline->BuildBindings(m_context);
    }
}

void LightingRenderer::BindShadowData(Nut::RenderPipeline* pipeline, uint32_t groupIndex)
{
    if (!pipeline || !m_initialized)
        return;

    auto paramsBuffer = GetShadowParamsBuffer();
    auto edgeBuffer = GetShadowEdgeBuffer();

    if (paramsBuffer)
    {
        pipeline->SetBinding(groupIndex, 0, *paramsBuffer);
    }

    if (edgeBuffer)
    {
        pipeline->SetBinding(groupIndex, 1, *edgeBuffer);
    }

    // 重新构建绑定组以应用更改
    if (m_context)
    {
        pipeline->BuildBindings(m_context);
    }
}

void LightingRenderer::BindAllLightingData(Nut::RenderPipeline* pipeline, 
                                            uint32_t lightingGroupIndex,
                                            uint32_t shadowGroupIndex)
{
    if (!pipeline || !m_initialized)
        return;

    // 绑定光照数据
    auto globalBuffer = GetGlobalBuffer();
    auto lightBuffer = GetLightBuffer();

    if (globalBuffer)
    {
        pipeline->SetBinding(lightingGroupIndex, 0, *globalBuffer);
    }

    if (lightBuffer)
    {
        pipeline->SetBinding(lightingGroupIndex, 1, *lightBuffer);
    }

    // 绑定面光源数据到 Group 1, Binding 2 和 3
    // Feature: 2d-lighting-enhancement
    auto areaLightGlobalBuffer = GetAreaLightGlobalBuffer();
    auto areaLightBuffer = GetAreaLightBuffer();

    if (areaLightGlobalBuffer)
    {
        pipeline->SetBinding(lightingGroupIndex, 2, *areaLightGlobalBuffer);
    }

    if (areaLightBuffer)
    {
        pipeline->SetBinding(lightingGroupIndex, 3, *areaLightBuffer);
    }

    // 绑定阴影数据
    auto paramsBuffer = GetShadowParamsBuffer();
    auto edgeBuffer = GetShadowEdgeBuffer();

    if (paramsBuffer)
    {
        pipeline->SetBinding(shadowGroupIndex, 0, *paramsBuffer);
    }

    if (edgeBuffer)
    {
        pipeline->SetBinding(shadowGroupIndex, 1, *edgeBuffer);
    }

    // 重新构建绑定组以应用更改
    if (m_context)
    {
        pipeline->BuildBindings(m_context);
    }
}

void LightingRenderer::CreateDefaultIndirectBuffers()
{
    if (!m_context)
        return;

    // 创建默认间接光照全局数据缓冲区
    Nut::BufferLayout globalLayout;
    globalLayout.usage = Nut::BufferBuilder::GetCommonUniformUsage();
    globalLayout.size = sizeof(ECS::IndirectLightingGlobalData);
    globalLayout.mapped = false;

    m_defaultIndirectGlobalBuffer = std::make_shared<Nut::Buffer>(globalLayout, m_context);

    // 创建默认反射体数据缓冲区
    Nut::BufferLayout reflectorLayout;
    reflectorLayout.usage = Nut::BufferBuilder::GetCommonStorageUsage();
    reflectorLayout.size = sizeof(ECS::IndirectLightData);
    reflectorLayout.mapped = false;

    m_defaultReflectorBuffer = std::make_shared<Nut::Buffer>(reflectorLayout, m_context);

    // 写入默认数据
    ECS::IndirectLightingGlobalData defaultGlobal{};
    defaultGlobal.reflectorCount = 0;
    defaultGlobal.enableIndirect = 0;
    m_defaultIndirectGlobalBuffer->WriteBuffer(&defaultGlobal, sizeof(ECS::IndirectLightingGlobalData));

    ECS::IndirectLightData emptyReflector{};
    m_defaultReflectorBuffer->WriteBuffer(&emptyReflector, sizeof(ECS::IndirectLightData));
}

void LightingRenderer::CreateDefaultAreaLightBuffers()
{
    if (!m_context)
        return;

    // 创建默认面光源全局数据缓冲区（16 字节）
    // Feature: 2d-lighting-enhancement
    Nut::BufferLayout globalLayout;
    globalLayout.usage = Nut::BufferBuilder::GetCommonUniformUsage();
    globalLayout.size = 16; // areaLightCount + 3 padding
    globalLayout.mapped = false;

    m_defaultAreaLightGlobalBuffer = std::make_shared<Nut::Buffer>(globalLayout, m_context);

    // 创建默认面光源数据缓冲区（至少1个面光源大小，64字节）
    Nut::BufferLayout areaLightLayout;
    areaLightLayout.usage = Nut::BufferBuilder::GetCommonStorageUsage();
    areaLightLayout.size = sizeof(ECS::AreaLightData);
    areaLightLayout.mapped = false;

    m_defaultAreaLightBuffer = std::make_shared<Nut::Buffer>(areaLightLayout, m_context);

    // 写入默认数据
    uint32_t defaultGlobal[4] = {0, 0, 0, 0}; // areaLightCount = 0
    m_defaultAreaLightGlobalBuffer->WriteBuffer(defaultGlobal, sizeof(defaultGlobal));

    ECS::AreaLightData emptyAreaLight{};
    m_defaultAreaLightBuffer->WriteBuffer(&emptyAreaLight, sizeof(ECS::AreaLightData));

    LogInfo("Default area light buffers created");
}

void LightingRenderer::CreateDefaultAmbientZoneBuffers()
{
    if (!m_context)
        return;

    // 创建默认环境光区域全局数据缓冲区（16 字节）
    // Feature: 2d-lighting-enhancement
    Nut::BufferLayout globalLayout;
    globalLayout.usage = Nut::BufferBuilder::GetCommonUniformUsage();
    globalLayout.size = 16; // ambientZoneCount + 3 padding
    globalLayout.mapped = false;

    m_defaultAmbientZoneGlobalBuffer = std::make_shared<Nut::Buffer>(globalLayout, m_context);

    // 创建默认环境光区域数据缓冲区（至少1个区域大小，80字节）
    Nut::BufferLayout zoneLayout;
    zoneLayout.usage = Nut::BufferBuilder::GetCommonStorageUsage();
    zoneLayout.size = sizeof(ECS::AmbientZoneData);
    zoneLayout.mapped = false;

    m_defaultAmbientZoneBuffer = std::make_shared<Nut::Buffer>(zoneLayout, m_context);

    // 写入默认数据
    uint32_t defaultGlobal[4] = {0, 0, 0, 0}; // ambientZoneCount = 0
    m_defaultAmbientZoneGlobalBuffer->WriteBuffer(defaultGlobal, sizeof(defaultGlobal));

    ECS::AmbientZoneData emptyZone{};
    m_defaultAmbientZoneBuffer->WriteBuffer(&emptyZone, sizeof(ECS::AmbientZoneData));

    LogInfo("Default ambient zone buffers created");
}

std::shared_ptr<Nut::Buffer> LightingRenderer::GetAmbientZoneGlobalBuffer() const
{
    // 从 AmbientZoneSystem 获取环境光区域全局缓冲区
    if (m_ambientZoneSystem)
    {
        auto buffer = m_ambientZoneSystem->GetAmbientZoneGlobalBuffer();
        if (buffer)
            return buffer;
    }
    return m_defaultAmbientZoneGlobalBuffer;
}

std::shared_ptr<Nut::Buffer> LightingRenderer::GetAmbientZoneBuffer() const
{
    // 从 AmbientZoneSystem 获取环境光区域数据缓冲区
    if (m_ambientZoneSystem)
    {
        auto buffer = m_ambientZoneSystem->GetAmbientZoneBuffer();
        if (buffer)
            return buffer;
    }
    return m_defaultAmbientZoneBuffer;
}

std::shared_ptr<Nut::Buffer> LightingRenderer::GetAreaLightGlobalBuffer() const
{
    // 从 LightingSystem 获取面光源全局缓冲区
    if (m_lightingSystem)
    {
        auto buffer = m_lightingSystem->GetAreaLightGlobalBuffer();
        if (buffer)
            return buffer;
    }
    return m_defaultAreaLightGlobalBuffer;
}

std::shared_ptr<Nut::Buffer> LightingRenderer::GetAreaLightBuffer() const
{
    // 从 LightingSystem 获取面光源数据缓冲区
    if (m_lightingSystem)
    {
        auto buffer = m_lightingSystem->GetAreaLightBuffer();
        if (buffer)
            return buffer;
    }
    return m_defaultAreaLightBuffer;
}

std::shared_ptr<Nut::Buffer> LightingRenderer::GetIndirectGlobalBuffer() const
{
    auto* indirectSystem = Systems::IndirectLightingSystem::GetInstance();
    if (indirectSystem)
    {
        auto buffer = indirectSystem->GetGlobalBuffer();
        if (buffer)
            return buffer;
    }
    return m_defaultIndirectGlobalBuffer;
}

std::shared_ptr<Nut::Buffer> LightingRenderer::GetReflectorBuffer() const
{
    auto* indirectSystem = Systems::IndirectLightingSystem::GetInstance();
    if (indirectSystem)
    {
        auto buffer = indirectSystem->GetReflectorBuffer();
        if (buffer)
            return buffer;
    }
    return m_defaultReflectorBuffer;
}

bool LightingRenderer::IsIndirectLightingEnabled() const
{
    auto* indirectSystem = Systems::IndirectLightingSystem::GetInstance();
    return indirectSystem != nullptr && indirectSystem->IsEnabled() && indirectSystem->GetReflectorCount() > 0;
}

uint32_t LightingRenderer::GetReflectorCount() const
{
    auto* indirectSystem = Systems::IndirectLightingSystem::GetInstance();
    if (indirectSystem)
    {
        return indirectSystem->GetReflectorCount();
    }
    return 0;
}

void LightingRenderer::BindIndirectLightingData(Nut::RenderPipeline* pipeline, uint32_t groupIndex)
{
    if (!pipeline || !m_initialized)
        return;

    auto globalBuffer = GetIndirectGlobalBuffer();
    auto reflectorBuffer = GetReflectorBuffer();

    if (globalBuffer)
    {
        pipeline->SetBinding(groupIndex, 0, *globalBuffer);
    }

    if (reflectorBuffer)
    {
        pipeline->SetBinding(groupIndex, 1, *reflectorBuffer);
    }

    // 绑定自发光全局数据到 Binding 2
    // 即使 shader 不使用 emissionGlobal，也需要绑定以匹配 bind group layout
    auto emissionGlobalBuffer = GetEmissionGlobalBuffer();
    if (emissionGlobalBuffer)
    {
        pipeline->SetBinding(groupIndex, 2, *emissionGlobalBuffer);
    }

    // 绑定环境光区域数据到 Binding 3 和 4
    // Feature: 2d-lighting-enhancement
    auto ambientZoneGlobalBuffer = GetAmbientZoneGlobalBuffer();
    auto ambientZoneBuffer = GetAmbientZoneBuffer();

    if (ambientZoneGlobalBuffer)
    {
        pipeline->SetBinding(groupIndex, 3, *ambientZoneGlobalBuffer);
    }

    if (ambientZoneBuffer)
    {
        pipeline->SetBinding(groupIndex, 4, *ambientZoneBuffer);
    }

    // 重新构建绑定组以应用更改
    if (m_context)
    {
        pipeline->BuildBindings(m_context);
    }
}

void LightingRenderer::BindAllLightingDataWithIndirect(Nut::RenderPipeline* pipeline, 
                                                        uint32_t lightingGroupIndex,
                                                        uint32_t shadowGroupIndex,
                                                        uint32_t indirectGroupIndex)
{
    if (!pipeline || !m_initialized)
        return;

    // 绑定光照数据
    auto globalBuffer = GetGlobalBuffer();
    auto lightBuffer = GetLightBuffer();

    if (globalBuffer)
    {
        pipeline->SetBinding(lightingGroupIndex, 0, *globalBuffer);
    }

    if (lightBuffer)
    {
        pipeline->SetBinding(lightingGroupIndex, 1, *lightBuffer);
    }

    // 绑定面光源数据到 Group 1, Binding 2 和 3
    // Feature: 2d-lighting-enhancement
    auto areaLightGlobalBuffer = GetAreaLightGlobalBuffer();
    auto areaLightBuffer = GetAreaLightBuffer();

    if (areaLightGlobalBuffer)
    {
        pipeline->SetBinding(lightingGroupIndex, 2, *areaLightGlobalBuffer);
    }

    if (areaLightBuffer)
    {
        pipeline->SetBinding(lightingGroupIndex, 3, *areaLightBuffer);
    }

    // 绑定阴影数据
    auto paramsBuffer = GetShadowParamsBuffer();
    auto edgeBuffer = GetShadowEdgeBuffer();

    if (paramsBuffer)
    {
        pipeline->SetBinding(shadowGroupIndex, 0, *paramsBuffer);
    }

    if (edgeBuffer)
    {
        pipeline->SetBinding(shadowGroupIndex, 1, *edgeBuffer);
    }

    // 绑定间接光照数据
    auto indirectGlobalBuffer = GetIndirectGlobalBuffer();
    auto reflectorBuffer = GetReflectorBuffer();

    if (indirectGlobalBuffer)
    {
        pipeline->SetBinding(indirectGroupIndex, 0, *indirectGlobalBuffer);
    }

    if (reflectorBuffer)
    {
        pipeline->SetBinding(indirectGroupIndex, 1, *reflectorBuffer);
    }

    // 绑定自发光全局数据到 Group 3, Binding 2
    // 即使 shader 不使用 emissionGlobal，也需要绑定以匹配 bind group layout
    auto emissionGlobalBuffer = GetEmissionGlobalBuffer();
    if (emissionGlobalBuffer)
    {
        pipeline->SetBinding(indirectGroupIndex, 2, *emissionGlobalBuffer);
    }

    // 绑定环境光区域数据到 Group 3, Binding 3 和 4
    // Feature: 2d-lighting-enhancement
    auto ambientZoneGlobalBuffer = GetAmbientZoneGlobalBuffer();
    auto ambientZoneBuffer = GetAmbientZoneBuffer();

    if (ambientZoneGlobalBuffer)
    {
        pipeline->SetBinding(indirectGroupIndex, 3, *ambientZoneGlobalBuffer);
    }

    if (ambientZoneBuffer)
    {
        pipeline->SetBinding(indirectGroupIndex, 4, *ambientZoneBuffer);
    }

    // 重新构建绑定组以应用更改
    if (m_context)
    {
        pipeline->BuildBindings(m_context);
    }
}

// ============================================================================
// Emission Data 绑定实现
// Feature: 2d-lighting-enhancement
// Requirements: 4.3
// ============================================================================

void LightingRenderer::BindEmissionData(Nut::RenderPipeline* pipeline, uint32_t groupIndex, uint32_t bindingIndex)
{
    if (!pipeline || !m_initialized)
        return;

    auto emissionGlobalBuffer = GetEmissionGlobalBuffer();

    if (emissionGlobalBuffer)
    {
        pipeline->SetBinding(groupIndex, bindingIndex, *emissionGlobalBuffer);
    }

    // 重新构建绑定组以应用更改
    if (m_context)
    {
        pipeline->BuildBindings(m_context);
    }
}

void LightingRenderer::BindAllLightingDataWithEmission(Nut::RenderPipeline* pipeline, 
                                                        uint32_t lightingGroupIndex,
                                                        uint32_t shadowGroupIndex,
                                                        uint32_t indirectGroupIndex)
{
    if (!pipeline || !m_initialized)
        return;

    // 绑定光照数据
    auto globalBuffer = GetGlobalBuffer();
    auto lightBuffer = GetLightBuffer();

    if (globalBuffer)
    {
        pipeline->SetBinding(lightingGroupIndex, 0, *globalBuffer);
    }

    if (lightBuffer)
    {
        pipeline->SetBinding(lightingGroupIndex, 1, *lightBuffer);
    }

    // 绑定面光源数据到 Group 1, Binding 2 和 3
    // Feature: 2d-lighting-enhancement
    auto areaLightGlobalBuffer = GetAreaLightGlobalBuffer();
    auto areaLightBuffer = GetAreaLightBuffer();

    if (areaLightGlobalBuffer)
    {
        pipeline->SetBinding(lightingGroupIndex, 2, *areaLightGlobalBuffer);
    }

    if (areaLightBuffer)
    {
        pipeline->SetBinding(lightingGroupIndex, 3, *areaLightBuffer);
    }

    // 绑定阴影数据
    auto paramsBuffer = GetShadowParamsBuffer();
    auto edgeBuffer = GetShadowEdgeBuffer();

    if (paramsBuffer)
    {
        pipeline->SetBinding(shadowGroupIndex, 0, *paramsBuffer);
    }

    if (edgeBuffer)
    {
        pipeline->SetBinding(shadowGroupIndex, 1, *edgeBuffer);
    }

    // 绑定间接光照数据
    auto indirectGlobalBuffer = GetIndirectGlobalBuffer();
    auto reflectorBuffer = GetReflectorBuffer();

    if (indirectGlobalBuffer)
    {
        pipeline->SetBinding(indirectGroupIndex, 0, *indirectGlobalBuffer);
    }

    if (reflectorBuffer)
    {
        pipeline->SetBinding(indirectGroupIndex, 1, *reflectorBuffer);
    }

    // 绑定自发光数据到 Group 3, Binding 2（与间接光照同组）
    auto emissionGlobalBuffer = GetEmissionGlobalBuffer();

    if (emissionGlobalBuffer)
    {
        pipeline->SetBinding(indirectGroupIndex, 2, *emissionGlobalBuffer);
    }

    // 绑定环境光区域数据到 Group 3, Binding 3 和 4
    // Feature: 2d-lighting-enhancement
    auto ambientZoneGlobalBuffer = GetAmbientZoneGlobalBuffer();
    auto ambientZoneBuffer = GetAmbientZoneBuffer();

    if (ambientZoneGlobalBuffer)
    {
        pipeline->SetBinding(indirectGroupIndex, 3, *ambientZoneGlobalBuffer);
    }

    if (ambientZoneBuffer)
    {
        pipeline->SetBinding(indirectGroupIndex, 4, *ambientZoneBuffer);
    }

    // 重新构建绑定组以应用更改
    if (m_context)
    {
        pipeline->BuildBindings(m_context);
    }
}

// ============================================================================
// Emission Buffer 管理实现
// Feature: 2d-lighting-enhancement
// Requirements: 4.3
// ============================================================================

void LightingRenderer::CreateEmissionGlobalBuffer()
{
    if (!m_context)
        return;

    // 创建自发光全局数据缓冲区
    Nut::BufferLayout layout;
    layout.usage = Nut::BufferBuilder::GetCommonUniformUsage();
    layout.size = sizeof(EmissionGlobalData);
    layout.mapped = false;

    m_emissionGlobalBuffer = std::make_shared<Nut::Buffer>(layout, m_context);

    // 写入默认数据
    m_emissionGlobalData.emissionEnabled = 1;
    m_emissionGlobalData.emissionScale = 1.0f;
    m_emissionGlobalBuffer->WriteBuffer(&m_emissionGlobalData, sizeof(EmissionGlobalData));
}

bool LightingRenderer::CreateEmissionBuffer(uint32_t width, uint32_t height)
{
    if (!m_context || width == 0 || height == 0)
    {
        LogError("LightingRenderer::CreateEmissionBuffer - Invalid parameters");
        return false;
    }

    // 如果尺寸相同且缓冲区已存在，不需要重新创建
    if (m_emissionBuffer && m_emissionBufferWidth == width && m_emissionBufferHeight == height)
    {
        return true;
    }

    // 释放旧的缓冲区
    m_emissionBuffer.reset();

    // 创建新的 Emission Buffer 纹理
    // 使用 RGBA16Float 格式以支持 HDR 自发光值
    wgpu::TextureDescriptor textureDesc{};
    textureDesc.label = "EmissionBuffer";
    textureDesc.size = {width, height, 1};
    textureDesc.mipLevelCount = 1;
    textureDesc.sampleCount = 1;
    textureDesc.dimension = wgpu::TextureDimension::e2D;
    textureDesc.format = wgpu::TextureFormat::RGBA16Float;
    textureDesc.usage = wgpu::TextureUsage::RenderAttachment | 
                        wgpu::TextureUsage::TextureBinding |
                        wgpu::TextureUsage::CopySrc;

    wgpu::Texture texture = m_context->GetWGPUDevice().CreateTexture(&textureDesc);
    if (!texture)
    {
        LogError("LightingRenderer::CreateEmissionBuffer - Failed to create texture");
        return false;
    }

    m_emissionBuffer = std::make_shared<Nut::RenderTarget>(
        texture, 
        static_cast<uint16_t>(width), 
        static_cast<uint16_t>(height)
    );

    m_emissionBufferWidth = width;
    m_emissionBufferHeight = height;

    LogInfo("EmissionBuffer created: {}x{}", width, height);
    return true;
}

std::shared_ptr<Nut::RenderTarget> LightingRenderer::GetEmissionBuffer() const
{
    return m_emissionBuffer;
}

wgpu::TextureView LightingRenderer::GetEmissionBufferView() const
{
    if (m_emissionBuffer)
    {
        return m_emissionBuffer->GetView();
    }
    return nullptr;
}

std::shared_ptr<Nut::Buffer> LightingRenderer::GetEmissionGlobalBuffer() const
{
    return m_emissionGlobalBuffer;
}

void LightingRenderer::UpdateEmissionGlobalData(bool enabled, float scale)
{
    m_emissionGlobalData.emissionEnabled = enabled ? 1 : 0;
    m_emissionGlobalData.emissionScale = scale;

    if (m_emissionGlobalBuffer)
    {
        m_emissionGlobalBuffer->WriteBuffer(&m_emissionGlobalData, sizeof(EmissionGlobalData));
    }
}

void LightingRenderer::ClearEmissionBuffer()
{
    // 清除 Emission Buffer 需要在渲染 Pass 中进行
    // 这里只是标记需要清除，实际清除在渲染时进行
    // 可以通过设置 RenderPass 的 loadOp 为 Clear 来实现
}

bool LightingRenderer::IsEmissionBufferValid() const
{
    return m_emissionBuffer != nullptr && 
           m_emissionBufferWidth > 0 && 
           m_emissionBufferHeight > 0;
}

uint32_t LightingRenderer::GetEmissionBufferWidth() const
{
    return m_emissionBufferWidth;
}

uint32_t LightingRenderer::GetEmissionBufferHeight() const
{
    return m_emissionBufferHeight;
}
