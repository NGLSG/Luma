/**
 * @file RenderBufferManager.cpp
 * @brief 渲染缓冲区管理器实现
 * 
 * Feature: 2d-lighting-enhancement
 * Requirements: 12.1, 12.2, 12.3, 12.4
 */

#include "RenderBufferManager.h"
#include "../Utils/Logger.h"
#include <algorithm>
#include <sstream>

// 静态成员初始化
RenderBufferManager* RenderBufferManager::s_instance = nullptr;

RenderBufferManager& RenderBufferManager::GetInstance()
{
    if (!s_instance)
    {
        s_instance = new RenderBufferManager();
    }
    return *s_instance;
}

void RenderBufferManager::DestroyInstance()
{
    if (s_instance)
    {
        s_instance->Shutdown();
        delete s_instance;
        s_instance = nullptr;
    }
}

RenderBufferManager::RenderBufferManager()
{
    // 预分配临时缓冲区池
    m_tempBufferPool.reserve(MAX_TEMP_BUFFERS);
}

RenderBufferManager::~RenderBufferManager()
{
    Shutdown();
}

// ==================== 初始化和清理 ====================

bool RenderBufferManager::Initialize(
    const std::shared_ptr<Nut::NutContext>& context,
    uint32_t baseWidth,
    uint32_t baseHeight)
{
    if (m_initialized)
    {
        LogWarn("RenderBufferManager already initialized");
        return true;
    }

    if (!context)
    {
        LogError("RenderBufferManager::Initialize: Invalid NutContext");
        return false;
    }

    if (baseWidth == 0 || baseHeight == 0)
    {
        LogError("RenderBufferManager::Initialize: Invalid dimensions {}x{}", baseWidth, baseHeight);
        return false;
    }

    m_context = context;
    m_baseWidth = baseWidth;
    m_baseHeight = baseHeight;

    // 应用默认配置
    ApplyDefaultConfigs();

    // 创建所有启用的缓冲区
    RecreateAllBuffers();

    m_initialized = true;
    LogInfo("RenderBufferManager initialized with base size {}x{}", baseWidth, baseHeight);

    return true;
}

void RenderBufferManager::Shutdown()
{
    if (!m_initialized)
    {
        return;
    }

    // 销毁所有缓冲区
    for (size_t i = 0; i < static_cast<size_t>(RenderBufferType::Count); ++i)
    {
        DestroyBuffer(static_cast<RenderBufferType>(i));
    }

    // 清空临时缓冲区池
    ClearTempBufferPool();

    // 清空回调
    m_resizeCallbacks.clear();

    m_context.reset();
    m_initialized = false;

    LogInfo("RenderBufferManager shutdown");
}

// ==================== 缓冲区配置 ====================

void RenderBufferManager::SetBufferConfig(RenderBufferType type, const BufferConfig& config)
{
    size_t index = static_cast<size_t>(type);
    if (index >= static_cast<size_t>(RenderBufferType::Count))
    {
        LogError("RenderBufferManager::SetBufferConfig: Invalid buffer type");
        return;
    }

    m_buffers[index].config = config;
    m_buffers[index].isDirty = true;

    if (m_initialized && config.enabled)
    {
        CreateBuffer(type);
    }
    else if (m_initialized && !config.enabled)
    {
        DestroyBuffer(type);
    }
}

const BufferConfig& RenderBufferManager::GetBufferConfig(RenderBufferType type) const
{
    size_t index = static_cast<size_t>(type);
    if (index >= static_cast<size_t>(RenderBufferType::Count))
    {
        static BufferConfig defaultConfig;
        return defaultConfig;
    }
    return m_buffers[index].config;
}

void RenderBufferManager::ApplyDefaultConfigs()
{
    // 根据设计文档设置默认配置
    // Requirements: 12.1
    
    // Light Buffer - HDR 格式，全分辨率
    m_buffers[static_cast<size_t>(RenderBufferType::Light)].config = 
        BufferConfig(wgpu::TextureFormat::RGBA16Float, 1.0f, true, true, "LightBuffer");

    // Shadow Buffer - 单通道浮点
    m_buffers[static_cast<size_t>(RenderBufferType::Shadow)].config = 
        BufferConfig(wgpu::TextureFormat::R32Float, 1.0f, true, true, "ShadowBuffer");

    // Emission Buffer - HDR 格式
    m_buffers[static_cast<size_t>(RenderBufferType::Emission)].config = 
        BufferConfig(wgpu::TextureFormat::RGBA16Float, 1.0f, true, true, "EmissionBuffer");

    // Normal Buffer - 有符号归一化
    m_buffers[static_cast<size_t>(RenderBufferType::Normal)].config = 
        BufferConfig(wgpu::TextureFormat::RGBA8Snorm, 1.0f, true, true, "NormalBuffer");

    // Bloom Buffer - HDR 格式，半分辨率
    m_buffers[static_cast<size_t>(RenderBufferType::Bloom)].config = 
        BufferConfig(wgpu::TextureFormat::RGBA16Float, 0.5f, true, true, "BloomBuffer");

    // Bloom Temp Buffer - HDR 格式，半分辨率
    m_buffers[static_cast<size_t>(RenderBufferType::BloomTemp)].config = 
        BufferConfig(wgpu::TextureFormat::RGBA16Float, 0.5f, true, true, "BloomTempBuffer");

    // Light Shaft Buffer - HDR 格式
    m_buffers[static_cast<size_t>(RenderBufferType::LightShaft)].config = 
        BufferConfig(wgpu::TextureFormat::RGBA16Float, 1.0f, false, true, "LightShaftBuffer");

    // Fog Buffer - HDR 格式
    m_buffers[static_cast<size_t>(RenderBufferType::Fog)].config = 
        BufferConfig(wgpu::TextureFormat::RGBA16Float, 1.0f, false, true, "FogBuffer");

    // Tone Mapping Buffer - 标准格式
    m_buffers[static_cast<size_t>(RenderBufferType::ToneMapping)].config = 
        BufferConfig(wgpu::TextureFormat::RGBA8Unorm, 1.0f, true, true, "ToneMappingBuffer");

    // Composite Buffer - HDR 格式
    m_buffers[static_cast<size_t>(RenderBufferType::Composite)].config = 
        BufferConfig(wgpu::TextureFormat::RGBA16Float, 1.0f, true, true, "CompositeBuffer");

    // G-Buffer Position - 高精度位置
    m_buffers[static_cast<size_t>(RenderBufferType::GBufferPosition)].config = 
        BufferConfig(wgpu::TextureFormat::RGBA16Float, 1.0f, true, true, "GBufferPosition");

    // G-Buffer Normal - 有符号归一化法线
    m_buffers[static_cast<size_t>(RenderBufferType::GBufferNormal)].config = 
        BufferConfig(wgpu::TextureFormat::RGBA8Snorm, 1.0f, true, true, "GBufferNormal");

    // G-Buffer Albedo - 标准颜色
    m_buffers[static_cast<size_t>(RenderBufferType::GBufferAlbedo)].config = 
        BufferConfig(wgpu::TextureFormat::RGBA8Unorm, 1.0f, true, true, "GBufferAlbedo");

    // G-Buffer Material - 材质属性
    m_buffers[static_cast<size_t>(RenderBufferType::GBufferMaterial)].config = 
        BufferConfig(wgpu::TextureFormat::RGBA8Unorm, 1.0f, true, true, "GBufferMaterial");

    // Forward Buffer - HDR 格式（用于透明物体）
    m_buffers[static_cast<size_t>(RenderBufferType::Forward)].config = 
        BufferConfig(wgpu::TextureFormat::RGBA16Float, 1.0f, true, true, "ForwardBuffer");
}

// ==================== 缓冲区访问 ====================

std::shared_ptr<Nut::RenderTarget> RenderBufferManager::GetBuffer(RenderBufferType type)
{
    size_t index = static_cast<size_t>(type);
    if (index >= static_cast<size_t>(RenderBufferType::Count))
    {
        return nullptr;
    }

    auto& info = m_buffers[index];
    
    // 如果缓冲区脏或不存在，尝试创建
    if (info.isDirty && info.config.enabled && m_initialized)
    {
        CreateBuffer(type);
    }

    // 更新使用时间
    if (info.target)
    {
        info.lastUsedFrame = m_currentFrame;
    }

    return info.target;
}

wgpu::TextureView RenderBufferManager::GetBufferView(RenderBufferType type)
{
    auto buffer = GetBuffer(type);
    if (buffer)
    {
        return buffer->GetView();
    }
    return wgpu::TextureView();
}

bool RenderBufferManager::IsBufferValid(RenderBufferType type) const
{
    size_t index = static_cast<size_t>(type);
    if (index >= static_cast<size_t>(RenderBufferType::Count))
    {
        return false;
    }
    return m_buffers[index].target != nullptr && m_buffers[index].config.enabled;
}

uint32_t RenderBufferManager::GetBufferWidth(RenderBufferType type) const
{
    size_t index = static_cast<size_t>(type);
    if (index >= static_cast<size_t>(RenderBufferType::Count))
    {
        return 0;
    }
    return m_buffers[index].width;
}

uint32_t RenderBufferManager::GetBufferHeight(RenderBufferType type) const
{
    size_t index = static_cast<size_t>(type);
    if (index >= static_cast<size_t>(RenderBufferType::Count))
    {
        return 0;
    }
    return m_buffers[index].height;
}

// ==================== 动态分辨率 ====================

void RenderBufferManager::SetGlobalRenderScale(float scale)
{
    // 钳制到有效范围
    scale = std::clamp(scale, 0.25f, 2.0f);
    
    if (std::abs(m_globalRenderScale - scale) < 0.001f)
    {
        return; // 无变化
    }

    m_globalRenderScale = scale;
    
    // 标记所有缓冲区为脏
    for (auto& info : m_buffers)
    {
        info.isDirty = true;
    }

    if (m_initialized)
    {
        RecreateAllBuffers();
        LogInfo("Global render scale changed to {:.2f}", scale);
    }
}

void RenderBufferManager::SetBufferScale(RenderBufferType type, float scale)
{
    size_t index = static_cast<size_t>(type);
    if (index >= static_cast<size_t>(RenderBufferType::Count))
    {
        return;
    }

    scale = std::clamp(scale, 0.1f, 2.0f);
    
    auto& info = m_buffers[index];
    if (std::abs(info.config.scale - scale) < 0.001f)
    {
        return; // 无变化
    }

    info.config.scale = scale;
    info.isDirty = true;

    if (m_initialized && info.config.enabled)
    {
        CreateBuffer(type);
    }
}

float RenderBufferManager::GetBufferScale(RenderBufferType type) const
{
    size_t index = static_cast<size_t>(type);
    if (index >= static_cast<size_t>(RenderBufferType::Count))
    {
        return 1.0f;
    }
    return m_buffers[index].config.scale;
}

// ==================== 窗口大小变化响应 ====================

void RenderBufferManager::OnWindowResize(uint32_t newWidth, uint32_t newHeight)
{
    if (newWidth == 0 || newHeight == 0)
    {
        LogWarn("RenderBufferManager::OnWindowResize: Invalid dimensions {}x{}", newWidth, newHeight);
        return;
    }

    if (m_baseWidth == newWidth && m_baseHeight == newHeight)
    {
        return; // 无变化
    }

    m_baseWidth = newWidth;
    m_baseHeight = newHeight;

    // 标记所有缓冲区为脏
    for (auto& info : m_buffers)
    {
        info.isDirty = true;
    }

    if (m_initialized)
    {
        RecreateAllBuffers();
        NotifyResizeCallbacks();
        LogInfo("Window resized to {}x{}, buffers recreated", newWidth, newHeight);
    }
}

int RenderBufferManager::RegisterResizeCallback(BufferResizeCallback callback)
{
    int id = m_nextCallbackId++;
    m_resizeCallbacks.emplace_back(id, std::move(callback));
    return id;
}

void RenderBufferManager::UnregisterResizeCallback(int callbackId)
{
    m_resizeCallbacks.erase(
        std::remove_if(m_resizeCallbacks.begin(), m_resizeCallbacks.end(),
            [callbackId](const auto& pair) { return pair.first == callbackId; }),
        m_resizeCallbacks.end()
    );
}

void RenderBufferManager::NotifyResizeCallbacks()
{
    for (const auto& [id, callback] : m_resizeCallbacks)
    {
        if (callback)
        {
            callback(m_baseWidth, m_baseHeight);
        }
    }
}


// ==================== 缓冲区复用 ====================

std::shared_ptr<Nut::RenderTarget> RenderBufferManager::AcquireTempBuffer(const TempBufferRequest& request)
{
    // 查找匹配的空闲缓冲区
    for (auto& entry : m_tempBufferPool)
    {
        if (!entry.inUse &&
            entry.width == request.width &&
            entry.height == request.height &&
            entry.format == request.format)
        {
            entry.inUse = true;
            entry.lastUsedFrame = m_currentFrame;
            
            if (m_debugMode)
            {
                LogDebug("Reusing temp buffer {}x{} (pool size: {})", 
                         request.width, request.height, m_tempBufferPool.size());
            }
            
            return entry.target;
        }
    }

    // 没有匹配的，创建新的
    if (m_tempBufferPool.size() >= MAX_TEMP_BUFFERS)
    {
        // 池已满，尝试清理过期的
        CleanupExpiredTempBuffers();
        
        // 如果还是满的，移除最旧的未使用缓冲区
        if (m_tempBufferPool.size() >= MAX_TEMP_BUFFERS)
        {
            auto oldest = std::min_element(m_tempBufferPool.begin(), m_tempBufferPool.end(),
                [](const TempBufferEntry& a, const TempBufferEntry& b) {
                    if (a.inUse != b.inUse) return !a.inUse; // 优先移除未使用的
                    return a.lastUsedFrame < b.lastUsedFrame;
                });
            
            if (oldest != m_tempBufferPool.end() && !oldest->inUse)
            {
                m_tempBufferPool.erase(oldest);
            }
        }
    }

    // 创建新缓冲区
    auto target = CreateRenderTarget(request.width, request.height, request.format, request.debugName);
    if (!target)
    {
        LogError("Failed to create temp buffer {}x{}", request.width, request.height);
        return nullptr;
    }

    TempBufferEntry entry;
    entry.target = target;
    entry.width = request.width;
    entry.height = request.height;
    entry.format = request.format;
    entry.lastUsedFrame = m_currentFrame;
    entry.inUse = true;

    m_tempBufferPool.push_back(entry);

    if (m_debugMode)
    {
        LogDebug("Created new temp buffer {}x{} (pool size: {})", 
                 request.width, request.height, m_tempBufferPool.size());
    }

    return target;
}

void RenderBufferManager::ReleaseTempBuffer(std::shared_ptr<Nut::RenderTarget> buffer)
{
    if (!buffer)
    {
        return;
    }

    for (auto& entry : m_tempBufferPool)
    {
        if (entry.target == buffer)
        {
            entry.inUse = false;
            entry.lastUsedFrame = m_currentFrame;
            return;
        }
    }

    // 缓冲区不在池中，忽略
    LogWarn("RenderBufferManager::ReleaseTempBuffer: Buffer not found in pool");
}

void RenderBufferManager::CleanupExpiredTempBuffers()
{
    auto it = m_tempBufferPool.begin();
    while (it != m_tempBufferPool.end())
    {
        if (!it->inUse && (m_currentFrame - it->lastUsedFrame) > TEMP_BUFFER_EXPIRE_FRAMES)
        {
            if (m_debugMode)
            {
                LogDebug("Removing expired temp buffer {}x{}", it->width, it->height);
            }
            it = m_tempBufferPool.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

void RenderBufferManager::ClearTempBufferPool()
{
    m_tempBufferPool.clear();
}

// ==================== 帧管理 ====================

void RenderBufferManager::BeginFrame()
{
    m_currentFrame++;
}

void RenderBufferManager::EndFrame()
{
    // 每 60 帧清理一次过期缓冲区
    if (m_currentFrame % 60 == 0)
    {
        CleanupExpiredTempBuffers();
    }
}

// ==================== 调试功能 ====================

std::string RenderBufferManager::GetBufferDebugInfo(RenderBufferType type) const
{
    size_t index = static_cast<size_t>(type);
    if (index >= static_cast<size_t>(RenderBufferType::Count))
    {
        return "Invalid buffer type";
    }

    const auto& info = m_buffers[index];
    std::ostringstream oss;
    
    oss << GetBufferTypeName(type) << ": ";
    
    if (!info.config.enabled)
    {
        oss << "Disabled";
    }
    else if (!info.target)
    {
        oss << "Not created";
    }
    else
    {
        oss << info.width << "x" << info.height;
        oss << " (scale: " << info.config.scale << ")";
        oss << " [" << info.config.debugName << "]";
    }

    return oss.str();
}

size_t RenderBufferManager::GetTotalMemoryUsage() const
{
    size_t total = 0;

    // 持久缓冲区
    for (const auto& info : m_buffers)
    {
        if (info.target && info.config.enabled)
        {
            total += CalculateTextureMemorySize(info.width, info.height, info.config.format);
        }
    }

    // 临时缓冲区
    for (const auto& entry : m_tempBufferPool)
    {
        total += CalculateTextureMemorySize(entry.width, entry.height, entry.format);
    }

    return total;
}

const char* RenderBufferManager::GetBufferTypeName(RenderBufferType type)
{
    switch (type)
    {
        case RenderBufferType::Light: return "Light";
        case RenderBufferType::Shadow: return "Shadow";
        case RenderBufferType::Emission: return "Emission";
        case RenderBufferType::Normal: return "Normal";
        case RenderBufferType::Bloom: return "Bloom";
        case RenderBufferType::BloomTemp: return "BloomTemp";
        case RenderBufferType::LightShaft: return "LightShaft";
        case RenderBufferType::Fog: return "Fog";
        case RenderBufferType::ToneMapping: return "ToneMapping";
        case RenderBufferType::Composite: return "Composite";
        case RenderBufferType::GBufferPosition: return "GBufferPosition";
        case RenderBufferType::GBufferNormal: return "GBufferNormal";
        case RenderBufferType::GBufferAlbedo: return "GBufferAlbedo";
        case RenderBufferType::GBufferMaterial: return "GBufferMaterial";
        case RenderBufferType::Forward: return "Forward";
        default: return "Unknown";
    }
}

// ==================== 质量等级集成 ====================

void RenderBufferManager::ApplyQualitySettings(
    float renderScale,
    bool enableBloom,
    bool enableLightShafts,
    bool enableFog)
{
    // 设置全局渲染缩放
    SetGlobalRenderScale(renderScale);

    // 启用/禁用特定缓冲区
    m_buffers[static_cast<size_t>(RenderBufferType::Bloom)].config.enabled = enableBloom;
    m_buffers[static_cast<size_t>(RenderBufferType::BloomTemp)].config.enabled = enableBloom;
    m_buffers[static_cast<size_t>(RenderBufferType::LightShaft)].config.enabled = enableLightShafts;
    m_buffers[static_cast<size_t>(RenderBufferType::Fog)].config.enabled = enableFog;

    // 标记为脏并重新创建
    for (size_t i = 0; i < static_cast<size_t>(RenderBufferType::Count); ++i)
    {
        m_buffers[i].isDirty = true;
    }

    if (m_initialized)
    {
        RecreateAllBuffers();
    }
}

// ==================== 私有方法 ====================

bool RenderBufferManager::CreateBuffer(RenderBufferType type)
{
    size_t index = static_cast<size_t>(type);
    if (index >= static_cast<size_t>(RenderBufferType::Count))
    {
        return false;
    }

    auto& info = m_buffers[index];
    
    if (!info.config.enabled)
    {
        info.target.reset();
        info.isDirty = false;
        return true;
    }

    // 计算实际尺寸
    uint32_t width, height;
    CalculateBufferSize(type, width, height);

    // 如果尺寸相同且缓冲区存在，不需要重新创建
    if (!info.isDirty && info.target && info.width == width && info.height == height)
    {
        return true;
    }

    // 创建新的渲染目标
    auto target = CreateRenderTarget(width, height, info.config.format, info.config.debugName);
    if (!target)
    {
        LogError("Failed to create buffer: {}", GetBufferTypeName(type));
        return false;
    }

    info.target = target;
    info.width = width;
    info.height = height;
    info.isDirty = false;

    if (m_debugMode)
    {
        LogDebug("Created buffer {}: {}x{}", GetBufferTypeName(type), width, height);
    }

    return true;
}

void RenderBufferManager::DestroyBuffer(RenderBufferType type)
{
    size_t index = static_cast<size_t>(type);
    if (index >= static_cast<size_t>(RenderBufferType::Count))
    {
        return;
    }

    auto& info = m_buffers[index];
    info.target.reset();
    info.width = 0;
    info.height = 0;
    info.isDirty = true;
}

void RenderBufferManager::RecreateAllBuffers()
{
    for (size_t i = 0; i < static_cast<size_t>(RenderBufferType::Count); ++i)
    {
        auto type = static_cast<RenderBufferType>(i);
        if (m_buffers[i].config.enabled)
        {
            CreateBuffer(type);
        }
        else
        {
            DestroyBuffer(type);
        }
    }
}

void RenderBufferManager::CalculateBufferSize(RenderBufferType type, uint32_t& outWidth, uint32_t& outHeight) const
{
    size_t index = static_cast<size_t>(type);
    if (index >= static_cast<size_t>(RenderBufferType::Count))
    {
        outWidth = m_baseWidth;
        outHeight = m_baseHeight;
        return;
    }

    const auto& config = m_buffers[index].config;
    
    // 应用全局缩放和缓冲区特定缩放
    float totalScale = m_globalRenderScale * config.scale;
    
    outWidth = static_cast<uint32_t>(m_baseWidth * totalScale);
    outHeight = static_cast<uint32_t>(m_baseHeight * totalScale);

    // 确保最小尺寸
    outWidth = std::max(outWidth, 1u);
    outHeight = std::max(outHeight, 1u);
}

std::shared_ptr<Nut::RenderTarget> RenderBufferManager::CreateRenderTarget(
    uint32_t width,
    uint32_t height,
    wgpu::TextureFormat format,
    const std::string& debugName)
{
    if (!m_context)
    {
        LogError("RenderBufferManager::CreateRenderTarget: No context");
        return nullptr;
    }

    wgpu::TextureDescriptor desc;
    desc.size.width = width;
    desc.size.height = height;
    desc.size.depthOrArrayLayers = 1;
    desc.mipLevelCount = 1;
    desc.sampleCount = 1;
    desc.dimension = wgpu::TextureDimension::e2D;
    desc.format = format;
    desc.usage = wgpu::TextureUsage::RenderAttachment | 
                 wgpu::TextureUsage::TextureBinding |
                 wgpu::TextureUsage::CopySrc |
                 wgpu::TextureUsage::CopyDst;
    
    if (!debugName.empty())
    {
        desc.label = debugName.c_str();
    }

    wgpu::Texture texture = m_context->GetWGPUDevice().CreateTexture(&desc);
    if (!texture)
    {
        return nullptr;
    }

    return std::make_shared<Nut::RenderTarget>(texture, static_cast<uint16_t>(width), static_cast<uint16_t>(height));
}

size_t RenderBufferManager::CalculateTextureMemorySize(
    uint32_t width,
    uint32_t height,
    wgpu::TextureFormat format)
{
    size_t bytesPerPixel = 4; // 默认 RGBA8

    switch (format)
    {
        case wgpu::TextureFormat::R8Unorm:
        case wgpu::TextureFormat::R8Snorm:
            bytesPerPixel = 1;
            break;
        case wgpu::TextureFormat::R16Float:
            bytesPerPixel = 2;
            break;
        case wgpu::TextureFormat::R32Float:
            bytesPerPixel = 4;
            break;
        case wgpu::TextureFormat::RG8Unorm:
        case wgpu::TextureFormat::RG8Snorm:
            bytesPerPixel = 2;
            break;
        case wgpu::TextureFormat::RG16Float:
            bytesPerPixel = 4;
            break;
        case wgpu::TextureFormat::RG32Float:
            bytesPerPixel = 8;
            break;
        case wgpu::TextureFormat::RGBA8Unorm:
        case wgpu::TextureFormat::RGBA8Snorm:
            bytesPerPixel = 4;
            break;
        case wgpu::TextureFormat::RGBA16Float:
            bytesPerPixel = 8;
            break;
        case wgpu::TextureFormat::RGBA32Float:
            bytesPerPixel = 16;
            break;
        default:
            bytesPerPixel = 4;
            break;
    }

    return static_cast<size_t>(width) * height * bytesPerPixel;
}
