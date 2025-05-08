/**
 * @File EventManagerTest.cpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2025/5/8
 * @Brief This file is part of Bee.
 */

#include <algorithm>
#include <random>
#include <gtest/gtest.h>

#include <Engine/EventManager.hpp>

using namespace bee;

// A simple event for testing purposes
class TestEventA : public IEventBase
{
public:
    int data;
    std::string message;

    TestEventA(int d, std::string msg) : data(d), message(std::move(msg))
    {
    }

    static std::type_index staticTypeId()
    {
        return EventTypeId<TestEventA>();
    }

    std::type_index typeId() const override
    {
        return staticTypeId();
    }

    String toString() const override
    {
        return "TestEventA: { data: " + std::to_string(data) + ", message: \"" + message + "\" }";
    }
};

// Another distinct event type for testing specificity
class TestEventB : public IEventBase
{
public:
    double value;

    explicit TestEventB(double val) : value(val)
    {
    }

    static std::type_index staticTypeId()
    {
        return EventTypeId<TestEventB>();
    }

    std::type_index typeId() const override
    {
        return staticTypeId();
    }

    String toString() const override
    {
        return "TestEventB: { value: " + std::to_string(value) + " }";
    }
};

// Test fixture for EventManager tests
class EventManagerDirectDispatchTest : public ::testing::Test
{
protected:
    EventManager* em = {}; // Use pointer to avoid issues if CSingleton has specific init/shutdown

    std::string lastMessageA = {};
    int listenerACallCount   = {};
    int lastDataA            = {};

    int listenerBCallCount = {};
    double lastValueB      = {};

    void SetUp() override
    {
        em                 = &EventManager::Get();
        listenerACallCount = 0;
        lastDataA          = 0;
        lastMessageA       = "";
        listenerBCallCount = 0;
        lastValueB         = 0.0;
    }

    void TearDown() override
    {
    }

    // Listener for TestEventA
    void eventAListener(const TestEventA& event)
    {
        listenerACallCount++;
        lastDataA    = event.data;
        lastMessageA = event.message;
    }

    // Listener for TestEventB
    void eventBListener(const TestEventB& event)
    {
        listenerBCallCount++;
        lastValueB = event.value;
    }
};

TEST_F(EventManagerDirectDispatchTest, SubscribeDirect_ValidHandle)
{
    auto handle = em->subscribeDirect<TestEventA>(
        [this](const TestEventA& event) { this->eventAListener(event); }
        );
    EXPECT_NE(handle, EventManager::kInvalidHandle);
    EXPECT_TRUE(em->unsubscribe(handle));
}

TEST_F(EventManagerDirectDispatchTest, Dispatch_CallsSubscribedListener)
{
    auto handle = em->subscribeDirect<TestEventA>(
        [this](const TestEventA& event) { this->eventAListener(event); }
        );
    ASSERT_NE(handle, EventManager::kInvalidHandle);

    auto event = std::make_shared<TestEventA>(123, "Hello");
    em->dispatch(event);

    EXPECT_EQ(listenerACallCount, 1);
    EXPECT_EQ(lastDataA, 123);
    EXPECT_EQ(lastMessageA, "Hello");

    EXPECT_TRUE(em->unsubscribe(handle));
}

TEST_F(EventManagerDirectDispatchTest, Unsubscribe_PreventsFutureCalls)
{
    auto handle = em->subscribeDirect<TestEventA>(
        [this](const TestEventA& event) { this->eventAListener(event); }
        );
    ASSERT_NE(handle, EventManager::kInvalidHandle);

    EXPECT_TRUE(em->unsubscribe(handle));

    auto event = std::make_shared<TestEventA>(456, "World");
    em->dispatch(event); // Dispatch after unsubscription

    EXPECT_EQ(listenerACallCount, 0); // Listener should not have been called

    // Attempting to unsubscribe an already invalid/unsubscribed handle
    EXPECT_FALSE(em->unsubscribe(handle));                       // Should return false for already unsubscribed or invalid
    EXPECT_FALSE(em->unsubscribe(EventManager::kInvalidHandle)); // Test with kInvalidHandle
}

TEST_F(EventManagerDirectDispatchTest, Dispatch_MultipleListenersSameEvent)
{
    int listener1Count = 0;
    int listener2Count = 0;

    auto handle1 = em->subscribeDirect<TestEventA>(
        [&listener1Count](const TestEventA& event) { listener1Count++; }
        );
    auto handle2 = em->subscribeDirect<TestEventA>(
        [&listener2Count](const TestEventA& event) { listener2Count++; }
        );
    ASSERT_NE(handle1, EventManager::kInvalidHandle);
    ASSERT_NE(handle2, EventManager::kInvalidHandle);
    ASSERT_NE(handle1, handle2); // Handles should be unique

    auto event = std::make_shared<TestEventA>(789, "Multi");
    em->dispatch(event);

    EXPECT_EQ(listener1Count, 1);
    EXPECT_EQ(listener2Count, 1);

    // Clean up
    EXPECT_TRUE(em->unsubscribe(handle1));
    EXPECT_TRUE(em->unsubscribe(handle2));
}

TEST_F(EventManagerDirectDispatchTest, Dispatch_ListenerSpecificity)
{
    auto handleA = em->subscribeDirect<TestEventA>(
        [this](const TestEventA& event) { this->eventAListener(event); }
        );
    auto handleB = em->subscribeDirect<TestEventB>(
        [this](const TestEventB& event) { this->eventBListener(event); }
        );
    ASSERT_NE(handleA, EventManager::kInvalidHandle);
    ASSERT_NE(handleB, EventManager::kInvalidHandle);

    // Dispatch EventA
    auto eventA = std::make_shared<TestEventA>(10, "Event A");
    em->dispatch(eventA);

    EXPECT_EQ(listenerACallCount, 1); // Listener A should be called
    EXPECT_EQ(lastDataA, 10);
    EXPECT_EQ(listenerBCallCount, 0); // Listener B should NOT be called

    // Reset counts for next dispatch
    listenerACallCount = 0;
    lastDataA          = 0;

    // Dispatch EventB
    auto eventB = std::make_shared<TestEventB>(3.14);
    em->dispatch(eventB);

    EXPECT_EQ(listenerACallCount, 0); // Listener A should NOT be called
    EXPECT_EQ(listenerBCallCount, 1); // Listener B should be called
    EXPECT_DOUBLE_EQ(lastValueB, 3.14);

    // Clean up
    EXPECT_TRUE(em->unsubscribe(handleA));
    EXPECT_TRUE(em->unsubscribe(handleB));
}

TEST_F(EventManagerDirectDispatchTest, Dispatch_NoListenersSubscribed)
{
    // Ensure no listeners are active from previous tests if TearDown isn't perfect.
    // This test assumes a clean slate or that previous unsubscribes worked.

    auto event = std::make_shared<TestEventA>(999, "NoListeners");

    // We expect this to not throw any exceptions and complete silently.
    EXPECT_NO_THROW(em->dispatch(event));

    EXPECT_EQ(listenerACallCount, 0); // No listener was registered, so count should be 0.
}

TEST_F(EventManagerDirectDispatchTest, SubscribeDirect_CorrectDataReceived)
{
    bool dataCorrect = false;
    TestEventA originalEvent(2024, "DataCheck");

    auto handle = em->subscribeDirect<TestEventA>(
        [&originalEvent, &dataCorrect](const TestEventA& receivedEvent) {
            if (receivedEvent.data == originalEvent.data &&
                receivedEvent.message == originalEvent.message &&
                receivedEvent.typeId() == originalEvent.typeId()) {
                // also check typeId
                dataCorrect = true;
            }
        }
        );
    ASSERT_NE(handle, EventManager::kInvalidHandle);

    em->dispatch(std::make_shared<TestEventA>(originalEvent.data, originalEvent.message));

    EXPECT_TRUE(dataCorrect);

    // Clean up
    EXPECT_TRUE(em->unsubscribe(handle));
}

TEST_F(EventManagerDirectDispatchTest, Unsubscribe_DoesNotAffectOtherListeners)
{
    int listener1Count = 0;
    int listener2Count = 0;

    auto handle1 = em->subscribeDirect<TestEventA>(
        [&listener1Count](const TestEventA& event) { listener1Count++; }
        );
    auto handle2 = em->subscribeDirect<TestEventA>(
        [&listener2Count](const TestEventA& event) { listener2Count++; }
        );
    ASSERT_NE(handle1, EventManager::kInvalidHandle);
    ASSERT_NE(handle2, EventManager::kInvalidHandle);

    // Unsubscribe listener 1
    EXPECT_TRUE(em->unsubscribe(handle1));

    // Dispatch event
    auto event = std::make_shared<TestEventA>(111, "PartialUnsubscribe");
    em->dispatch(event);

    EXPECT_EQ(listener1Count, 0); // Listener 1 should not be called
    EXPECT_EQ(listener2Count, 1); // Listener 2 should still be called

    // Clean up listener 2
    EXPECT_TRUE(em->unsubscribe(handle2));
}

TEST_F(EventManagerDirectDispatchTest, SubscribeDirect_LambdaCaptureMember)
{
    // This test essentially re-validates Dispatch_CallsSubscribedListener
    // but explicitly uses the fixture's methods as part of the test assertion.
    auto handle = em->subscribeDirect<TestEventA>(
        [this](const TestEventA& event) { this->eventAListener(event); }
        );
    ASSERT_NE(handle, EventManager::kInvalidHandle);

    em->dispatch(std::make_shared<TestEventA>(1, "CaptureTest"));
    EXPECT_EQ(listenerACallCount, 1);
    EXPECT_EQ(lastDataA, 1);
    EXPECT_EQ(lastMessageA, "CaptureTest");

    EXPECT_TRUE(em->unsubscribe(handle));
}

// Test dispatching a base pointer where the listener expects a derived type
TEST_F(EventManagerDirectDispatchTest, Dispatch_BasePointerToDerivedListener)
{
    std::shared_ptr<IEventBase> baseEventPtr = std::make_shared<TestEventA>(77, "BaseToDerived");

    auto handle = em->subscribeDirect<TestEventA>(
        [this](const TestEventA& event) { this->eventAListener(event); }
        );
    ASSERT_NE(handle, EventManager::kInvalidHandle);

    em->dispatch(baseEventPtr); // Dispatching std::shared_ptr<IEventBase>

    EXPECT_EQ(listenerACallCount, 1);
    EXPECT_EQ(lastDataA, 77);
    EXPECT_EQ(lastMessageA, "BaseToDerived");

    EXPECT_TRUE(em->unsubscribe(handle));
}

class EventManagerQueueDispatchTest : public ::testing::Test
{
protected:
    EventManager* em = {};

    template<typename ConditionFunc>
    bool waitForCondition(ConditionFunc condition, std::chrono::milliseconds timeout = std::chrono::seconds(2), std::chrono::milliseconds checkInterval = std::chrono::milliseconds(10))
    {
        auto startTime = std::chrono::steady_clock::now();
        while (std::chrono::steady_clock::now() - startTime < timeout) {
            if (condition()) {
                return true;
            }
            std::this_thread::sleep_for(checkInterval);
        }
        return condition(); // Final check
    }


    void SetUp() override
    {
        em = &EventManager::Get();
    }

    void TearDown() override
    {
    }
};

struct QueueCountingListener
{
    std::atomic<int> callCount{0};
    std::atomic<int> lastReceivedData{0}; // Optional: to store last data

    void operator()(const TestEventA& event)
    {
        callCount++;
        lastReceivedData.store(event.data);
    }
};

TEST_F(EventManagerQueueDispatchTest, EventPriorityOrder)
{
    std::vector<int> receivedDataOrder;
    std::mutex orderMutex; // Protect receivedDataOrder

    const int lowPriorityData     = 1;
    const int normalPriorityData  = 2;
    const int highPriorityData    = 3;
    const int highestPriorityData = 4;

    auto listener = [&](const TestEventA& event) {
        std::lock_guard<std::mutex> lock(orderMutex);
        receivedDataOrder.push_back(event.data);
    };

    EventManager::Handle handle = em->subscribeQueued<TestEventA>(listener);
    ASSERT_NE(handle, EventManager::kInvalidHandle);

    // mixture enqueue
    em->enqueue(std::make_shared<TestEventA>(normalPriorityData, "Normal Prio"), EventPriority::Normal);
    em->enqueue(std::make_shared<TestEventA>(highestPriorityData, "Highest Prio"), EventPriority::Highest);
    em->enqueue(std::make_shared<TestEventA>(lowPriorityData, "Low Prio"), EventPriority::Low);
    em->enqueue(std::make_shared<TestEventA>(highPriorityData, "High Prio"), EventPriority::High);

    bool processed = waitForCondition([&]() {
                                          std::lock_guard<std::mutex> lock(orderMutex);
                                          return receivedDataOrder.size() == 4;
                                      },
                                      std::chrono::seconds(3));
    ASSERT_TRUE(processed) << "Not all prioritized events processed within timeout.";

    std::vector expectedOrder = {highestPriorityData, highPriorityData, normalPriorityData, lowPriorityData};
    std::lock_guard lock(orderMutex); // Lock for final check
    ASSERT_EQ(receivedDataOrder.size(), 4);
    EXPECT_EQ(receivedDataOrder, expectedOrder);

    ASSERT_TRUE(em->unsubscribe(handle));
}

TEST_F(EventManagerQueueDispatchTest, ConcurrentSubscribeAndEnqueueStability)
{
    const int numSubscriberThreads    = 4;
    const int subscriptionsPerThread  = 20;
    const int numEnqueuerThreads      = 4;
    const int eventsPerEnqueuerThread = 50; // Total events = 4 * 50 = 200

    std::atomic<int> totalSuccessfulSubscriptions{0};
    std::atomic<long> totalExpectedListenerCalls{0}; // Sum of (events * listeners_active_at_that_time) - complex
    // Simpler: total listener invocations
    std::atomic<long> totalActualListenerInvocations{0};

    std::vector<EventManager::Handle> allCreatedHandles;
    std::mutex handlesMutex;

    std::vector<std::thread> threads;

    // Subscriber Task
    auto subscriberWorker = [&]() {
        for (int i = 0; i < subscriptionsPerThread; ++i) {
            auto handle = em->subscribeQueued<TestEventA>([&](const TestEventA& /*event*/) {
                totalActualListenerInvocations++;
            });

            if (handle != EventManager::kInvalidHandle) {
                totalSuccessfulSubscriptions++;
                std::lock_guard<std::mutex> lock(handlesMutex);
                allCreatedHandles.push_back(handle);
            }
            // Introduce some varied timing
            std::this_thread::sleep_for(std::chrono::microseconds(rand() % 500 + 50));
        }
    };

    // Enqueuer Task
    auto enqueuerWorker = [&]() {
        for (int i = 0; i < eventsPerEnqueuerThread; ++i) {
            em->enqueue(std::make_shared<TestEventA>(i, "ConcurrentEnqueue"), EventPriority::Normal);
            // Introduce some varied timing
            std::this_thread::sleep_for(std::chrono::microseconds(rand() % 200 + 20));
        }
    };

    // Start subscriber threads
    for (int i = 0; i < numSubscriberThreads; ++i) {
        threads.emplace_back(subscriberWorker);
    }
    // Start enqueuer threads
    for (int i = 0; i < numEnqueuerThreads; ++i) {
        threads.emplace_back(enqueuerWorker);
    }

    // Join all threads
    for (auto& t : threads) {
        t.join();
    }

    // Wait for all queued events to be processed
    // This is the tricky part. We need to estimate a "sufficient" time.
    // A better way would be if EventManager exposed a way to know if the queue is idle.
    // For now, wait based on expected work.
    const int totalExpectedSubscriptions = numSubscriberThreads * subscriptionsPerThread;
    const int totalEnqueuedEvents        = numEnqueuerThreads * eventsPerEnqueuerThread;

    // Heuristic: wait a bit longer if many events or subscriptions
    long expectedTotalCallsIfAllSubscribedBeforeAllEnqueued = (long)totalExpectedSubscriptions * totalEnqueuedEvents;

    // Wait for listener invocations to stabilize
    bool stabilized = waitForCondition([&, prev_calls = 0L]() mutable {
                                           long current_calls = totalActualListenerInvocations.load();
                                           if (current_calls == prev_calls && current_calls >= 0) {
                                               // >=0 for initial state
                                               // If no change for a few checks, assume stabilized.
                                               // This condition needs to be adapted if some events are still legitimately processing
                                               // For this test, once threads are joined, all enqueueing is done.
                                               // We are just waiting for the queue to drain.
                                               return true;
                                           }
                                           prev_calls = current_calls;
                                           return false;
                                       },
                                       std::chrono::seconds(5 + totalEnqueuedEvents / 100)); // Adjust timeout

    ASSERT_TRUE(stabilized) << "Listener invocation count did not stabilize.";

    EXPECT_EQ(totalSuccessfulSubscriptions.load(), totalExpectedSubscriptions);

    // Verification of totalActualListenerInvocations:
    // If all N*M subscriptions happened *before* any of K*P events were enqueued,
    // then totalActualListenerInvocations should be (N*M) * (K*P).
    // Since they happen concurrently, this exact number is not guaranteed.
    // What we can expect:
    // 1. It should be greater than 0 if events and subscriptions occurred.
    // 2. It should be <= expectedTotalCallsIfAllSubscribedBeforeAllEnqueued.
    // 3. Every event should have been processed by the listeners *active at the time of its processing*.
    if (totalExpectedSubscriptions > 0 && totalEnqueuedEvents > 0) {
        EXPECT_GT(totalActualListenerInvocations.load(), 0L)
            << "Listeners were subscribed and events enqueued, but no listener calls were registered.";
    }
    std::cout << "ConcurrentSubscribeAndEnqueueStability: Total Subscriptions = " << totalSuccessfulSubscriptions.load()
        << ", Total Listener Invocations = " << totalActualListenerInvocations.load()
        << " (Upper bound if all subs before all enqueues: " << expectedTotalCallsIfAllSubscribedBeforeAllEnqueued << ")"
        << std::endl;

    // Clean up all handles
    {
        std::lock_guard<std::mutex> lock(handlesMutex);
        ASSERT_EQ(allCreatedHandles.size(), totalSuccessfulSubscriptions.load());
        int unsubscribedCount = 0;
        for (auto handle : allCreatedHandles) {
            if (em->unsubscribe(handle)) {
                unsubscribedCount++;
            }
        }
        EXPECT_EQ(unsubscribedCount, totalSuccessfulSubscriptions.load());
    }

    // Final sanity check: EventManager still usable
    auto finalListener = std::make_shared<QueueCountingListener>();
    auto finalHandle   = em->subscribeQueued<TestEventA>([&](const TestEventA& event) { (*finalListener)(event); });
    ASSERT_NE(finalHandle, EventManager::kInvalidHandle);
    em->enqueue(std::make_shared<TestEventA>(1.0, ""), EventPriority::Normal);
    waitForCondition([&]() { return finalListener->callCount.load() == 1; });
    EXPECT_EQ(finalListener->callCount.load(), 1);
    EXPECT_TRUE(em->unsubscribe(finalHandle));
}