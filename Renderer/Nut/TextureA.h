#ifndef LUMAENGINE_TEXTUREA_H
#define LUMAENGINE_TEXTUREA_H
#include <string>
#include <memory>

#include "dawn/webgpu_cpp.h"

namespace Nut
{
    class NutContext;
    class TextureA;
    using TextureAPtr = std::shared_ptr<TextureA>;

    /**
     * @brief 纹理类 - 支持1D/2D/3D纹理、纹理数组、Cube纹理等
     */
    class LUMA_API TextureA
    {
        wgpu::Texture m_texture;
        wgpu::TextureView m_textureView;
        std::shared_ptr<NutContext> m_Context;

    public:
        TextureA(std::nullptr_t) noexcept;

        TextureA()
        {
            m_texture = nullptr;
            m_textureView = nullptr;
        }

        TextureA(const wgpu::Texture& texture,
                 const std::shared_ptr<NutContext>& context);

        static TextureAPtr CreateTextureA(const wgpu::Texture& texture,
                                          const std::shared_ptr<NutContext>& context)
        {
            return std::make_shared<Nut::TextureA>(texture, context);
        }

        // 基础访问
        wgpu::Texture GetTexture() const;
        wgpu::TextureView GetTextureView() const;
        wgpu::TextureView GetView() const;
        operator bool() const;

        // === 这里是你要的隐式转换 ===
        operator const wgpu::Texture&() const { return m_texture; }
        operator const wgpu::TextureView&() const { return m_textureView; }

        // 如果你希望能修改内部对象，也可以再加非 const 版本：
        operator wgpu::Texture&() { return m_texture; }
        operator wgpu::TextureView&() { return m_textureView; }

        // 尺寸信息（支持3D纹理）
        size_t GetWidth() const;
        size_t GetHeight() const;
        size_t GetDepth() const; // 获取深度（3D纹理）或数组层数

        // 纹理属性
        wgpu::TextureDimension GetDimension() const;
        wgpu::TextureFormat GetFormat() const;
        uint32_t GetMipLevelCount() const;
        uint32_t GetSampleCount() const;

        // 高级视图创建（用于纹理数组、Cube纹理、特定Mip级别）
        wgpu::TextureView CreateView(
            uint32_t baseMipLevel = 0,
            uint32_t mipLevelCount = 0, // 0 = all remaining
            uint32_t baseArrayLayer = 0,
            uint32_t arrayLayerCount = 0, // 0 = all remaining
            wgpu::TextureViewDimension dimension = wgpu::TextureViewDimension::Undefined,
            wgpu::TextureAspect aspect = wgpu::TextureAspect::All
        ) const;

        // 便捷方法
        bool Is3D() const { return GetDimension() == wgpu::TextureDimension::e3D; }
        bool IsCube() const;
        bool IsArray() const { return GetDepth() > 1 && !Is3D(); }

        // 文件操作（仅2D纹理）
        void WriteToFile(const std::string& fileName) const;
    };


    // 纹理使用标志位辅助类
    class TextureUsageFlags
    {
    public:
        TextureUsageFlags() : m_usage(wgpu::TextureUsage::None)
        {
        }

        TextureUsageFlags& AddTextureBinding()
        {
            m_usage |= wgpu::TextureUsage::TextureBinding;
            return *this;
        }

        TextureUsageFlags& AddCopyDst()
        {
            m_usage |= wgpu::TextureUsage::CopyDst;
            return *this;
        }

        TextureUsageFlags& AddCopySrc()
        {
            m_usage |= wgpu::TextureUsage::CopySrc;
            return *this;
        }

        TextureUsageFlags& AddRenderAttachment()
        {
            m_usage |= wgpu::TextureUsage::RenderAttachment;
            return *this;
        }

        TextureUsageFlags& AddStorageBinding()
        {
            m_usage |= wgpu::TextureUsage::StorageBinding;
            return *this;
        }

        wgpu::TextureUsage GetUsage() const { return m_usage; }

        operator wgpu::TextureUsage() const { return m_usage; }

        // 常用组合
        static TextureUsageFlags GetCommonTextureUsage()
        {
            TextureUsageFlags flags;
            flags.AddTextureBinding().AddCopyDst().AddCopySrc();
            return flags;
        }

        static TextureUsageFlags GetRenderTargetUsage()
        {
            TextureUsageFlags flags;
            flags.AddRenderAttachment().AddTextureBinding().AddCopySrc();
            return flags;
        }

    private:
        wgpu::TextureUsage m_usage;
    };

    // 纹理描述符构建器
    class TextureDescriptor
    {
    public:
        TextureDescriptor()
        {
            // 默认配置
            m_descriptor.dimension = wgpu::TextureDimension::e2D;
            m_descriptor.format = wgpu::TextureFormat::RGBA8Unorm;
            m_descriptor.mipLevelCount = 1;
            m_descriptor.sampleCount = 1;
            m_descriptor.size = {1, 1, 1};
            m_descriptor.usage = TextureUsageFlags::GetCommonTextureUsage().GetUsage();
        }

        // 设置纹理尺寸
        TextureDescriptor& SetSize(uint32_t width, uint32_t height, uint32_t depth = 1)
        {
            m_descriptor.size = {width, height, depth};
            return *this;
        }

        // 设置纹理格式
        TextureDescriptor& SetFormat(wgpu::TextureFormat format)
        {
            m_descriptor.format = format;
            return *this;
        }

        // 设置维度
        TextureDescriptor& SetDimension(wgpu::TextureDimension dimension)
        {
            m_descriptor.dimension = dimension;
            return *this;
        }

        // 设置Mip级别数量
        TextureDescriptor& SetMipLevelCount(uint32_t count)
        {
            m_descriptor.mipLevelCount = count;
            return *this;
        }

        // 设置采样数量(用于MSAA)
        TextureDescriptor& SetSampleCount(uint32_t count)
        {
            m_descriptor.sampleCount = count;
            return *this;
        }

        // 设置使用标志
        TextureDescriptor& SetUsage(wgpu::TextureUsage usage)
        {
            m_descriptor.usage = usage;
            return *this;
        }

        // 设置使用标志(通过辅助类)
        TextureDescriptor& SetUsage(const TextureUsageFlags& flags)
        {
            m_descriptor.usage = flags.GetUsage();
            return *this;
        }

        // 设置标签
        TextureDescriptor& SetLabel(const std::string& label)
        {
            m_label = label;
            m_descriptor.label = m_label.c_str();
            return *this;
        }

        // 获取描述符
        const wgpu::TextureDescriptor& GetDescriptor() const
        {
            return m_descriptor;
        }

    private:
        wgpu::TextureDescriptor m_descriptor;
        std::string m_label; // 需要保持字符串生命周期
    };

    // 纹理数据上传配置
    struct TextureUploadConfig
    {
        uint32_t mipLevel = 0;
        wgpu::TextureAspect aspect = wgpu::TextureAspect::All;
        uint32_t originX = 0;
        uint32_t originY = 0;
        uint32_t originZ = 0;
        uint32_t bytesPerRow = 0; // 0表示自动计算
        uint32_t rowsPerImage = 0; // 0表示自动计算
    };

    // 纹理构建器
    class LUMA_API TextureBuilder
    {
    public:
        TextureBuilder() = default;

        // 从文件加载
        TextureBuilder& LoadFromFile(const std::string& filePath)
        {
            m_loadFromFile = true;
            m_filePath = filePath;
            return *this;
        }

        // 从内存加载
        TextureBuilder& LoadFromMemory(const void* data, size_t size)
        {
            m_loadFromMemory = true;
            m_memoryData = data;
            m_memorySize = size;
            return *this;
        }

        // 设置原始像素数据（2D）
        TextureBuilder& SetPixelData(const void* pixels, uint32_t width, uint32_t height, uint32_t channels)
        {
            m_hasPixelData = true;
            m_pixelData = pixels;
            m_width = width;
            m_height = height;
            m_depth = 1;
            m_channels = channels;
            return *this;
        }

        // 设置3D纹理数据
        TextureBuilder& SetPixelData3D(const void* pixels, uint32_t width, uint32_t height, uint32_t depth,
                                       uint32_t channels)
        {
            m_hasPixelData = true;
            m_pixelData = pixels;
            m_width = width;
            m_height = height;
            m_depth = depth;
            m_channels = channels;
            m_descriptorBuilder.SetDimension(wgpu::TextureDimension::e3D);
            m_descriptorBuilder.SetSize(width, height, depth);
            return *this;
        }

        // 从多个文件加载纹理数组
        TextureBuilder& LoadFromFiles(const std::vector<std::string>& filePaths)
        {
            m_loadFromFileArray = true;
            m_filePathArray = filePaths;
            return *this;
        }

        // 设置纹理描述符
        TextureBuilder& SetDescriptor(const TextureDescriptor& descriptorBuilder)
        {
            m_descriptorBuilder = descriptorBuilder;
            return *this;
        }

        // 快捷设置尺寸（2D）
        TextureBuilder& SetSize(uint32_t width, uint32_t height)
        {
            m_descriptorBuilder.SetSize(width, height);
            return *this;
        }

        // 快捷设置尺寸（3D）
        TextureBuilder& SetSize3D(uint32_t width, uint32_t height, uint32_t depth)
        {
            m_descriptorBuilder.SetSize(width, height, depth);
            m_descriptorBuilder.SetDimension(wgpu::TextureDimension::e3D);
            return *this;
        }

        // 快捷设置维度
        TextureBuilder& SetDimension(wgpu::TextureDimension dimension)
        {
            m_descriptorBuilder.SetDimension(dimension);
            return *this;
        }

        // 创建Cube纹理
        TextureBuilder& SetupCube(uint32_t size)
        {
            m_descriptorBuilder.SetSize(size, size, 6);
            m_descriptorBuilder.SetDimension(wgpu::TextureDimension::e2D);
            return *this;
        }

        // 快捷设置格式
        TextureBuilder& SetFormat(wgpu::TextureFormat format)
        {
            m_descriptorBuilder.SetFormat(format);
            return *this;
        }

        // 快捷设置使用标志
        TextureBuilder& SetUsage(wgpu::TextureUsage usage)
        {
            m_descriptorBuilder.SetUsage(usage);
            return *this;
        }

        // 设置上传配置
        TextureBuilder& SetUploadConfig(const TextureUploadConfig& config)
        {
            m_uploadConfig = config;
            return *this;
        }

        // 是否生成Mipmaps
        TextureBuilder& GenerateMipmaps(bool generate = true)
        {
            m_generateMipmaps = generate;
            return *this;
        }

        // 构建纹理
        TextureAPtr Build(std::shared_ptr<NutContext> context);

    private:
        TextureDescriptor m_descriptorBuilder;
        TextureUploadConfig m_uploadConfig;

        bool m_loadFromFile = false;
        std::string m_filePath;

        bool m_loadFromMemory = false;
        const void* m_memoryData = nullptr;
        size_t m_memorySize = 0;

        bool m_hasPixelData = false;
        const void* m_pixelData = nullptr;
        uint32_t m_width = 0;
        uint32_t m_height = 0;
        uint32_t m_depth = 1;
        uint32_t m_channels = 0;

        bool m_loadFromFileArray = false;
        std::vector<std::string> m_filePathArray;

        bool m_generateMipmaps = false;
    };

    struct Size
    {
        uint32_t width;
        uint32_t height;
    };
} // namespace Nut

#endif //LUMAENGINE_TEXTUREA_H
