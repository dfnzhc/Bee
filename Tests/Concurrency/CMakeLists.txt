bee_add_component_test(
    COMPONENT Concurrency
    TARGET Concurrency.Tests
    SOURCES
        ConcurrencyTests.cpp
        ThreadingTests.cpp
        SPSCQueueTests.cpp
        MPMCQueueTests.cpp
        ChaseLevDequeTests.cpp
)
