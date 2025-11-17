#include "RenderTarget.h"

wgpu::Texture RenderTarget::GetTexture() const
{
    return m_texture;
}

wgpu::TextureView RenderTarget::GetView() const
{
    return m_texture.CreateView();
}

uint16_t RenderTarget::GetWidth() const
{
    return m_width;
}

uint16_t RenderTarget::GetHeight() const
{
    return m_height;
}

RenderTarget::RenderTarget(wgpu::Texture texture, uint16_t width, uint16_t height) : m_texture(texture), m_width(width),
    m_height(height)
{
}
