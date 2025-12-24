#include "UICamera.h"
#include "include/core/SkCanvas.h"

void UICamera::ApplyTo(SkCanvas* canvas) const
{
    if (!canvas) return;
    
    std::shared_lock<std::shared_mutex> lock(m_mutex);
    const auto props = m_properties;

    if (props.scale.x() != 1.0f || props.scale.y() != 1.0f)
    {
        canvas->scale(props.scale.x(), props.scale.y());
    }
}

SkPoint UICamera::ScreenToUI(const SkPoint& screenPoint) const
{
    std::shared_lock<std::shared_mutex> lock(m_mutex);
    const auto props = m_properties;

    float uiX = (screenPoint.x() - props.viewport.x()) / props.scale.x();
    float uiY = (screenPoint.y() - props.viewport.y()) / props.scale.y();
    
    return {uiX, uiY};
}

SkPoint UICamera::UIToScreen(const SkPoint& uiPoint) const
{
    std::shared_lock<std::shared_mutex> lock(m_mutex);
    const auto props = m_properties;

    float screenX = uiPoint.x() * props.scale.x() + props.viewport.x();
    float screenY = uiPoint.y() * props.scale.y() + props.viewport.y();
    
    return {screenX, screenY};
}

void UICamera::SetProperties(const UICamProperties& properties)
{
    std::unique_lock<std::shared_mutex> lock(m_mutex);
    m_properties = properties;

    if (!std::isfinite(m_properties.scale.x()) || !std::isfinite(m_properties.scale.y()) ||
        m_properties.scale.x() <= 1e-6f || m_properties.scale.y() <= 1e-6f)
    {
        m_properties.scale = {1.0f, 1.0f};
    }

    if (m_properties.viewport.width() <= 0 || m_properties.viewport.height() <= 0)
    {
        m_properties.viewport = SkRect::MakeXYWH(0, 0, 1, 1);
    }
}

UICamera::UICamProperties UICamera::GetProperties() const
{
    std::shared_lock<std::shared_mutex> lock(m_mutex);
    return m_properties;
}

void UICamera::SetViewport(const SkRect& viewport)
{
    std::unique_lock<std::shared_mutex> lock(m_mutex);
    if (viewport.width() > 0 && viewport.height() > 0)
    {
        m_properties.viewport = viewport;
    }
}

void UICamera::SetScale(const SkPoint& scale)
{
    std::unique_lock<std::shared_mutex> lock(m_mutex);
    if (std::isfinite(scale.x()) && std::isfinite(scale.y()) &&
        scale.x() > 1e-6f && scale.y() > 1e-6f)
    {
        m_properties.scale = scale;
    }
}

SkM44 UICamera::BuildViewMatrix() const
{
    std::shared_lock<std::shared_mutex> lock(m_mutex);
    const auto props = m_properties;
    
    SkM44 viewMatrix;
    viewMatrix.preScale(props.scale.x(), props.scale.y());
    
    return viewMatrix;
}
