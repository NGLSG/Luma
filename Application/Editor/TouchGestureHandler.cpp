#include "TouchGestureHandler.h"
#include <cmath>
#include <algorithm>

void TouchGestureHandler::OnTouchDown(SDL_FingerID fingerId, float x, float y, float pressure)
{
    TouchPoint& point = m_touches[fingerId];
    point.x = x;
    point.y = y;
    point.startX = x;
    point.startY = y;
    point.pressure = pressure;
    point.active = true;
    
    int touchCount = GetActiveTouchCount();
    
    if (touchCount == 1)
    {
        m_touchDownTime = 0.0f;
        m_touchDownPos = NormalizedToScreen(x, y);
        m_touchMoved = false;
        m_state.isSingleTouch = true;
        m_state.singleTouchPos = m_touchDownPos;
    }
    else if (touchCount == 2)
    {
        // 双指按下 - 初始化双指手势
        m_state.isSingleTouch = false;
        m_state.isSingleDragging = false;
        m_state.isTwoFingerTouch = true;
        
        // 计算初始双指距离和中心
        std::vector<TouchPoint*> activePoints;
        for (auto& [id, pt] : m_touches)
        {
            if (pt.active) activePoints.push_back(&pt);
        }
        
        if (activePoints.size() >= 2)
        {
            glm::vec2 p1 = NormalizedToScreen(activePoints[0]->x, activePoints[0]->y);
            glm::vec2 p2 = NormalizedToScreen(activePoints[1]->x, activePoints[1]->y);
            m_lastPinchDistance = glm::distance(p1, p2);
            m_lastTwoFingerCenter = (p1 + p2) * 0.5f;
        }
    }
}

void TouchGestureHandler::OnTouchMove(SDL_FingerID fingerId, float x, float y, float dx, float dy, float pressure)
{
    auto it = m_touches.find(fingerId);
    if (it == m_touches.end() || !it->second.active)
        return;
    
    TouchPoint& point = it->second;
    point.x = x;
    point.y = y;
    point.pressure = pressure;
    
    int touchCount = GetActiveTouchCount();
    glm::vec2 screenPos = NormalizedToScreen(x, y);
    glm::vec2 screenDelta = glm::vec2(dx * m_screenWidth, dy * m_screenHeight);
    
    if (touchCount == 1 && m_state.isSingleTouch)
    {
        // 单指移动
        float distFromStart = glm::distance(screenPos, m_touchDownPos);
        
        if (!m_state.isSingleDragging && distFromStart > m_dragThreshold)
        {
            m_state.isSingleDragging = true;
            m_touchMoved = true;
            if (m_onDragStart)
            {
                m_onDragStart(screenPos.x, screenPos.y);
            }
        }
        
        if (m_state.isSingleDragging)
        {
            m_state.singleTouchPos = screenPos;
            m_state.singleTouchDelta = screenDelta;
            if (m_onDrag)
            {
                m_onDrag(screenPos.x, screenPos.y, screenDelta.x, screenDelta.y);
            }
        }
    }
    else if (touchCount >= 2 && m_state.isTwoFingerTouch)
    {
        std::vector<TouchPoint*> activePoints;
        for (auto& [id, pt] : m_touches)
        {
            if (pt.active) activePoints.push_back(&pt);
        }
        
        if (activePoints.size() >= 2)
        {
            glm::vec2 p1 = NormalizedToScreen(activePoints[0]->x, activePoints[0]->y);
            glm::vec2 p2 = NormalizedToScreen(activePoints[1]->x, activePoints[1]->y);
            
            float currentDistance = glm::distance(p1, p2);
            glm::vec2 currentCenter = (p1 + p2) * 0.5f;

            if (m_lastPinchDistance > 0.0f)
            {
                float scale = currentDistance / m_lastPinchDistance;
                if (std::abs(scale - 1.0f) > 0.001f)
                {
                    m_state.isPinching = true;
                    m_state.pinchScale = scale;
                    m_state.pinchCenter = currentCenter;
                    if (m_onZoom)
                    {
                        m_onZoom(scale, currentCenter.x, currentCenter.y);
                    }
                }
            }

            glm::vec2 panDelta = currentCenter - m_lastTwoFingerCenter;
            if (glm::length(panDelta) > 0.5f)
            {
                m_state.isPanning = true;
                m_state.panDelta = panDelta;
                if (m_onPan)
                {
                    m_onPan(panDelta.x, panDelta.y);
                }
            }
            
            m_lastPinchDistance = currentDistance;
            m_lastTwoFingerCenter = currentCenter;
        }
    }
}

void TouchGestureHandler::OnTouchUp(SDL_FingerID fingerId, float x, float y)
{
    auto it = m_touches.find(fingerId);
    if (it == m_touches.end())
        return;
    
    TouchPoint& point = it->second;
    glm::vec2 screenPos = NormalizedToScreen(x, y);
    
    if (m_state.isSingleTouch && !m_touchMoved && m_touchDownTime < m_tapTimeThreshold)
    {
        // 识别为点击
        float distFromStart = glm::distance(screenPos, m_touchDownPos);
        if (distFromStart < m_tapThreshold)
        {
            m_state.justTapped = true;
            m_state.tapPosition = screenPos;
            if (m_onTap)
            {
                m_onTap(screenPos.x, screenPos.y);
            }
        }
    }
    
    if (m_state.isSingleDragging)
    {
        if (m_onDragEnd)
        {
            m_onDragEnd(screenPos.x, screenPos.y);
        }
    }
    
    point.active = false;
    m_touches.erase(it);

    int touchCount = GetActiveTouchCount();
    if (touchCount == 0)
    {
        m_state.isSingleTouch = false;
        m_state.isSingleDragging = false;
        m_state.isTwoFingerTouch = false;
        m_state.isPanning = false;
        m_state.isPinching = false;
    }
    else if (touchCount == 1)
    {
        m_state.isTwoFingerTouch = false;
        m_state.isPanning = false;
        m_state.isPinching = false;
        for (auto& [id, pt] : m_touches)
        {
            if (pt.active)
            {
                m_state.isSingleTouch = true;
                m_state.singleTouchPos = NormalizedToScreen(pt.x, pt.y);
                break;
            }
        }
    }
}

void TouchGestureHandler::Update(float deltaTime)
{
    m_touchDownTime += deltaTime;

    m_state.justTapped = false;
    m_state.singleTouchDelta = glm::vec2(0.0f);
    m_state.panDelta = glm::vec2(0.0f);
    m_state.pinchScale = 1.0f;
}

void TouchGestureHandler::Reset()
{
    m_touches.clear();
    m_state = GestureState();
    m_lastPinchDistance = 0.0f;
    m_lastTwoFingerCenter = glm::vec2(0.0f);
    m_touchDownTime = 0.0f;
    m_touchDownPos = glm::vec2(0.0f);
    m_touchMoved = false;
}

void TouchGestureHandler::SetScreenSize(float width, float height)
{
    m_screenWidth = width;
    m_screenHeight = height;
}

int TouchGestureHandler::GetActiveTouchCount() const
{
    int count = 0;
    for (const auto& [id, pt] : m_touches)
    {
        if (pt.active) count++;
    }
    return count;
}

glm::vec2 TouchGestureHandler::NormalizedToScreen(float nx, float ny) const
{
    return glm::vec2(nx * m_screenWidth, ny * m_screenHeight);
}
