#include "RenderTarget.h"

wgpu::Texture RenderTarget::GetTexture() const
{
    return m_texture.GetTexture();
}

const Nut::TextureA& RenderTarget::GetTextureA() const
{
    return m_texture;
}

uint16_t RenderTarget::GetWidth() const
{
    return m_width;
}

uint16_t RenderTarget::GetHeight() const
{
    return m_height;
}

RenderTarget::RenderTarget(wgpu::Texture texture, uint16_t width, uint16_t height) 
    : m_texture(nullptr), m_width(width), m_height(height)
{
    // 注意：这个构造函数不再直接支持 wgpu::Texture
    // 应该使用 RenderTarget(Nut::TextureA, width, height) 构造函数
}

RenderTarget::RenderTarget(Nut::TextureA texture, uint16_t width, uint16_t height)
    : m_texture(texture), m_width(width), m_height(height)
{
}
