bee_add_component_test(
    COMPONENT Task
    TARGET Task.Tests
    SOURCES
        TaskStateTests.cpp
        TaskBasicTests.cpp
        SchedulerTests.cpp
        ThenTests.cpp
        WhenAllTests.cpp
        WhenAnyTests.cpp
)
