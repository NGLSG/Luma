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
        finalChannels = 4; 
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

    
    if (m_loadFromFile || m_loadFromMemory)
    {
        m_descriptorBuilder.SetSize(finalWidth, finalHeight);
    }

    
    const auto& descriptor = m_descriptorBuilder.GetDescriptor();
    auto texture = context->GetWGPUDevice().CreateTexture(&descriptor);

    if (!texture)
    {
        std::cerr << "创建纹理失败" << std::endl;
        if (loadedPixels)
            stbi_image_free(loadedPixels);
        return nullptr;
    }

    
    if (finalPixels)
    {
        wgpu::TexelCopyTextureInfo destination;
        destination.texture = texture;
        destination.mipLevel = m_uploadConfig.mipLevel;
        destination.aspect = m_uploadConfig.aspect;
        destination.origin = {m_uploadConfig.originX, m_uploadConfig.originY, m_uploadConfig.originZ};

        wgpu::TexelCopyBufferLayout dataLayout;
        dataLayout.offset = 0;

        
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

    
    if (m_generateMipmaps && descriptor.mipLevelCount > 1)
    {
        
        
        
        wgpu::CommandEncoder encoder = context->GetWGPUDevice().CreateCommandEncoder();
        
        for (uint32_t mipLevel = 1; mipLevel < descriptor.mipLevelCount; ++mipLevel)
        {
            
            wgpu::TextureViewDescriptor dstViewDesc;
            dstViewDesc.baseMipLevel = mipLevel;
            dstViewDesc.mipLevelCount = 1;
            dstViewDesc.baseArrayLayer = 0;
            dstViewDesc.arrayLayerCount = 1;
            dstViewDesc.dimension = wgpu::TextureViewDimension::e2D;
            dstViewDesc.format = descriptor.format;
            wgpu::TextureView dstView = texture.CreateView(&dstViewDesc);
            
            
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


void TextureA::WriteToFile(const std::string& fileName) const
{
    if (!m_Context || !m_texture)
    {
        
        return;
    }

    wgpu::Device device = m_Context->GetWGPUDevice();
    wgpu::Queue queue = device.GetQueue();
    uint32_t width = m_texture.GetWidth();
    uint32_t height = m_texture.GetHeight();
    wgpu::Extent3D textureSize = {width, height, 1};

    
    
    constexpr uint32_t bytesPerPixel = 4; 
    uint32_t unpaddedBytesPerRow = width * bytesPerPixel;
    uint32_t paddedBytesPerRow = (unpaddedBytesPerRow + 255) & ~255;
    uint64_t bufferSize = paddedBytesPerRow * height;

    
    wgpu::BufferDescriptor bufferDesc;
    bufferDesc.label = "Texture Readback Buffer";
    bufferDesc.usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::MapRead;
    bufferDesc.size = bufferSize;
    wgpu::Buffer readbackBuffer = device.CreateBuffer(&bufferDesc);

    
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

    
    wgpu::CommandBuffer commands = encoder.Finish();
    device.GetQueue().Submit(1, &commands);

    
    
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

    
    if (!mapFuture.get())
    {
        readbackBuffer.Unmap();
        return;
    }

    
    const uint8_t* mappedData = static_cast<const uint8_t*>(readbackBuffer.GetConstMappedRange());

    
    
    
    std::vector<uint8_t> tightRgbaData(width * height * bytesPerPixel);

    for (uint32_t y = 0; y < height; ++y)
    {
        const uint8_t* srcRow = mappedData + y * paddedBytesPerRow;
        uint8_t* dstRow = tightRgbaData.data() + y * unpaddedBytesPerRow;

        for (uint32_t x = 0; x < width; ++x)
        {
            const uint8_t* srcPixel = srcRow + x * bytesPerPixel;
            uint8_t* dstPixel = dstRow + x * bytesPerPixel;

            
            dstPixel[0] = srcPixel[2]; 
            dstPixel[1] = srcPixel[1]; 
            dstPixel[2] = srcPixel[0]; 
            dstPixel[3] = srcPixel[3]; 
        }
    }

    
    stbi_write_png(
        fileName.c_str(),
        width,
        height,
        bytesPerPixel,
        tightRgbaData.data(),
        unpaddedBytesPerRow
    );

    
    readbackBuffer.Unmap();
}

} 
