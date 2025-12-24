#include "Camera.h"

#include "Cursor.h"
#include "include/core/SkCanvas.h"

Camera::Camera()
{
    m_properties = CameraProperties{};
}

SkPoint Camera::ScreenToWorld(const SkPoint& screenPoint) const
{
    std::shared_lock<std::shared_mutex> lock(m_mutex);
    const auto props = m_properties;
    const float localX = screenPoint.x() - props.viewport.x();
    const float localY = screenPoint.y() - props.viewport.y();

    SkPoint effectiveZoom = props.GetEffectiveZoom();
    float worldX = (localX - props.viewport.width() * 0.5f) / effectiveZoom.x() + props.position.x();
    float worldY = (localY - props.viewport.height() * 0.5f) / effectiveZoom.y() + props.position.y();
    return {worldX, worldY};
}

SkPoint Camera::WorldToScreen(const SkPoint& worldPoint) const
{
    std::shared_lock<std::shared_mutex> lock(m_mutex);
    const auto props = m_properties;
    SkPoint effectiveZoom = props.GetEffectiveZoom();
    float localX = (worldPoint.x() - props.position.x()) * effectiveZoom.x() + props.viewport.width() * 0.5f;
    float localY = (worldPoint.y() - props.position.y()) * effectiveZoom.y() + props.viewport.height() * 0.5f;

    float screenX = localX + props.viewport.x();
    float screenY = localY + props.viewport.y();
    return {screenX, screenY};
}

void Camera::SetProperties(const CameraProperties& properties)
{
    std::unique_lock<std::shared_mutex> lock(m_mutex);
    m_properties = properties;

    if (!std::isfinite(m_properties.position.fX) || !std::isfinite(m_properties.position.fY))
    {
        LogWarn("相机位置包含无效值 (NaN/Inf)，重置为 (0,0)");
        m_properties.position = {0.0f, 0.0f};
    }

    if (!std::isfinite(m_properties.zoom.x()) || !std::isfinite(m_properties.zoom.y()) ||
        m_properties.zoom.x() <= 1e-6f || m_properties.zoom.y() <= 1e-6f)
    {
        LogWarn("相机缩放包含无效值 (NaN/Inf/Zero/Negative)，重置为 (1,1)");
        m_properties.zoom = {1.0f, 1.0f};
    }

    if (!std::isfinite(m_properties.zoomFactor.x()) || !std::isfinite(m_properties.zoomFactor.y()) ||
        m_properties.zoomFactor.x() <= 1e-6f || m_properties.zoomFactor.y() <= 1e-6f)
    {
        LogWarn("相机缩放因子包含无效值 (NaN/Inf/Zero/Negative)，重置为 (1,1)");
        m_properties.zoomFactor = {1.0f, 1.0f};
    }

    if (m_properties.viewport.width() <= 0 || m_properties.viewport.height() <= 0)
    {
        m_properties.viewport = SkRect::MakeXYWH(0, 0, 1, 1);
    }
}

CameraProperties Camera::GetProperties() const
{
    std::shared_lock<std::shared_mutex> lock(m_mutex);
    return m_properties;
}

SkM44 Camera::BuildViewMatrix() const
{
    std::shared_lock<std::shared_mutex> lock(m_mutex);
    const auto props = m_properties;
    SkM44 viewMatrix;

    SkPoint effectiveZoom = props.GetEffectiveZoom();
    viewMatrix.preTranslate(props.viewport.width() * 0.5f, props.viewport.height() * 0.5f);
    viewMatrix.preScale(effectiveZoom.x(), effectiveZoom.y());
    viewMatrix.preTranslate(-props.position.x(), -props.position.y());

    return viewMatrix;
}

void Camera::ApplyTo(SkCanvas* canvas) const
{
    if (!canvas) return;
    std::shared_lock<std::shared_mutex> lock(m_mutex);
    const auto props = m_properties;
    SkPoint effectiveZoom = props.GetEffectiveZoom();
    canvas->translate(props.viewport.width() * 0.5f, props.viewport.height() * 0.5f);
    canvas->scale(effectiveZoom.x(), effectiveZoom.y());
    canvas->translate(-props.position.x(), -props.position.y());
}

SkM44 Camera::GetViewProjectionMatrix() const
{
    std::shared_lock<std::shared_mutex> lock(m_mutex);
    const auto props = m_properties;
    SkPoint effectiveZoom = props.GetEffectiveZoom();
    SkM44 viewProjMatrix;
    viewProjMatrix.preTranslate(props.viewport.width() * 0.5f, props.viewport.height() * 0.5f);
    viewProjMatrix.preScale(effectiveZoom.x(), effectiveZoom.y());
    viewProjMatrix.preTranslate(-props.position.x(), -props.position.y());
    return viewProjMatrix;
}

void Camera::FillEngineData(EngineData& data) const
{
    std::shared_lock<std::shared_mutex> lock(m_mutex);
    SkPoint effectiveZoom = m_properties.GetEffectiveZoom();
    data.CameraPosition = {m_properties.position.x(), m_properties.position.y()};
    data.CameraScaleX = effectiveZoom.x();
    data.CameraScaleY = effectiveZoom.y();
    data.CameraSinR = sin(m_properties.rotation);
    data.CameraCosR = cos(m_properties.rotation);
    data.ViewportSize = {m_properties.viewport.width(), m_properties.viewport.height()};
    data.MousePosition = {
        (float)LumaCursor::GetInstance().GetPosition().x, (float)LumaCursor::GetInstance().GetPosition().y
    };
}

CameraManager& CameraManager::GetInstance()
{
    static CameraManager instance;
    return instance;
}

CameraManager::CameraManager()
{
    m_cameras[DEFAULT_CAMERA_ID] = std::make_unique<Camera>();
    m_cameras[UI_CAMERA_ID] = std::make_unique<Camera>();
    m_activeCameraId = DEFAULT_CAMERA_ID;
}

bool CameraManager::CreateCamera(const std::string& id)
{
    std::unique_lock<std::shared_mutex> lock(m_managerMutex);
    if (m_cameras.find(id) != m_cameras.end())
    {
        return false;
    }
    m_cameras[id] = std::make_unique<Camera>();
    return true;
}

bool CameraManager::DestroyCamera(const std::string& id)
{
    std::unique_lock<std::shared_mutex> lock(m_managerMutex);

    if (id == m_activeCameraId)
    {
        return false;
    }

    if (id == DEFAULT_CAMERA_ID || id == UI_CAMERA_ID)
    {
        return false;
    }

    auto it = m_cameras.find(id);
    if (it == m_cameras.end())
    {
        return false;
    }

    m_cameras.erase(it);
    return true;
}

Camera* CameraManager::GetCamera(const std::string& id)
{
    std::shared_lock<std::shared_mutex> lock(m_managerMutex);
    auto it = m_cameras.find(id);
    if (it != m_cameras.end())
    {
        return it->second.get();
    }
    return nullptr;
}

Camera& CameraManager::GetActiveCamera()
{
    std::shared_lock<std::shared_mutex> lock(m_managerMutex);
    return *m_cameras.at(m_activeCameraId);
}

const Camera& CameraManager::GetActiveCamera() const
{
    std::shared_lock<std::shared_mutex> lock(m_managerMutex);
    return *m_cameras.at(m_activeCameraId);
}

bool CameraManager::SetActiveCamera(const std::string& id)
{
    std::unique_lock<std::shared_mutex> lock(m_managerMutex);
    if (m_cameras.find(id) == m_cameras.end())
    {
        return false; // ID不存在
    }
    m_activeCameraId = id;
    return true;
}

const std::string& CameraManager::GetActiveCameraId() const
{
    std::shared_lock<std::shared_mutex> lock(m_managerMutex);
    return m_activeCameraId;
}

bool CameraManager::HasCamera(const std::string& id) const
{
    std::shared_lock<std::shared_mutex> lock(m_managerMutex);
    return m_cameras.find(id) != m_cameras.end();
}

std::vector<std::string> CameraManager::GetAllCameraIds() const
{
    std::shared_lock<std::shared_mutex> lock(m_managerMutex);
    std::vector<std::string> ids;
    ids.reserve(m_cameras.size());
    for (const auto& [id, _] : m_cameras)
    {
        ids.push_back(id);
    }
    return ids;
}

Camera& CameraManager::GetUICamera()
{
    std::shared_lock<std::shared_mutex> lock(m_managerMutex);
    return *m_cameras.at(UI_CAMERA_ID);
}
