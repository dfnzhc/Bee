/**
 * @File EventManager.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2025/4/7
 * @Brief This file is part of Bee.
 */

#pragma once

#include "Base/Defines.hpp"
#include "Base/Memory.hpp"
#include "Base/Object.hpp"

#include "Core/Events/Event.hpp"

#include <functional>

namespace bee {

class BEE_API EventManager final : public NonCopyable
{
public:
    static EventManager& Instance()
    {
        static EventManager es;
        return es;
    }

    using Handle       = u64;
    using CallbackType = std::function<void(const EventPtr&)>;
    
    /// ==========================
    Handle Register(EventType type, CallbackType&& callback) const;
    
    void Unregister(Handle handle) const;

    // Asynchronously
    void Push(const EventPtr& event) const;
    void Process() const;

    // Synchronously
    void Dispatch(const EventPtr& event) const;

private:
    EventManager();
    ~EventManager() override;

private:
    class Impl;
    UniquePtr<Impl> _impl;
};
} // namespace bee