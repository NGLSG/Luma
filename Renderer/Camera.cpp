#include "Camera.h"
#include "include/core/SkCanvas.h"
#include "fstream"

SkPoint Camera::ScreenToWorld(const SkPoint& screenPoint) const
{
    std::shared_lock<std::shared_mutex> lock(m_mutex);
    const auto props = m_properties;
    const float localX = screenPoint.x() - props.viewport.x();
    const float localY = screenPoint.y() - props.viewport.y();


    float worldX = (localX - props.viewport.width() * 0.5f) / props.zoom + props.position.x();
    float worldY = (localY - props.viewport.height() * 0.5f) / props.zoom + props.position.y();
    return {worldX, worldY};
}

SkPoint Camera::WorldToScreen(const SkPoint& worldPoint) const
{
    std::shared_lock<std::shared_mutex> lock(m_mutex);
    const auto props = m_properties;
    float localX = (worldPoint.x() - props.position.x()) * props.zoom + props.viewport.width() * 0.5f;
    float localY = (worldPoint.y() - props.position.y()) * props.zoom + props.viewport.height() * 0.5f;


    float screenX = localX + props.viewport.x();
    float screenY = localY + props.viewport.y();
    return {screenX, screenY};
}

void Camera::SetProperties(const CamProperties& cam_properties)
{
    std::unique_lock<std::shared_mutex> lock(m_mutex);
    m_properties = cam_properties;

    if (!std::isfinite(m_properties.position.fX) || !std::isfinite(m_properties.position.fY))
    {
        LogWarn("相机位置包含无效值 (NaN/Inf)，重置为 (0,0)");
        m_properties.position = {0.0f, 0.0f};
    }

    if (!std::isfinite(m_properties.zoom) || m_properties.zoom <= 1e-6f)
    {
        LogWarn("相机缩放包含无效值 (NaN/Inf/Zero/Negative)，重置为 1.0");
        m_properties.zoom = 1.0f;
    }

    if (m_properties.viewport.width() <= 0 || m_properties.viewport.height() <= 0)
    {
        m_properties.viewport = SkRect::MakeXYWH(0, 0, 1, 1);
    }
}

SkM44 Camera::BuildViewMatrix()
{
    std::shared_lock<std::shared_mutex> lock(m_mutex);
    const auto props = m_properties;
    SkM44 viewMatrix;


    viewMatrix.preTranslate(props.viewport.width() * 0.5f, props.viewport.height() * 0.5f);


    viewMatrix.preScale(props.zoom, props.zoom);


    viewMatrix.preTranslate(-props.position.x(), -props.position.y());

    return viewMatrix;
}

void Camera::ApplyTo(SkCanvas* canvas) const
{
    if (!canvas) return;
    std::shared_lock<std::shared_mutex> lock(m_mutex);
    const auto props = m_properties;
    canvas->translate(props.viewport.width() * 0.5f, props.viewport.height() * 0.5f);
    canvas->scale(props.zoom, props.zoom);
    canvas->translate(-props.position.x(), -props.position.y());
}

Camera::CamProperties Camera::GetProperties() const
{
    std::shared_lock<std::shared_mutex> lock(m_mutex);
    return m_properties;
}
