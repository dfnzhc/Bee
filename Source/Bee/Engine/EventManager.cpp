/**
 * @File EventManager.cpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2025/4/7
 * @Brief This file is part of Bee.
 */

#include "Engine/EventManager.hpp"
#include "Engine/Events/DispatchEvents.hpp"

#include "Core/Error.hpp"

#include <utility>
#include <mutex>

#include <eventpp/utilities/argumentadapter.h>
#include <eventpp/utilities/conditionalfunctor.h>
#include <eventpp/utilities/orderedqueuelist.h>
#include <eventpp/utilities/anydata.h>
#include <eventpp/eventdispatcher.h>
#include <eventpp/eventqueue.h>

using namespace bee;

namespace {
struct EventWrap
{
    EventPtr event;
    std::type_index id;
};

struct EventPolicy
{
    static std::type_index getEvent(const EventWrap& wrap)
    {
        return wrap.id;
    }
};

struct EventWrapQueued
{
    EventPtr event;
    std::type_index id;
    int priority;
};

struct EventCompare
{
    template<typename T>
    bool operator()(const T& a, const T& b) const
    {
        return a.template getArgument<0>().priority > b.template getArgument<0>().priority;
    }
};

struct EventPolicyQueued
{
    template<typename Item>
    using QueueList = eventpp::OrderedQueueList<Item, EventCompare>;

    static std::type_index getEvent(const EventWrapQueued& wrap)
    {
        return wrap.id;
    }
};
} // namespace 

class bee::Impl
{
public:
    using DispatcherType = eventpp::EventDispatcher<std::type_index, void (const EventWrap&), EventPolicy>;
    using QueueType      = eventpp::EventQueue<std::type_index, void (const EventWrapQueued&), EventPolicyQueued>;

    using DispatcherHandle = DispatcherType::Handle;
    using QueueHandle      = QueueType::Handle;
    using Handle           = EventManager::Handle;

    using HandleRecordDispatcher = std::unordered_map<Handle, DispatcherHandle>;
    using HandleRecordQueue      = std::unordered_map<Handle, QueueHandle>;
    using EventRecord            = std::unordered_map<Handle, std::type_index>;

    DispatcherType dispatcher;
    QueueType queue;

    std::mutex dispatcherMutex;
    std::mutex queueMutex;
    std::mutex eventMutex;
    
    HandleRecordDispatcher dispatcherRecord;
    HandleRecordQueue queueRecord;
    EventRecord eventRecord;

    std::jthread queueWorker;
    std::stop_source stopSource;

    void addHandleRecord(Handle handle, DispatcherHandle dispatcherHandle, std::type_index eventId)
    {
        BEE_DEBUG_ASSERT(!dispatcherRecord.contains(handle));

        std::scoped_lock lock{dispatcherMutex, eventMutex};
        dispatcherRecord.emplace(handle, dispatcherHandle);
        eventRecord.emplace(handle, eventId);
    }

    void addHandleRecord(Handle handle, QueueHandle queueHandle, std::type_index eventId)
    {
        BEE_DEBUG_ASSERT(!queueRecord.contains(handle));

        std::scoped_lock lock{queueMutex, eventMutex};
        queueRecord.emplace(handle, queueHandle);
        eventRecord.emplace(handle, eventId);
    }

    bool removeHandle(Handle handle)
    {
        if (isDispatchHandle(handle)) {
            std::scoped_lock lock{dispatcherMutex, eventMutex};
            auto dispatcherHandle = dispatcherRecord.extract(handle).mapped();
            auto eventType        = eventRecord.extract(handle).mapped();

            return dispatcher.removeListener(eventType, dispatcherHandle);
        }

        if (isQueueHandle(handle)) {
            std::scoped_lock lock{queueMutex, eventMutex};
            auto queueHandle = queueRecord.extract(handle).mapped();
            auto eventType   = eventRecord.extract(handle).mapped();

            return queue.removeListener(eventType, queueHandle);
        }

        return false;
    }

    bool isDispatchHandle(Handle handle)
    {
        DispatcherHandle disHandle;
        Opt<std::type_index> eventId;

        {
            std::scoped_lock lock{dispatcherMutex, eventMutex};
            if (dispatcherRecord.contains(handle) && eventRecord.contains(handle)) {
                disHandle = dispatcherRecord.at(handle);
                eventId   = eventRecord.at(handle);
            }
        }

        if (disHandle && eventId.has_value())
            return dispatcher.ownsHandle(eventId.value(), disHandle);

        return false;
    }

    bool isQueueHandle(Handle handle)
    {
        QueueHandle queHandle;
        Opt<std::type_index> eventId;

        {
            std::scoped_lock lock{queueMutex, eventMutex};
            if (queueRecord.contains(handle) && eventRecord.contains(handle)) {
                queHandle = queueRecord.at(handle);
                eventId   = eventRecord.at(handle);
            }
        }

        if (queHandle && eventId.has_value())
            return queue.ownsHandle(eventId.value(), queHandle);

        return false;
    }

    Opt<std::type_index> queryEvent(Handle handle)
    {
        {
            std::lock_guard lock(eventMutex);

            if (eventRecord.contains(handle))
                return eventRecord.at(handle);
        }

        return {};
    }

    void queueProcess(std::stop_token stopToken)
    {
        while (true) {
            while (!queue.waitFor(std::chrono::milliseconds(20)) && !stopToken.stop_requested());
            if (stopToken.stop_requested())
                break;
            
            queue.process();
        }
    }
};

EventManager::EventManager() : _pImpl(std::make_unique<bee::Impl>())
{
    BEE_ASSERT(_pImpl != nullptr);

    _pImpl->queueWorker = std::jthread(&Impl::queueProcess, _pImpl.get(), _pImpl->stopSource.get_token());
}

EventManager::~EventManager()
{
    if (_pImpl) {
        _pImpl.reset();
        _pImpl = nullptr;
    }
}

bool EventManager::addSubscription(std::type_index eventId, ListenerInfo&& record)
{
    if (record.isDirectSubscription) {
        const auto disHandle = _pImpl->dispatcher.appendListener(eventId,
                                                                 [callback = std::move(record.callback)](const EventWrap& wrap) {
                                                                     callback(wrap.event);
                                                                 });

        _pImpl->addHandleRecord(record.handle, disHandle, eventId);
    }
    else {
        const auto queHandle = _pImpl->queue.appendListener(eventId,
                                                            [callback = std::move(record.callback)](const EventWrapQueued& wrap) {
                                                                callback(wrap.event);
                                                            });

        _pImpl->addHandleRecord(record.handle, queHandle, eventId);
    }

    return true;
}

bool EventManager::unsubscribe(Handle handle)
{
    return _pImpl->removeHandle(handle);
}

void EventManager::dispatch(std::shared_ptr<IEventBase> event) const
{
    const auto id = event->typeId();
    _pImpl->dispatcher.dispatch(EventWrap{.event = std::move(event), .id = id});
}

void EventManager::enqueue(std::shared_ptr<IEventBase> event, EventPriority priority)
{
    const auto id = event->typeId();
    _pImpl->queue.enqueue(EventWrapQueued{.event = std::move(event), .id = id, .priority = static_cast<int>(priority)});
}

#if 0
namespace {
struct EventPolicy
{
    static EventType getEvent(const EventPtr& event) { return event->type(); }
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
        const auto h  = nextHandle();
        const auto qh = queue.appendListener(type, callback);

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

    void Enqueue(const EventPtr& event) { queue.enqueue(event); }

    void Process() { queue.process(); }

    void Dispatch(const EventPtr& event) const { queue.dispatch(event); }

    void DispatchSDL(VoidPtr event) const { dispatchCallbackList(event); }

    Handle nextHandle() { return currentHandle++; }

    EventQueueType queue;
    std::unordered_map<Handle, QueueHandle> handles;
    std::unordered_map<Handle, EventType> types;

    eventpp::CallbackList<void(VoidPtr)> dispatchCallbackList;

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
#endif