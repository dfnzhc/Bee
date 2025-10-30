/**
 * @File PlatformTypes.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2025/10/16
 * @Brief This file is part of Bee.
 */

#pragma once

#include <string>
#include <functional>
#include <variant>

#include "Core/Base/Defines.hpp"
#include "Core/Error/Error.hpp"
#include "Core/Math/Math.hpp"

namespace Bee
{
    // 平台类型句柄
    #define XIHE_PLATFORM_HANDLE(HandleName, UnderlyingType)                                    \
        struct HandleName {                                                                     \
            UnderlyingType handle;                                                              \
            explicit HandleName(UnderlyingType h = {}) : handle(h) {}                           \
            bool operator==(const HandleName& other) const { return handle == other.handle; }   \
            bool operator!=(const HandleName& other) const { return handle != other.handle; }   \
            bool operator<(const HandleName& other) const { return handle < other.handle; }     \
            explicit operator bool() const { return handle != UnderlyingType{}; }               \
        }

    XIHE_PLATFORM_HANDLE(WindowHandle, u32);

    // 显示器信息
    struct DisplayInfo
    {
        std::string name = {};
        i32 index        = -1;
        i32 posX         = 0;
        i32 posY         = 0;
        u32 width        = 0;
        u32 height       = 0;
        f32 dpi          = 1.0f;
        f32 refreshRate  = 60.0f;
        bool isPrimary   = false;
    };

    // ==================== 平台事件 ====================

    enum class PlatformEventType : u32
    {
        None = 0,

        Quit,

        // --- 窗口 ---
        WindowShow,
        WindowHide,
        WindowMoved,
        WindowResize,
        WindowMinimized,
        WindowMaximized,
        WindowMouseEnter,
        WindowMouseLeave,
        WindowFocusGained,
        WindowFocusLost,
        WindowCloseRequested,
        WindowEnterFullscreen,
        WindowLeaveFullscreen,
        WindowDestroyed,

        // --- 键盘 ---
        KeyDown,
        KeyUp,
        TextInput,

        // --- 鼠标 ---
        MouseMotion,
        MouseButtonDown,
        MouseButtonUp,
        MouseWheel,

        // --- 拖拽 ---
        DropFile,
        DropText,
    };

    // 键盘按键
    enum class KeyCode : u32
    {
        Unknown = 0,

        A, B, C, D, E, F, G, H, I, J, K, L, M,
        N, O, P, Q, R, S, T, U, V, W, X, Y, Z,

        Num0, Num1, Num2, Num3, Num4, Num5, Num6, Num7, Num8, Num9,

        F1, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12,

        Left, Right, Up, Down,

        Space, Enter, Escape, Tab, Backspace, Delete,
        LeftShift, RightShift, LeftCtrl, RightCtrl, LeftAlt, RightAlt, LeftSuper, RightSuper,

        Apostrophe, Comma, Minus, Period, Slash, Backslash, Semicolon, Equals, LeftBracket, RightBracket, Grave,

        Keypad0, Keypad1, Keypad2, Keypad3, Keypad4, Keypad5, Keypad6, Keypad7, Keypad8, Keypad9, KeypadPeriod, KeypadDivide,
        KeypadMultiply, KeypadMinus, KeypadPlus, KeypadEnter, KeypadEquals,

        Home, End, PageUp, PageDown, Insert, CapsLock, ScrollLock, NumLock, PrintScreen, Pause, Menu,

        Count
    };

    // 键盘修饰键
    enum class KeyModifier : u16
    {
        None       = 0x0000,
        LeftShift  = 0x0001,
        RightShift = 0x0002,
        LeftCtrl   = 0x0040,
        RightCtrl  = 0x0080,
        LeftAlt    = 0x0100,
        RightAlt   = 0x0200,

        Shift = LeftShift | RightShift,
        Ctrl  = LeftCtrl | RightCtrl,
        Alt   = LeftAlt | RightAlt,
    };

    BEE_ENUM_CLASS_OPERATORS(KeyModifier)

    // 鼠标按钮
    enum class MouseButton : u32
    {
        Unknown = 0,
        Left,
        Right,
        Middle,
        X1,
        X2,

        Count
    };

    // ==================== 平台事件结构体 ====================

    struct CommonEvent
    {
        PlatformEventType type{};
        u32 reserved;
    };

    struct WindowEvent
    {
        PlatformEventType type{};
        WindowHandle handle{};
        i32 data1{0};
        i32 data2{0};
    };

    struct KeyboardEvent
    {
        PlatformEventType type{};
        WindowHandle windowHandle{};
        KeyCode keyCode{KeyCode::Unknown};
        KeyModifier modifiers{KeyModifier::None};
        u16 rawScancode{0};
        bool isPressed{false};
        bool isRepeat{false};
    };

    struct TextInputEvent
    {
        PlatformEventType type{};
        WindowHandle windowHandle{};
        std::string_view text{};
    };

    struct MouseMotionEvent
    {
        PlatformEventType type{};
        WindowHandle windowHandle{};
        f32 x{0.f};
        f32 y{0.f};
        f32 deltaX{0.f};
        f32 deltaY{0.f};
    };

    struct MouseButtonEvent
    {
        PlatformEventType type{};
        WindowHandle windowHandle{};
        MouseButton button{};
        f32 x{0};
        f32 y{0};
        u8 clicks{1};
        bool isPressed{};
    };

    struct MouseWheelEvent
    {
        PlatformEventType type{};
        WindowHandle windowHandle{};
        f32 deltaX{0.0f}; ///< 水平方向滚动，向右为正，向左为负
        f32 deltaY{0.0f}; ///< 竖直方向滚动，向上为正，向下为负
        f32 mouseX{0};
        f32 mouseY{0};
    };

    struct DropEvent
    {
        PlatformEventType type{};
        WindowHandle windowHandle{};
        std::string data{};
        f32 x = 0;
        f32 y = 0;
    };

    using PlatformEvent = std::variant<CommonEvent,
                                       WindowEvent,
                                       KeyboardEvent,
                                       TextInputEvent,
                                       MouseMotionEvent,
                                       MouseButtonEvent,
                                       MouseWheelEvent,
                                       DropEvent>;

    using InputEvent = std::variant<KeyboardEvent,
                                    TextInputEvent,
                                    MouseMotionEvent,
                                    MouseButtonEvent,
                                    MouseWheelEvent>;
} // namespace Bee
