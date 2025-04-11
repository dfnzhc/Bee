/**
 * @File EventManager.cpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2025/4/7
 * @Brief This file is part of Bee.
 */


#include "Core/Events/EventManager.hpp"
#include "Base/Error.hpp"

#include <utility>

#include <eventpp/utilities/argumentadapter.h>
#include <eventpp/utilities/conditionalfunctor.h>
#include <eventpp/utilities/anydata.h>
#include <eventpp/eventdispatcher.h>
#include <eventpp/eventqueue.h>

using namespace bee;

namespace  {
struct EventPolicy
{
    static EventType getEvent(const EventPtr& event)
    {
        return event->type();
    }
};
} // namespace 

class bee::EventManager::Impl
{
public:
    /// ==========================
    using EventQueueType = eventpp::EventQueue<EventType, void(const EventPtr&), EventPolicy>;
    using QueueHandle    = EventQueueType::Handle;

    Handle Register(EventType type, CallbackType&& callback)
    {
        const auto h = nextHandle();
        const auto qh = queue.appendListener(type, std::move(callback));
        
        handles.emplace(h, qh);
        types.emplace(h, type);

        return h;
    }

    void Unregister(Handle handle)
    {
        if (!(handles.contains(handle) && types.contains(handle))) {
            LogWarn("No such event handle exist: '{}'.", handle);
            return;
        }

        const auto qh = handles.extract(handle).mapped();
        const auto type = types.extract(handle).mapped();
            
        if (!queue.removeListener(type, qh)) {
            LogWarn("Unregister from event system failed: '{}'.", handle);
        }
    }

    void Enqueue(const EventPtr& event)
    {
        queue.enqueue(event);
    }

    void Process()
    {
        queue.process();
    }

    void Dispatch(const EventPtr& event) const
    {
        queue.dispatch(event);
    }

    Handle nextHandle()
    {
        return currentHandle++;
    }
    
    EventQueueType queue;
    std::unordered_map<Handle, QueueHandle> handles;
    std::unordered_map<Handle, EventType> types;

    Handle currentHandle = 0;
};

EventManager::Handle EventManager::Register(EventType type, CallbackType&& callback) const
{
    return _impl->Register(type, std::move(callback));
}

void EventManager::Unregister(Handle handle) const
{
    _impl->Unregister(handle);
}

void EventManager::Push(const EventPtr& event) const
{
    _impl->Enqueue(event);
}

void EventManager::Process() const
{
    _impl->Process();
}

void EventManager::Dispatch(const EventPtr& event) const
{
    _impl->Dispatch(event);
}

EventManager::EventManager() : _impl(std::make_unique<Impl>())
{
    BEE_ASSERT(_impl, "Failed to init event system.");
}

EventManager::~EventManager()
{
    if (_impl) {
        _impl.reset();
        _impl = nullptr;
    }
}