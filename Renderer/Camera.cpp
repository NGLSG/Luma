#include "Camera.h"
#include "include/core/SkCanvas.h"

SkPoint Camera::ScreenToWorld(const SkPoint& screenPoint) const
{
    const float localX = screenPoint.x() - m_properties.viewport.x();
    const float localY = screenPoint.y() - m_properties.viewport.y();


    float worldX = (localX - m_properties.viewport.width() * 0.5f) / m_properties.zoom + m_properties.position.x();
    float worldY = (localY - m_properties.viewport.height() * 0.5f) / m_properties.zoom + m_properties.position.y();
    return {worldX, worldY};
}

SkPoint Camera::WorldToScreen(const SkPoint& worldPoint) const
{
    float localX = (worldPoint.x() - m_properties.position.x()) * m_properties.zoom + m_properties.viewport.width() *
        0.5f;
    float localY = (worldPoint.y() - m_properties.position.y()) * m_properties.zoom + m_properties.viewport.height() *
        0.5f;


    float screenX = localX + m_properties.viewport.x();
    float screenY = localY + m_properties.viewport.y();
    return {screenX, screenY};
}

void Camera::SetProperties(const CamProperties& cam_properties)
{
    m_properties = cam_properties;


    if (m_properties.viewport.width() <= 0 || m_properties.viewport.height() <= 0)
    {
        m_properties.viewport = SkRect::MakeXYWH(0, 0, 1, 1);
    }
}

SkM44 Camera::BuildViewMatrix()
{
    SkM44 viewMatrix;


    viewMatrix.preTranslate(m_properties.viewport.width() * 0.5f, m_properties.viewport.height() * 0.5f);


    viewMatrix.preScale(m_properties.zoom, m_properties.zoom);


    viewMatrix.preTranslate(-m_properties.position.x(), -m_properties.position.y());

    return viewMatrix;
}

void Camera::ApplyTo(SkCanvas* canvas) const
{
    if (!canvas) return;


    canvas->translate(m_properties.viewport.width() * 0.5f, m_properties.viewport.height() * 0.5f);


    canvas->scale(m_properties.zoom, m_properties.zoom);


    canvas->translate(-m_properties.position.x(), -m_properties.position.y());
}
