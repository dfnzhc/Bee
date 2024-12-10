#include "IO/Inputs.hpp"

#include <mutex>

using namespace bee;

namespace {

std::atomic<std::size_t> sSubscribeIndex = 0;
std::mutex sSubscribeMutex, sMouseMutex, sKeyboardMutex;

} // namespace

std::size_t InputManager::subscribe(InputCallback&& callback)
{
    auto idx = sSubscribeIndex.fetch_add(1, std::memory_order_relaxed);

    std::unique_lock<std::mutex> lock(sSubscribeMutex);
    _subscribers.emplace(idx, std::move(callback));

    return idx;
}

void InputManager::unsubscribe(std::size_t subId)
{
    std::unique_lock<std::mutex> lock(sSubscribeMutex);
    if (_subscribers.contains(subId)) {
        _subscribers.extract(subId);
    }
}

void InputManager::onMouseEvent(const MouseEvent& event)
{
    {
        std::unique_lock<std::mutex> lock(sMouseMutex);
        _mouse.onMouseEvent(event);
    }

    update();
}

void InputManager::onKeyboardEvent(const KeyboardEvent& event)
{
    {
        std::unique_lock<std::mutex> lock(sKeyboardMutex);
        _keyboard.onKeyboardEvent(event);
    }

    update();
}

void InputManager::tick()
{
    _mouse.tick();
    _keyboard.tick();
}

void InputManager::update()
{
    std::unique_lock<std::mutex> lock(sSubscribeMutex);
    for (const auto& callback : _subscribers | std::views::values) {
        callback(_mouse, _keyboard);
    }
}