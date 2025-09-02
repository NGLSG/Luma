#include "Cursor.h"

void LumaCursor::Update()
{
    m_delta = {0, 0};
    m_scrollDelta = {0.0f, 0.0f};


    for (auto& button : m_buttons)
    {
        button.wasDown = button.isDown;
    }
}

void LumaCursor::ProcessEvent(const SDL_Event& event)
{
    switch (event.type)
    {
    case SDL_EVENT_MOUSE_MOTION:
        m_position = {event.motion.x, event.motion.y};
        m_delta = {event.motion.xrel, event.motion.yrel};
        break;

    case SDL_EVENT_MOUSE_BUTTON_DOWN:
        {
            Uint8 buttonIndex = event.button.button;
            if (buttonIndex < m_buttons.size())
            {
                m_buttons[buttonIndex].isDown = true;
                m_buttons[buttonIndex].clickPosition = {event.button.x, event.button.y};
            }
            break;
        }

    case SDL_EVENT_MOUSE_BUTTON_UP:
        {
            Uint8 buttonIndex = event.button.button;
            if (buttonIndex < m_buttons.size())
            {
                m_buttons[buttonIndex].isDown = false;
            }
            break;
        }

    case SDL_EVENT_MOUSE_WHEEL:

        m_scrollDelta = {event.wheel.x, event.wheel.y};
        break;
    }
}


ECS::Vector2i LumaCursor::GetPosition() { return GetInstance().m_position; }
ECS::Vector2i LumaCursor::GetDelta() { return GetInstance().m_delta; }
ECS::Vector2f LumaCursor::GetScrollDelta() { return GetInstance().m_scrollDelta; }


const MouseButton& LumaCursor::Left = LumaCursor::GetInstance().m_buttons[SDL_BUTTON_LEFT];
const MouseButton& LumaCursor::Right = LumaCursor::GetInstance().m_buttons[SDL_BUTTON_RIGHT];
const MouseButton& LumaCursor::Middle = LumaCursor::GetInstance().m_buttons[SDL_BUTTON_MIDDLE];
