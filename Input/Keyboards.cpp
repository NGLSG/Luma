#include "Keyboards.h"

void Keyboard::Update()
{
    for (auto& key : m_keys)
    {
        key.wasDown = key.isDown;
    }
}

void Keyboard::ProcessEvent(const SDL_Event& event)
{
    if (event.type == SDL_EVENT_KEY_DOWN)
    {
        if (event.key.scancode < SDL_SCANCODE_COUNT)
        {
            m_keys[event.key.scancode].isDown = true;
        }
    }
    else if (event.type == SDL_EVENT_KEY_UP)
    {
        if (event.key.scancode < SDL_SCANCODE_COUNT)
        {
            m_keys[event.key.scancode].isDown = false;
        }
    }
}


const Key& Keyboard::A = Keyboard::GetInstance().m_keys[SDL_SCANCODE_A];
const Key& Keyboard::B = Keyboard::GetInstance().m_keys[SDL_SCANCODE_B];
const Key& Keyboard::C = Keyboard::GetInstance().m_keys[SDL_SCANCODE_C];
const Key& Keyboard::D = Keyboard::GetInstance().m_keys[SDL_SCANCODE_D];
const Key& Keyboard::E = Keyboard::GetInstance().m_keys[SDL_SCANCODE_E];
const Key& Keyboard::F = Keyboard::GetInstance().m_keys[SDL_SCANCODE_F];
const Key& Keyboard::G = Keyboard::GetInstance().m_keys[SDL_SCANCODE_G];
const Key& Keyboard::H = Keyboard::GetInstance().m_keys[SDL_SCANCODE_H];
const Key& Keyboard::I = Keyboard::GetInstance().m_keys[SDL_SCANCODE_I];
const Key& Keyboard::J = Keyboard::GetInstance().m_keys[SDL_SCANCODE_J];
const Key& Keyboard::K = Keyboard::GetInstance().m_keys[SDL_SCANCODE_K];
const Key& Keyboard::L = Keyboard::GetInstance().m_keys[SDL_SCANCODE_L];
const Key& Keyboard::M = Keyboard::GetInstance().m_keys[SDL_SCANCODE_M];
const Key& Keyboard::N = Keyboard::GetInstance().m_keys[SDL_SCANCODE_N];
const Key& Keyboard::O = Keyboard::GetInstance().m_keys[SDL_SCANCODE_O];
const Key& Keyboard::P = Keyboard::GetInstance().m_keys[SDL_SCANCODE_P];
const Key& Keyboard::Q = Keyboard::GetInstance().m_keys[SDL_SCANCODE_Q];
const Key& Keyboard::R = Keyboard::GetInstance().m_keys[SDL_SCANCODE_R];
const Key& Keyboard::S = Keyboard::GetInstance().m_keys[SDL_SCANCODE_S];
const Key& Keyboard::T = Keyboard::GetInstance().m_keys[SDL_SCANCODE_T];
const Key& Keyboard::U = Keyboard::GetInstance().m_keys[SDL_SCANCODE_U];
const Key& Keyboard::V = Keyboard::GetInstance().m_keys[SDL_SCANCODE_V];
const Key& Keyboard::W = Keyboard::GetInstance().m_keys[SDL_SCANCODE_W];
const Key& Keyboard::X = Keyboard::GetInstance().m_keys[SDL_SCANCODE_X];
const Key& Keyboard::Y = Keyboard::GetInstance().m_keys[SDL_SCANCODE_Y];
const Key& Keyboard::Z = Keyboard::GetInstance().m_keys[SDL_SCANCODE_Z];


const Key& Keyboard::Num0 = Keyboard::GetInstance().m_keys[SDL_SCANCODE_0];
const Key& Keyboard::Num1 = Keyboard::GetInstance().m_keys[SDL_SCANCODE_1];
const Key& Keyboard::Num2 = Keyboard::GetInstance().m_keys[SDL_SCANCODE_2];
const Key& Keyboard::Num3 = Keyboard::GetInstance().m_keys[SDL_SCANCODE_3];
const Key& Keyboard::Num4 = Keyboard::GetInstance().m_keys[SDL_SCANCODE_4];
const Key& Keyboard::Num5 = Keyboard::GetInstance().m_keys[SDL_SCANCODE_5];
const Key& Keyboard::Num6 = Keyboard::GetInstance().m_keys[SDL_SCANCODE_6];
const Key& Keyboard::Num7 = Keyboard::GetInstance().m_keys[SDL_SCANCODE_7];
const Key& Keyboard::Num8 = Keyboard::GetInstance().m_keys[SDL_SCANCODE_8];
const Key& Keyboard::Num9 = Keyboard::GetInstance().m_keys[SDL_SCANCODE_9];


const Key& Keyboard::F1 = Keyboard::GetInstance().m_keys[SDL_SCANCODE_F1];
const Key& Keyboard::F2 = Keyboard::GetInstance().m_keys[SDL_SCANCODE_F2];
const Key& Keyboard::F3 = Keyboard::GetInstance().m_keys[SDL_SCANCODE_F3];
const Key& Keyboard::F4 = Keyboard::GetInstance().m_keys[SDL_SCANCODE_F4];
const Key& Keyboard::F5 = Keyboard::GetInstance().m_keys[SDL_SCANCODE_F5];
const Key& Keyboard::F6 = Keyboard::GetInstance().m_keys[SDL_SCANCODE_F6];
const Key& Keyboard::F7 = Keyboard::GetInstance().m_keys[SDL_SCANCODE_F7];
const Key& Keyboard::F8 = Keyboard::GetInstance().m_keys[SDL_SCANCODE_F8];
const Key& Keyboard::F9 = Keyboard::GetInstance().m_keys[SDL_SCANCODE_F9];
const Key& Keyboard::F10 = Keyboard::GetInstance().m_keys[SDL_SCANCODE_F10];
const Key& Keyboard::F11 = Keyboard::GetInstance().m_keys[SDL_SCANCODE_F11];
const Key& Keyboard::F12 = Keyboard::GetInstance().m_keys[SDL_SCANCODE_F12];


const Key& Keyboard::Escape = Keyboard::GetInstance().m_keys[SDL_SCANCODE_ESCAPE];
const Key& Keyboard::Enter = Keyboard::GetInstance().m_keys[SDL_SCANCODE_RETURN];
const Key& Keyboard::Space = Keyboard::GetInstance().m_keys[SDL_SCANCODE_SPACE];
const Key& Keyboard::Tab = Keyboard::GetInstance().m_keys[SDL_SCANCODE_TAB];
const Key& Keyboard::Backspace = Keyboard::GetInstance().m_keys[SDL_SCANCODE_BACKSPACE];
const Key& Keyboard::Delete = Keyboard::GetInstance().m_keys[SDL_SCANCODE_DELETE];
const Key& Keyboard::Insert = Keyboard::GetInstance().m_keys[SDL_SCANCODE_INSERT];
const Key& Keyboard::Home = Keyboard::GetInstance().m_keys[SDL_SCANCODE_HOME];
const Key& Keyboard::End = Keyboard::GetInstance().m_keys[SDL_SCANCODE_END];
const Key& Keyboard::PageUp = Keyboard::GetInstance().m_keys[SDL_SCANCODE_PAGEUP];
const Key& Keyboard::PageDown = Keyboard::GetInstance().m_keys[SDL_SCANCODE_PAGEDOWN];


const Key& Keyboard::Left = Keyboard::GetInstance().m_keys[SDL_SCANCODE_LEFT];
const Key& Keyboard::Right = Keyboard::GetInstance().m_keys[SDL_SCANCODE_RIGHT];
const Key& Keyboard::Up = Keyboard::GetInstance().m_keys[SDL_SCANCODE_UP];
const Key& Keyboard::Down = Keyboard::GetInstance().m_keys[SDL_SCANCODE_DOWN];


const Key& Keyboard::Minus = Keyboard::GetInstance().m_keys[SDL_SCANCODE_MINUS];
const Key& Keyboard::Equal = Keyboard::GetInstance().m_keys[SDL_SCANCODE_EQUALS];
const Key& Keyboard::LeftBracket = Keyboard::GetInstance().m_keys[SDL_SCANCODE_LEFTBRACKET];
const Key& Keyboard::RightBracket = Keyboard::GetInstance().m_keys[SDL_SCANCODE_RIGHTBRACKET];
const Key& Keyboard::Backslash = Keyboard::GetInstance().m_keys[SDL_SCANCODE_BACKSLASH];
const Key& Keyboard::Semicolon = Keyboard::GetInstance().m_keys[SDL_SCANCODE_SEMICOLON];
const Key& Keyboard::Apostrophe = Keyboard::GetInstance().m_keys[SDL_SCANCODE_APOSTROPHE];
const Key& Keyboard::Grave = Keyboard::GetInstance().m_keys[SDL_SCANCODE_GRAVE];
const Key& Keyboard::Comma = Keyboard::GetInstance().m_keys[SDL_SCANCODE_COMMA];
const Key& Keyboard::Period = Keyboard::GetInstance().m_keys[SDL_SCANCODE_PERIOD];
const Key& Keyboard::Slash = Keyboard::GetInstance().m_keys[SDL_SCANCODE_SLASH];


const Key& Keyboard::Keypad0 = Keyboard::GetInstance().m_keys[SDL_SCANCODE_KP_0];
const Key& Keyboard::Keypad1 = Keyboard::GetInstance().m_keys[SDL_SCANCODE_KP_1];
const Key& Keyboard::Keypad2 = Keyboard::GetInstance().m_keys[SDL_SCANCODE_KP_2];
const Key& Keyboard::Keypad3 = Keyboard::GetInstance().m_keys[SDL_SCANCODE_KP_3];
const Key& Keyboard::Keypad4 = Keyboard::GetInstance().m_keys[SDL_SCANCODE_KP_4];
const Key& Keyboard::Keypad5 = Keyboard::GetInstance().m_keys[SDL_SCANCODE_KP_5];
const Key& Keyboard::Keypad6 = Keyboard::GetInstance().m_keys[SDL_SCANCODE_KP_6];
const Key& Keyboard::Keypad7 = Keyboard::GetInstance().m_keys[SDL_SCANCODE_KP_7];
const Key& Keyboard::Keypad8 = Keyboard::GetInstance().m_keys[SDL_SCANCODE_KP_8];
const Key& Keyboard::Keypad9 = Keyboard::GetInstance().m_keys[SDL_SCANCODE_KP_9];
const Key& Keyboard::KeypadDivide = Keyboard::GetInstance().m_keys[SDL_SCANCODE_KP_DIVIDE];
const Key& Keyboard::KeypadMultiply = Keyboard::GetInstance().m_keys[SDL_SCANCODE_KP_MULTIPLY];
const Key& Keyboard::KeypadMinus = Keyboard::GetInstance().m_keys[SDL_SCANCODE_KP_MINUS];
const Key& Keyboard::KeypadPlus = Keyboard::GetInstance().m_keys[SDL_SCANCODE_KP_PLUS];
const Key& Keyboard::KeypadEnter = Keyboard::GetInstance().m_keys[SDL_SCANCODE_KP_ENTER];
const Key& Keyboard::KeypadPeriod = Keyboard::GetInstance().m_keys[SDL_SCANCODE_KP_PERIOD];


const Key& Keyboard::LeftShift = Keyboard::GetInstance().m_keys[SDL_SCANCODE_LSHIFT];
const Key& Keyboard::RightShift = Keyboard::GetInstance().m_keys[SDL_SCANCODE_RSHIFT];
const Key& Keyboard::LeftCtrl = Keyboard::GetInstance().m_keys[SDL_SCANCODE_LCTRL];
const Key& Keyboard::RightCtrl = Keyboard::GetInstance().m_keys[SDL_SCANCODE_RCTRL];
const Key& Keyboard::LeftAlt = Keyboard::GetInstance().m_keys[SDL_SCANCODE_LALT];
const Key& Keyboard::RightAlt = Keyboard::GetInstance().m_keys[SDL_SCANCODE_RALT];
const Key& Keyboard::LeftGUI = Keyboard::GetInstance().m_keys[SDL_SCANCODE_LGUI];
const Key& Keyboard::RightGUI = Keyboard::GetInstance().m_keys[SDL_SCANCODE_RGUI];
const Key& Keyboard::CapsLock = Keyboard::GetInstance().m_keys[SDL_SCANCODE_CAPSLOCK];
const Key& Keyboard::ScrollLock = Keyboard::GetInstance().m_keys[SDL_SCANCODE_SCROLLLOCK];
const Key& Keyboard::NumLock = Keyboard::GetInstance().m_keys[SDL_SCANCODE_NUMLOCKCLEAR];
const Key& Keyboard::PrintScreen = Keyboard::GetInstance().m_keys[SDL_SCANCODE_PRINTSCREEN];
const Key& Keyboard::Pause = Keyboard::GetInstance().m_keys[SDL_SCANCODE_PAUSE];
