#ifndef KEYBOARD_H
#define KEYBOARD_H

#include "Utils/LazySingleton.h"
#include <SDL3/SDL_events.h>
#include <array>

#include "SDL3/SDL_oldnames.h"

/**
 * @brief 表示一个按键的状态。
 *
 * 包含按键当前是否按下以及上一帧是否按下的信息。
 */
struct Key
{
    /**
     * @brief 检查按键当前是否处于按下状态。
     * @return 如果按键当前被按下，则返回 true；否则返回 false。
     */
    [[nodiscard]] bool IsPressed() const { return isDown; }

    /**
     * @brief 检查按键是否在当前帧被按下（从抬起到按下的瞬间）。
     * @return 如果按键在当前帧被按下，则返回 true；否则返回 false。
     */
    [[nodiscard]] bool IsDown() const { return isDown && !wasDown; }

    /**
     * @brief 检查按键是否在当前帧被抬起（从按下到抬起的瞬间）。
     * @return 如果按键在当前帧被抬起，则返回 true；否则返回 false。
     */
    [[nodiscard]] bool IsUp() const { return !isDown && wasDown; }

private:
    friend class Keyboard;
    bool isDown = false; ///< 按键当前是否被按下。
    bool wasDown = false; ///< 按键上一帧是否被按下。
};

/**
 * @brief 键盘输入管理器，采用懒汉式单例模式。
 *
 * 负责处理键盘事件，并提供查询各个按键状态的接口。
 */
class Keyboard : public LazySingleton<Keyboard>
{
public:
    friend class LazySingleton<Keyboard>;

    /**
     * @brief 更新键盘状态。
     *
     * 在每帧开始时调用，用于更新按键的“上一帧状态”。
     */
    void Update();

    /**
     * @brief 处理 SDL 事件。
     * @param event SDL 事件结构体。
     */
    void ProcessEvent(const SDL_Event& event);

    ///< 字母 A 键的状态。
    static const Key& A;
    ///< 字母 B 键的状态。
    static const Key& B;
    ///< 字母 C 键的状态。
    static const Key& C;
    ///< 字母 D 键的状态。
    static const Key& D;
    ///< 字母 E 键的状态。
    static const Key& E;
    ///< 字母 F 键的状态。
    static const Key& F;
    ///< 字母 G 键的状态。
    static const Key& G;
    ///< 字母 H 键的状态。
    static const Key& H;
    ///< 字母 I 键的状态。
    static const Key& I;
    ///< 字母 J 键的状态。
    static const Key& J;
    ///< 字母 K 键的状态。
    static const Key& K;
    ///< 字母 L 键的状态。
    static const Key& L;
    ///< 字母 M 键的状态。
    static const Key& M;
    ///< 字母 N 键的状态。
    static const Key& N;
    ///< 字母 O 键的状态。
    static const Key& O;
    ///< 字母 P 键的状态。
    static const Key& P;
    ///< 字母 Q 键的状态。
    static const Key& Q;
    ///< 字母 R 键的状态。
    static const Key& R;
    ///< 字母 S 键的状态。
    static const Key& S;
    ///< 字母 T 键的状态。
    static const Key& T;
    ///< 字母 U 键的状态。
    static const Key& U;
    ///< 字母 V 键的状态。
    static const Key& V;
    ///< 字母 W 键的状态。
    static const Key& W;
    ///< 字母 X 键的状态。
    static const Key& X;
    ///< 字母 Y 键的状态。
    static const Key& Y;
    ///< 字母 Z 键的状态。
    static const Key& Z;

    ///< 数字 0 键的状态。
    static const Key& Num0;
    ///< 数字 1 键的状态。
    static const Key& Num1;
    ///< 数字 2 键的状态。
    static const Key& Num2;
    ///< 数字 3 键的状态。
    static const Key& Num3;
    ///< 数字 4 键的状态。
    static const Key& Num4;
    ///< 数字 5 键的状态。
    static const Key& Num5;
    ///< 数字 6 键的状态。
    static const Key& Num6;
    ///< 数字 7 键的状态。
    static const Key& Num7;
    ///< 数字 8 键的状态。
    static const Key& Num8;
    ///< 数字 9 键的状态。
    static const Key& Num9;

    ///< F1 键的状态。
    static const Key& F1;
    ///< F2 键的状态。
    static const Key& F2;
    ///< F3 键的状态。
    static const Key& F3;
    ///< F4 键的状态。
    static const Key& F4;
    ///< F5 键的状态。
    static const Key& F5;
    ///< F6 键的状态。
    static const Key& F6;
    ///< F7 键的状态。
    static const Key& F7;
    ///< F8 键的状态。
    static const Key& F8;
    ///< F9 键的状态。
    static const Key& F9;
    ///< F10 键的状态。
    static const Key& F10;
    ///< F11 键的状态。
    static const Key& F11;
    ///< F12 键的状态。
    static const Key& F12;

    ///< Escape 键的状态。
    static const Key& Escape;
    ///< Enter 键的状态。
    static const Key& Enter;
    ///< Space 键的状态。
    static const Key& Space;
    ///< Tab 键的状态。
    static const Key& Tab;
    ///< Backspace 键的状态。
    static const Key& Backspace;
    ///< Delete 键的状态。
    static const Key& Delete;
    ///< Insert 键的状态。
    static const Key& Insert;
    ///< Home 键的状态。
    static const Key& Home;
    ///< End 键的状态。
    static const Key& End;
    ///< PageUp 键的状态。
    static const Key& PageUp;
    ///< PageDown 键的状态。
    static const Key& PageDown;

    ///< 左方向键的状态。
    static const Key& Left;
    ///< 右方向键的状态。
    static const Key& Right;
    ///< 上方向键的状态。
    static const Key& Up;
    ///< 下方向键的状态。
    static const Key& Down;

    ///< 减号键的状态。
    static const Key& Minus;
    ///< 等号键的状态。
    static const Key& Equal;
    ///< 左方括号键的状态。
    static const Key& LeftBracket;
    ///< 右方括号键的状态。
    static const Key& RightBracket;
    ///< 反斜杠键的状态。
    static const Key& Backslash;
    ///< 分号键的状态。
    static const Key& Semicolon;
    ///< 单引号键的状态。
    static const Key& Apostrophe;
    ///< 重音符/波浪号键的状态。
    static const Key& Grave;
    ///< 逗号键的状态。
    static const Key& Comma;
    ///< 句号键的状态。
    static const Key& Period;
    ///< 斜杠键的状态。
    static const Key& Slash;

    ///< 小键盘 0 键的状态。
    static const Key& Keypad0;
    ///< 小键盘 1 键的状态。
    static const Key& Keypad1;
    ///< 小键盘 2 键的状态。
    static const Key& Keypad2;
    ///< 小键盘 3 键的状态。
    static const Key& Keypad3;
    ///< 小键盘 4 键的状态。
    static const Key& Keypad4;
    ///< 小键盘 5 键的状态。
    static const Key& Keypad5;
    ///< 小键盘 6 键的状态。
    static const Key& Keypad6;
    ///< 小键盘 7 键的状态。
    static const Key& Keypad7;
    ///< 小键盘 8 键的状态。
    static const Key& Keypad8;
    ///< 小键盘 9 键的状态。
    static const Key& Keypad9;
    ///< 小键盘除号键的状态。
    static const Key& KeypadDivide;
    ///< 小键盘乘号键的状态。
    static const Key& KeypadMultiply;
    ///< 小键盘减号键的状态。
    static const Key& KeypadMinus;
    ///< 小键盘加号键的状态。
    static const Key& KeypadPlus;
    ///< 小键盘 Enter 键的状态。
    static const Key& KeypadEnter;
    ///< 小键盘小数点键的状态。
    static const Key& KeypadPeriod;

    ///< 左 Shift 键的状态。
    static const Key& LeftShift;
    ///< 右 Shift 键的状态。
    static const Key& RightShift;
    ///< 左 Ctrl 键的状态。
    static const Key& LeftCtrl;
    ///< 右 Ctrl 键的状态。
    static const Key& RightCtrl;
    ///< 左 Alt 键的状态。
    static const Key& LeftAlt;
    ///< 右 Alt 键的状态。
    static const Key& RightAlt;
    ///< 左 GUI (Windows/Command) 键的状态。
    static const Key& LeftGUI;
    ///< 右 GUI (Windows/Command) 键的状态。
    static const Key& RightGUI;
    ///< Caps Lock 键的状态。
    static const Key& CapsLock;
    ///< Scroll Lock 键的状态。
    static const Key& ScrollLock;
    ///< Num Lock 键的状态。
    static const Key& NumLock;
    ///< Print Screen 键的状态。
    static const Key& PrintScreen;
    ///< Pause 键的状态。
    static const Key& Pause;

    /**
     * @brief 根据扫描码获取对应按键的状态。
     * @param scancode 按键的 SDL 扫描码。
     * @return 对应扫描码的 Key 对象引用。如果扫描码无效，则返回一个静态的无效 Key 对象。
     */
    Key& GetKeyState(int scancode)
    {
        if (scancode <= SDL_SCANCODE_UNKNOWN || scancode >= SDL_SCANCODE_COUNT)
        {
            static Key invalidKey;
            return invalidKey;
        }
        return m_keys[scancode];
    }

private:
    /**
     * @brief 构造函数，私有化以强制使用单例模式。
     */
    Keyboard() = default;

    ///< 存储所有按键状态的数组，索引为 SDL 扫描码。
    std::array<Key, SDL_SCANCODE_COUNT> m_keys{};
};

#endif