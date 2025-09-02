using System.Runtime.InteropServices;

namespace Luma.SDK
{
    
    
    
    public static class Input
    {
        private const string DllName = "LumaEngine";

        #region Public API - Cursor

        
        
        
        
        public static Vector2Int GetCursorPosition()
        {
            return cursor_GetPosition();
        }

        
        
        
        
        public static Vector2Int GetCursorDelta()
        {
            return cursor_GetDelta();
        }

        
        
        
        
        public static Vector2 GetScrollDelta()
        {
            return cursor_GetScrollDelta();
        }

        
        
        
        
        public static bool IsLeftMouseButtonPressed()
        {
            return cursor_IsLeftButtonPressed();
        }

        
        
        
        
        public static bool IsLeftMouseButtonDown()
        {
            return cursor_IsLeftButtonDown();
        }

        
        
        
        
        public static bool IsLeftMouseButtonUp()
        {
            return cursor_IsLeftButtonUp();
        }

        
        
        
        
        public static bool IsRightMouseButtonPressed()
        {
            return cursor_IsRightButtonPressed();
        }

        
        
        
        
        public static bool IsRightMouseButtonDown()
        {
            return cursor_IsRightButtonDown();
        }

        
        
        
        
        public static bool IsRightMouseButtonUp()
        {
            return cursor_IsRightButtonUp();
        }

        
        
        
        
        public static bool IsMiddleMouseButtonPressed()
        {
            return cursor_IsMiddleButtonPressed();
        }

        
        
        
        
        public static bool IsMiddleMouseButtonDown()
        {
            return cursor_IsMiddleButtonDown();
        }

        
        
        
        
        public static bool IsMiddleMouseButtonUp()
        {
            return cursor_IsMiddleButtonUp();
        }

        #endregion

        #region Public API - Keyboard

        
        
        
        
        
        public static bool IsKeyPressed(Scancode scancode)
        {
            return keyboard_IsKeyPressed((int)scancode);
        }

        
        
        
        
        
        public static bool IsKeyDown(Scancode scancode)
        {
            return keyboard_IsKeyDown((int)scancode);
        }

        
        
        
        
        
        public static bool IsKeyUp(Scancode scancode)
        {
            return keyboard_IsKeyUp((int)scancode);
        }

        #endregion

        #region Private Interop Methods

        [DllImport(DllName, EntryPoint = "Cursor_GetPosition")]
        private static extern Vector2Int cursor_GetPosition();

        [DllImport(DllName, EntryPoint = "Cursor_GetDelta")]
        private static extern Vector2Int cursor_GetDelta();

        [DllImport(DllName, EntryPoint = "Cursor_GetScrollDelta")]
        private static extern Vector2 cursor_GetScrollDelta();

        [DllImport(DllName, EntryPoint = "Cursor_IsLeftButtonPressed")]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool cursor_IsLeftButtonPressed();

        [DllImport(DllName, EntryPoint = "Cursor_IsLeftButtonDown")]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool cursor_IsLeftButtonDown();

        [DllImport(DllName, EntryPoint = "Cursor_IsLeftButtonUp")]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool cursor_IsLeftButtonUp();

        [DllImport(DllName, EntryPoint = "Cursor_IsRightButtonPressed")]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool cursor_IsRightButtonPressed();

        [DllImport(DllName, EntryPoint = "Cursor_IsRightButtonDown")]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool cursor_IsRightButtonDown();

        [DllImport(DllName, EntryPoint = "Cursor_IsRightButtonUp")]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool cursor_IsRightButtonUp();

        [DllImport(DllName, EntryPoint = "Cursor_IsMiddleButtonPressed")]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool cursor_IsMiddleButtonPressed();

        [DllImport(DllName, EntryPoint = "Cursor_IsMiddleButtonDown")]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool cursor_IsMiddleButtonDown();

        [DllImport(DllName, EntryPoint = "Cursor_IsMiddleButtonUp")]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool cursor_IsMiddleButtonUp();

        [DllImport(DllName, EntryPoint = "Keyboard_IsKeyPressed")]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool keyboard_IsKeyPressed(int scancode);

        [DllImport(DllName, EntryPoint = "Keyboard_IsKeyDown")]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool keyboard_IsKeyDown(int scancode);

        [DllImport(DllName, EntryPoint = "Keyboard_IsKeyUp")]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool keyboard_IsKeyUp(int scancode);

        #endregion
    }

    
    
    
    public enum Scancode
    {
        Unknown = 0,
        A = 4,
        B = 5,
        C = 6,
        D = 7,
        E = 8,
        F = 9,
        G = 10,
        H = 11,
        I = 12,
        J = 13,
        K = 14,
        L = 15,
        M = 16,
        N = 17,
        O = 18,
        P = 19,
        Q = 20,
        R = 21,
        S = 22,
        T = 23,
        U = 24,
        V = 25,
        W = 26,
        X = 27,
        Y = 28,
        Z = 29,
        Num1 = 30,
        Num2 = 31,
        Num3 = 32,
        Num4 = 33,
        Num5 = 34,
        Num6 = 35,
        Num7 = 36,
        Num8 = 37,
        Num9 = 38,
        Num0 = 39,
        Return = 40,
        Escape = 41,
        Backspace = 42,
        Tab = 43,
        Space = 44,
        Minus = 45,
        Equals = 46,
        LeftBracket = 47,
        RightBracket = 48,
        Backslash = 49,
        Semicolon = 51,
        Apostrophe = 52,
        Grave = 53,
        Comma = 54,
        Period = 55,
        Slash = 56,
        CapsLock = 57,
        F1 = 58,
        F2 = 59,
        F3 = 60,
        F4 = 61,
        F5 = 62,
        F6 = 63,
        F7 = 64,
        F8 = 65,
        F9 = 66,
        F10 = 67,
        F11 = 68,
        F12 = 69,
        PrintScreen = 70,
        ScrollLock = 71,
        Pause = 72,
        Insert = 73,
        Home = 74,
        PageUp = 75,
        Delete = 76,
        End = 77,
        PageDown = 78,
        Right = 79,
        Left = 80,
        Down = 81,
        Up = 82,
        NumLockClear = 83,
        KpDivide = 84,
        KpMultiply = 85,
        KpMinus = 86,
        KpPlus = 87,
        KpEnter = 88,
        Kp1 = 89,
        Kp2 = 90,
        Kp3 = 91,
        Kp4 = 92,
        Kp5 = 93,
        Kp6 = 94,
        Kp7 = 95,
        Kp8 = 96,
        Kp9 = 97,
        Kp0 = 98,
        KpPeriod = 99,
        Application = 101,
        Power = 102,
        KpEquals = 103,
        F13 = 104,
        F14 = 105,
        F15 = 106,
        F16 = 107,
        F17 = 108,
        F18 = 109,
        F19 = 110,
        F20 = 111,
        F21 = 112,
        F22 = 113,
        F23 = 114,
        F24 = 115,
        LeftCtrl = 224,
        LeftShift = 225,
        LeftAlt = 226,
        LeftGui = 227,
        RightCtrl = 228,
        RightShift = 229,
        RightAlt = 230,
        RightGui = 231,
    }
}