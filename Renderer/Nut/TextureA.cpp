#include "TextureA.h"

#include <future>
#include <iostream>

#include "NutContext.h"
#include "stb_image.h"
#include "stb_image_write.h"

namespace Nut {

TextureA TextureBuilder::Build(std::shared_ptr<NutContext> context)
{
    unsigned char* loadedPixels = nullptr;
    const void* finalPixels = nullptr;
    uint32_t finalWidth = 0;
    uint32_t finalHeight = 0;
    uint32_t finalChannels = 0;

    // 1. 加载像素数据
    if (m_loadFromFile)
    {
        int width, height, channels;
        loadedPixels = stbi_load(m_filePath.c_str(), &width, &height, &channels, STBI_rgb_alpha);

        if (!loadedPixels)
        {
            std::cerr << "加载纹理失败: " << m_filePath << std::endl;
            return nullptr;
        }

        std::cout << "纹理加载成功: " << m_filePath
            << " 宽度: " << width
            << " 高度: " << height
            << " 通道: " << channels << std::endl;

        finalPixels = loadedPixels;
        finalWidth = static_cast<uint32_t>(width);
        finalHeight = static_cast<uint32_t>(height);
        finalChannels = 4; // stbi_load 强制转换为RGBA
    }
    else if (m_loadFromMemory)
    {
        int width, height, channels;
        loadedPixels = stbi_load_from_memory(
            static_cast<const stbi_uc*>(m_memoryData),
            static_cast<int>(m_memorySize),
            &width, &height, &channels,
            STBI_rgb_alpha
        );

        if (!loadedPixels)
        {
            std::cerr << "从内存加载纹理失败" << std::endl;
            return nullptr;
        }

        std::cout << "从内存加载纹理成功"
            << " 宽度: " << width
            << " 高度: " << height
            << " 通道: " << channels << std::endl;

        finalPixels = loadedPixels;
        finalWidth = static_cast<uint32_t>(width);
        finalHeight = static_cast<uint32_t>(height);
        finalChannels = 4;
    }
    else if (m_hasPixelData)
    {
        finalPixels = m_pixelData;
        finalWidth = m_width;
        finalHeight = m_height;
        finalChannels = m_channels;
    }
    else
    {
        std::cerr << "没有提供纹理数据源" << std::endl;
        return nullptr;
    }

    // 2. 更新描述符的尺寸(如果从图片加载)
    if (m_loadFromFile || m_loadFromMemory)
    {
        m_descriptorBuilder.SetSize(finalWidth, finalHeight);
    }

    // 3. 创建纹理
    const auto& descriptor = m_descriptorBuilder.GetDescriptor();
    auto texture = context->GetWGPUDevice().CreateTexture(&descriptor);

    if (!texture)
    {
        std::cerr << "创建纹理失败" << std::endl;
        if (loadedPixels)
            stbi_image_free(loadedPixels);
        return nullptr;
    }

    // 4. 上传像素数据（支持2D和3D纹理）
    if (finalPixels)
    {
        wgpu::TexelCopyTextureInfo destination;
        destination.texture = texture;
        destination.mipLevel = m_uploadConfig.mipLevel;
        destination.aspect = m_uploadConfig.aspect;
        destination.origin = {m_uploadConfig.originX, m_uploadConfig.originY, m_uploadConfig.originZ};

        wgpu::TexelCopyBufferLayout dataLayout;
        dataLayout.offset = 0;

        // 自动计算bytesPerRow和rowsPerImage
        uint32_t bytesPerRow = m_uploadConfig.bytesPerRow;
        if (bytesPerRow == 0)
        {
            bytesPerRow = finalWidth * finalChannels;
        }

        uint32_t rowsPerImage = m_uploadConfig.rowsPerImage;
        if (rowsPerImage == 0)
        {
            rowsPerImage = finalHeight;
        }

        dataLayout.bytesPerRow = bytesPerRow;
        dataLayout.rowsPerImage = rowsPerImage;

        // 对于3D纹理或纹理数组，使用实际深度
        uint32_t uploadDepth = (m_hasPixelData && m_depth > 1) ? m_depth : 1;
        wgpu::Extent3D writeSize = {finalWidth, finalHeight, uploadDepth};

        size_t dataSize = finalWidth * finalHeight * uploadDepth * finalChannels;
        context->GetWGPUDevice().GetQueue().WriteTexture(
            &destination,
            finalPixels,
            dataSize,
            &dataLayout,
            &writeSize
        );
    }
    
    // 4b. 处理纹理数组（从多个文件加载）
    if (m_loadFromFileArray && !m_filePathArray.empty())
    {
        uint32_t layerIndex = 0;
        for (const auto& filePath : m_filePathArray)
        {
            int width, height, channels;
            unsigned char* pixels = stbi_load(filePath.c_str(), &width, &height, &channels, STBI_rgb_alpha);
            
            if (!pixels)
            {
                std::cerr << "加载纹理数组层失败: " << filePath << std::endl;
                continue;
            }
            
            // 上传到特定数组层
            wgpu::TexelCopyTextureInfo destination;
            destination.texture = texture;
            destination.mipLevel = 0;
            destination.aspect = wgpu::TextureAspect::All;
            destination.origin = {0, 0, layerIndex};
            
            wgpu::TexelCopyBufferLayout dataLayout;
            dataLayout.offset = 0;
            dataLayout.bytesPerRow = width * 4;
            dataLayout.rowsPerImage = height;
            
            wgpu::Extent3D writeSize = {static_cast<uint32_t>(width), static_cast<uint32_t>(height), 1};
            
            size_t dataSize = width * height * 4;
            context->GetWGPUDevice().GetQueue().WriteTexture(
                &destination,
                pixels,
                dataSize,
                &dataLayout,
                &writeSize
            );
            
            stbi_image_free(pixels);
            layerIndex++;
            
            std::cout << "纹理数组层 " << layerIndex << " 加载成功: " << filePath << std::endl;
        }
        
        std::cout << "纹理数组加载完成，共 " << layerIndex << " 层" << std::endl;
    }

    // 5. 生成Mipmaps(如果需要)
    if (m_generateMipmaps && descriptor.mipLevelCount > 1)
    {
        // 使用GPU生成Mipmap（框架代码）
        // 注意：完整实现需要配合blit shader进行下采样
        // 当前代码仅创建mipmap级别的纹理视图框架
        wgpu::CommandEncoder encoder = context->GetWGPUDevice().CreateCommandEncoder();
        
        for (uint32_t mipLevel = 1; mipLevel < descriptor.mipLevelCount; ++mipLevel)
        {
            // 为每个mip级别创建目标纹理视图
            wgpu::TextureViewDescriptor dstViewDesc;
            dstViewDesc.baseMipLevel = mipLevel;
            dstViewDesc.mipLevelCount = 1;
            dstViewDesc.baseArrayLayer = 0;
            dstViewDesc.arrayLayerCount = 1;
            dstViewDesc.dimension = wgpu::TextureViewDimension::e2D;
            dstViewDesc.format = descriptor.format;
            wgpu::TextureView dstView = texture.CreateView(&dstViewDesc);
            
            // 创建渲染pass来清空mip级别（占位实现）
            wgpu::RenderPassColorAttachment colorAttachment;
            colorAttachment.view = dstView;
            colorAttachment.loadOp = wgpu::LoadOp::Clear;
            colorAttachment.storeOp = wgpu::StoreOp::Store;
            colorAttachment.clearValue = {0.0f, 0.0f, 0.0f, 0.0f};
            
            wgpu::RenderPassDescriptor renderPassDesc;
            renderPassDesc.colorAttachmentCount = 1;
            renderPassDesc.colorAttachments = &colorAttachment;
            
            wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&renderPassDesc);
            pass.End();
        }
        
        wgpu::CommandBuffer commands = encoder.Finish();
        context->GetWGPUDevice().GetQueue().Submit(1, &commands);
        
        std::cout << "Mipmap框架生成完成，共 " << descriptor.mipLevelCount << " 级（需配合shader实现完整功能）" << std::endl;
    }

    // 6. 清理临时数据
    if (loadedPixels)
    {
        stbi_image_free(loadedPixels);
    }

    return {texture, context};
}

TextureA::TextureA(std::nullptr_t) noexcept
{
    m_texture = nullptr;
    m_textureView = nullptr;
}

TextureA::TextureA(const wgpu::Texture& texture, const std::shared_ptr<NutContext>& context) : m_Context(context)
{
    m_texture = texture;
    if (texture)
    {
        m_textureView = m_texture.CreateView();
    }
}

wgpu::Texture TextureA::GetTexture() const
{
    return m_texture;
}

wgpu::TextureView TextureA::GetTextureView() const
{
    return m_textureView;
}

size_t TextureA::GetWidth() const
{
    if (!m_texture)
        return 0;
    return m_texture.GetWidth();
}

size_t TextureA::GetHeight() const
{
    if (!m_texture)
        return 0;
    return m_texture.GetHeight();
}

size_t TextureA::GetDepth() const
{
    if (!m_texture)
        return 0;
    return m_texture.GetDepthOrArrayLayers();
}

wgpu::TextureDimension TextureA::GetDimension() const
{
    if (!m_texture)
        return wgpu::TextureDimension::e2D;
    return m_texture.GetDimension();
}

wgpu::TextureFormat TextureA::GetFormat() const
{
    if (!m_texture)
        return wgpu::TextureFormat::Undefined;
    return m_texture.GetFormat();
}

uint32_t TextureA::GetMipLevelCount() const
{
    if (!m_texture)
        return 0;
    return m_texture.GetMipLevelCount();
}

uint32_t TextureA::GetSampleCount() const
{
    if (!m_texture)
        return 0;
    return m_texture.GetSampleCount();
}

wgpu::TextureView TextureA::CreateView(
    uint32_t baseMipLevel,
    uint32_t mipLevelCount,
    uint32_t baseArrayLayer,
    uint32_t arrayLayerCount,
    wgpu::TextureViewDimension dimension,
    wgpu::TextureAspect aspect) const
{
    if (!m_texture)
    {
        std::cerr << "TextureA::CreateView: Invalid texture" << std::endl;
        return nullptr;
    }
    
    wgpu::TextureViewDescriptor viewDesc;
    viewDesc.baseMipLevel = baseMipLevel;
    viewDesc.mipLevelCount = (mipLevelCount == 0) ? (GetMipLevelCount() - baseMipLevel) : mipLevelCount;
    viewDesc.baseArrayLayer = baseArrayLayer;
    viewDesc.arrayLayerCount = (arrayLayerCount == 0) ? (GetDepth() - baseArrayLayer) : arrayLayerCount;
    viewDesc.aspect = aspect;
    
    // 自动推断维度
    if (dimension == wgpu::TextureViewDimension::Undefined)
    {
        auto texDim = GetDimension();
        if (texDim == wgpu::TextureDimension::e1D)
        {
            viewDesc.dimension = wgpu::TextureViewDimension::e1D;
        }
        else if (texDim == wgpu::TextureDimension::e2D)
        {
            if (GetDepth() == 6)
            {
                viewDesc.dimension = wgpu::TextureViewDimension::Cube;
            }
            else if (GetDepth() > 1)
            {
                viewDesc.dimension = wgpu::TextureViewDimension::e2DArray;
            }
            else
            {
                viewDesc.dimension = wgpu::TextureViewDimension::e2D;
            }
        }
        else if (texDim == wgpu::TextureDimension::e3D)
        {
            viewDesc.dimension = wgpu::TextureViewDimension::e3D;
        }
    }
    else
    {
        viewDesc.dimension = dimension;
    }
    
    viewDesc.format = GetFormat();
    
    return m_texture.CreateView(&viewDesc);
}

bool TextureA::IsCube() const
{
    return GetDimension() == wgpu::TextureDimension::e2D && GetDepth() == 6;
}

wgpu::TextureView TextureA::GetView() const
{
    return m_textureView;
}

TextureA::operator bool() const
{
    return m_texture != nullptr;
}

/**
 * @brief 将纹理内容写入指定的 PNG 文件。
 * @param fileName 要保存的文件路径。
 * @note 此操作是同步的，会阻塞直到文件写入完成。
 * 它涉及从 GPU 到 CPU 的数据回读，可能会影响性能。
 * 纹理格式假定为 BGRA8Unorm，这是交换链的常用格式。
 */
void TextureA::WriteToFile(const std::string& fileName) const
{
    if (!m_Context || !m_texture)
    {
        // 无效的上下文或纹理，无法继续
        return;
    }

    wgpu::Device device = m_Context->GetWGPUDevice();
    wgpu::Queue queue = device.GetQueue();
    uint32_t width = m_texture.GetWidth();
    uint32_t height = m_texture.GetHeight();
    wgpu::Extent3D textureSize = {width, height, 1};

    // 1. 计算回读缓冲区所需的大小
    // WebGPU 要求缓冲区中每行数据的字节数是 256 的倍数。
    constexpr uint32_t bytesPerPixel = 4; // 对应 BGRA8Unorm
    uint32_t unpaddedBytesPerRow = width * bytesPerPixel;
    uint32_t paddedBytesPerRow = (unpaddedBytesPerRow + 255) & ~255;
    uint64_t bufferSize = paddedBytesPerRow * height;

    // 2. 创建一个 CPU 可读的缓冲区，用于接收纹理数据
    wgpu::BufferDescriptor bufferDesc;
    bufferDesc.label = "Texture Readback Buffer";
    bufferDesc.usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::MapRead;
    bufferDesc.size = bufferSize;
    wgpu::Buffer readbackBuffer = device.CreateBuffer(&bufferDesc);

    // 3. 创建命令编码器，并记录从纹理到缓冲区的复制命令
    wgpu::CommandEncoder encoder = device.CreateCommandEncoder();

    wgpu::TexelCopyTextureInfo source;
    source.texture = m_texture;
    source.mipLevel = 0;
    source.origin = {0, 0, 0};

    wgpu::TexelCopyBufferInfo destination;
    destination.buffer = readbackBuffer;
    destination.layout.offset = 0;
    destination.layout.bytesPerRow = paddedBytesPerRow;
    destination.layout.rowsPerImage = height;

    encoder.CopyTextureToBuffer(&source, &destination, &textureSize);

    // 4. 提交命令
    wgpu::CommandBuffer commands = encoder.Finish();
    device.GetQueue().Submit(1, &commands);

    // 5. 异步映射缓冲区以在 CPU 上读取数据
    // 我们使用 promise 和 future 来同步等待异步操作完成
    std::promise<bool> mapPromise;
    std::future<bool> mapFuture = mapPromise.get_future();

    auto f1 = readbackBuffer.MapAsync(
        wgpu::MapMode::Read,
        0,
        bufferSize,
        wgpu::CallbackMode::AllowSpontaneous,
        [&mapPromise](wgpu::MapAsyncStatus status, char const* message)
        {
            mapPromise.set_value(status == wgpu::MapAsyncStatus::Success);
        });

    if (wgpu::WaitStatus::Success != m_Context->GetWGPUInstance().WaitAny(f1, -1))
    {
        std::cout << "Timeout while waiting for buffer mapping." << std::endl;
        readbackBuffer.Unmap();
        return;
    }

    // 检查映射是否成功
    if (!mapFuture.get())
    {
        readbackBuffer.Unmap();
        return;
    }

    // 6. 处理数据并写入文件
    const uint8_t* mappedData = static_cast<const uint8_t*>(readbackBuffer.GetConstMappedRange());

    // stb_image_write 需要紧密打包的 RGBA 数据
    // GPU 缓冲区的数据是 BGRA 格式，并且每行末尾可能有填充
    // 因此我们需要创建一个新的 CPU 缓冲区，并手动转换和去除填充
    std::vector<uint8_t> tightRgbaData(width * height * bytesPerPixel);

    for (uint32_t y = 0; y < height; ++y)
    {
        const uint8_t* srcRow = mappedData + y * paddedBytesPerRow;
        uint8_t* dstRow = tightRgbaData.data() + y * unpaddedBytesPerRow;

        for (uint32_t x = 0; x < width; ++x)
        {
            const uint8_t* srcPixel = srcRow + x * bytesPerPixel;
            uint8_t* dstPixel = dstRow + x * bytesPerPixel;

            // 颜色通道转换 (BGRA -> RGBA)
            dstPixel[0] = srcPixel[2]; // R
            dstPixel[1] = srcPixel[1]; // G
            dstPixel[2] = srcPixel[0]; // B
            dstPixel[3] = srcPixel[3]; // A
        }
    }

    // 使用 stb_image_write 将 RGBA 数据保存为 PNG 文件
    stbi_write_png(
        fileName.c_str(),
        width,
        height,
        bytesPerPixel,
        tightRgbaData.data(),
        unpaddedBytesPerRow
    );

    // 7. 清理资源
    readbackBuffer.Unmap();
}

} // namespace Nut
