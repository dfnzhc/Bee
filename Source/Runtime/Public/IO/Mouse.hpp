/**
 * @File Mouse.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2024/12/9
 * @Brief This file is part of Bee.
 */

#pragma once

#include "Core/Defines.hpp"
#include "Core/Portability.hpp"
#include "Memory/Memory.hpp"
#include "Utility/Enum.hpp"

#include <bitset>
#include "Math/Hash.hpp"

#include <algorithm>
#include <ranges>
#include <fmt/format.h>

namespace bee {

struct MouseEvent
{
    enum class Button : u8
    {
        Left,
        Middle,
        Right,
        // TODO: 添加侧键
        Unknown // Any unknown mouse button
    };

    enum class Type : u8
    {
        ButtonDown,
        ButtonUp,
        Move,
        Wheel,
    };

    Type type;
    vec2 pos        = {};              // 标准化坐标，范围[0, 1]
    vec2 screenPos  = {};              // 屏幕空间坐标，范围[0, 窗口大小]
    vec2 wheelDelta = {};              // 鼠标滚轮滚动量
    Button button   = Button::Unknown; // 鼠标按钮

    friend BEE_CONSTEXPR bool operator==(const MouseEvent& lhs, const MouseEvent& rhs)
    {
        return lhs.type == rhs.type && lhs.pos == rhs.pos && lhs.screenPos == rhs.screenPos && lhs.wheelDelta == rhs.wheelDelta &&
               lhs.button == rhs.button;
    }

    // clang-format off
    BEE_NODISCARD friend std::string ToString(const MouseEvent& me)
    {
        std::string_view typeName;
        switch (me.type) {
        case Type::ButtonDown : typeName = "按下"; break;
        case Type::ButtonUp   : typeName = "松开"; break;
        case Type::Move       : typeName = "移动"; break;
        case Type::Wheel      : typeName = "滚轮"; break;
        }

        std::string_view btn;
        switch (me.button) {
        case Button::Left   : btn = "左键"; break;
        case Button::Middle : btn = "中建"; break;
        case Button::Right  : btn = "右键"; break;
        case Button::Unknown: btn = "未知"; break;
        }

        return fmt::format("鼠标 [\"{}{}\" - {},{}({:.3f},{:.3f})|{:.3f}/{:.3f}]",
                           typeName, btn, static_cast<int>(me.screenPos.x), static_cast<int>(me.screenPos.y),
                           me.pos.x, me.pos.y, me.wheelDelta.x, me.wheelDelta.y);
    }

    // clang-format on
};

class MouseInput
{
public:
    void onMouseEvent(const MouseEvent& mouse)
    {
        _currState[mouse.button] = mouse;

        if (mouse.button == MouseEvent::Button::Unknown && mouse.type == MouseEvent::Type::Move) {
            _mouseMoving = true;
        }

        if (mouse.button == MouseEvent::Button::Middle && mouse.type == MouseEvent::Type::Wheel) {
            _wheelScrolled = true;
        }
    }

    void tick()
    {
        _prevState = _currState;

        _mouseMoving   = false;
        _wheelScrolled = false;
    }

    // clang-format off
    BEE_NODISCARD bool isMoving() const noexcept { return _mouseMoving; }
    BEE_NODISCARD bool isWheelScrolling() const noexcept { return _wheelScrolled; }
    
    BEE_NODISCARD bool isButtonDown(MouseEvent::Button mb)  const { return TestButtonType(_currState, mb, MouseEvent::Type::ButtonDown); }
    BEE_NODISCARD bool isButtonUp(MouseEvent::Button mb)    const { return TestButtonType(_currState, mb, MouseEvent::Type::ButtonUp); }
    BEE_NODISCARD bool wasButtonDown(MouseEvent::Button mb) const { return TestButtonType(_prevState, mb, MouseEvent::Type::ButtonDown); }
    BEE_NODISCARD bool wasButtonUp(MouseEvent::Button mb)   const { return TestButtonType(_prevState, mb, MouseEvent::Type::ButtonUp); }
    
    BEE_NODISCARD bool hasButtonDown() const { return TestButtonType(_currState, MouseEvent::Button::Unknown, MouseEvent::Type::ButtonDown); }
    BEE_NODISCARD bool hasButtonUp()   const { return TestButtonType(_currState, MouseEvent::Button::Unknown, MouseEvent::Type::ButtonUp); }
    
    BEE_NODISCARD bool isButtonRepeated(MouseEvent::Button mb) const { return isButtonDown(mb) && wasButtonDown(mb); }
    BEE_NODISCARD bool isButtonClicked(MouseEvent::Button mb)  const { return isButtonDown(mb) && !wasButtonDown(mb); }
    BEE_NODISCARD bool isButtonReleased(MouseEvent::Button mb) const { return !isButtonDown(mb) && wasButtonDown(mb); }

    BEE_NODISCARD bool isDragging(MouseEvent::Button mb) const noexcept { return isMoving() && isButtonDown(mb); }

    // clang-format on

    constexpr static auto kMovingButton = MouseEvent::Button::Unknown;

    BEE_NODISCARD Opt<vec2> posDelta() const noexcept
    {
        if (!_currState.contains(kMovingButton))
            return {};
        if (!_prevState.contains(kMovingButton))
            return vec2{};

        return _currState.at(kMovingButton).pos - _prevState.at(kMovingButton).pos;
    }

    BEE_NODISCARD Opt<vec2> screenPosDelta() const noexcept
    {
        if (!_currState.contains(kMovingButton))
            return {};
        if (!_prevState.contains(kMovingButton))
            return vec2{};

        return _currState.at(kMovingButton).screenPos - _prevState.at(kMovingButton).screenPos;
    }

    BEE_NODISCARD Opt<vec2> wheelDelta() const noexcept
    {
        constexpr auto kWheelButton = MouseEvent::Button::Middle;
        if (!_currState.contains(kWheelButton))
            return {};
        if (!_prevState.contains(kWheelButton))
            return _currState.at(kWheelButton).wheelDelta;

        return _currState.at(kWheelButton).wheelDelta - _prevState.at(kWheelButton).wheelDelta;
    }

private:
    using MouseState = std::unordered_map<MouseEvent::Button, MouseEvent>;

    BEE_NODISCARD inline static bool TestButtonType(const MouseState& ms, MouseEvent::Button mb, MouseEvent::Type mt)
    {
        if (mb < MouseEvent::Button::Unknown)
            return ms.contains(mb) && ms.at(mb).type == mt;

        return std::ranges::any_of(ms, [mt](const auto& p) { return p.second.type == mt; });
    }

private:
    MouseState _prevState = {}, _currState = {};
    bool _mouseMoving = false, _wheelScrolled = false;
};

} // namespace bee

namespace std {
template<> struct hash<bee::MouseEvent>
{
    size_t operator()(const bee::MouseEvent& me) const noexcept
    {
        size_t seed = 0;
        bee::HashCombine(seed, magic_enum::enum_name(me.type));
        bee::HashCombine(seed, me.pos);
        bee::HashCombine(seed, me.screenPos);
        bee::HashCombine(seed, me.wheelDelta);
        bee::HashCombine(seed, magic_enum::enum_name(me.button));

        return seed;
    }
};
} // namespace std