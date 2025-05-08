/**
 * @File EventManager.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2025/4/7
 * @Brief This file is part of Bee.
 */

#pragma once

#include "Core/Utility/ClassTypes.hpp"
#include "Engine/Events/Event.hpp"

#include <functional>

namespace bee {
/**
 * @brief Defines the priority for event listeners.
 * Listeners with higher priority values are typically invoked before those with lower priority for the same event. 
 */
enum class EventPriority
{
    Lowest  = 0,
    Low     = 100,
    Normal  = 200,
    High    = 300,
    Highest = 400,
    Monitor = 500
};

/**
 * @brief Manages event subscriptions and dispatching in the system.
 */
class BEE_API EventManager final : public CMoveable
{
public:
    /**
     * @brief Opaque handle representing a subscription to an event.
     */
    using Handle = u64;

    /**
     * @brief Represents an invalid or uninitialized subscription handle.
     */
    static constexpr Handle kInvalidHandle = 0;

    /**
     * @brief Get the singleton instance.
     */
    static EventManager& Get()
    {
        static EventManager manager;
        return manager;
    }

    /**
     * @brief Subscribes a listener for direct (synchronous) dispatch of a specific event type.
     * The listener is invoked immediately in the same thread that calls `dispatch_immediate`.
     *
     * @tparam EventType The specific event class (derived from IEventBase) to listen for.
     * @param listener A std::function or compatible callable that takes a `const EventPtr&` as argument.
     *                 This listener will be called when an event of `EventType` is dispatched directly.
     * @return A subscription handle to be used for unsubscribing. Returns kInvalidHandle on failure.
     */
    template<cEventType EventType>
    Handle subscribeDirect(std::function<void(const EventType&)> listener);

    /**
     * @brief Subscribes a listener for queued (asynchronous) dispatch of a specific event type.
     *
     * @tparam EventType The specific event class (derived from IEventBase) to listen for.
     * @param listener A std::function or compatible callable that takes a `const EventType&` as argument.
     *                 This listener will be called when an event of `EventType` is processed from the queue.
     * @return A subscription handle to be used for unsubscribing. Returns kInvalidHandle on failure.
     */
    template<cEventType EventType>
    Handle subscribeQueued(std::function<void(const EventType&)> listener);

    /**
     * @brief Unsubscribes a previously registered listener.
     * @param handle The SubscriptionHandle received from a `subscribeDirect` or `subscribeQueued` call.
     * @return True if the listener was successfully unsubscribed, false otherwise (e.g., handle was invalid or not found).
     */
    bool unsubscribe(Handle handle);

    /**
     * @brief Dispatches an event immediately (synchronously) to all relevant direct listeners.
     *
     * @param event A std::shared_ptr to a IEventBase object. The event system takes shared ownership.
     */
    void dispatch(std::shared_ptr<IEventBase> event) const;

    /**
     * @brief Enqueues an event for asynchronous dispatch by worker threads.
     *
     * @param event A std::shared_ptr to a const IEventBase object. The event system takes shared ownership.
     * @param priority The priority of this event. Events with higher priority are generally called first.
     */
    void enqueue(std::shared_ptr<IEventBase> event, EventPriority priority = EventPriority::Normal);

private:
    /**
     * @brief Helper function to subscribe a listener of a specific event type.
     */
    template<cEventType EventType>
    Handle subscribeHelper(std::function<void(const EventType&)> listener, bool bIsDirect);

private:
    EventManager();
    ~EventManager() override;

    struct ListenerInfo
    {
        std::function<void(const EventPtr&)> callback;
        Handle handle;
        bool isDirectSubscription;
    };

    bool addSubscription(std::type_index eventId, ListenerInfo&& record);

private:
    UniquePtr<class Impl> _pImpl;
    std::atomic<Handle> _nextSubscriptionHandle{1};

    friend class Impl;
};

template<cEventType EventType>
EventManager::Handle EventManager::subscribeDirect(std::function<void(const EventType&)> listener)
{
    return subscribeHelper<EventType>(listener, true);
}

template<cEventType EventType>
EventManager::Handle EventManager::subscribeQueued(std::function<void(const EventType&)> listener)
{
    return subscribeHelper<EventType>(listener, false);
}

template<cEventType EventType>
EventManager::Handle EventManager::subscribeHelper(std::function<void(const EventType&)> listener, bool bIsDirect)
{
    const auto handle = _nextSubscriptionHandle.fetch_add(1, std::memory_order_relaxed);

    ListenerInfo record{};
    record.handle               = handle;
    record.isDirectSubscription = bIsDirect;

    record.callback = [callback = std::move(listener)](const EventPtr& baseEvent) {
        if (auto specificEvent = static_cast<const EventType*>(baseEvent.get())) {
            callback(*specificEvent);
        }
    };

    if (addSubscription(EventTypeId<EventType>(), std::move(record)))
        return handle;

    return kInvalidHandle;
}

#if 0
class BEE_API EventManager final : public CMoveable
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
#endif
} // namespace bee