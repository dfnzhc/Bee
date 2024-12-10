/**
 * @File Inputs.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2024/11/19
 * @Brief This file is part of Bee.
 */

#pragma once

#include "IO/Keyboard.hpp"
#include "IO/Mouse.hpp"

namespace bee {

class BEE_API InputManager
{
public:
    using InputCallback   = std::function<void(const MouseInput&, const KeyboardInput&)>;
    using InputSubscriber = std::unordered_map<std::size_t, InputCallback>;

    std::size_t subscribe(InputCallback&& callback);
    void unsubscribe(std::size_t subId);

    void onMouseEvent(const MouseEvent& event);
    void onKeyboardEvent(const KeyboardEvent& event);

    void tick();

private:
    void update();
    
private:
    MouseInput _mouse;
    KeyboardInput _keyboard;

    InputSubscriber _subscribers;
};

} // namespace bee
