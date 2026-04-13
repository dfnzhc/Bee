bee_add_component_test(
    COMPONENT Task
    TARGET Task.Tests
    SOURCES
        TaskStateTests.cpp
        TaskTests.cpp
        WhenAllTests.cpp
        TaskGraphTests.cpp
        TaskGraphErrorTests.cpp
        TaskGraphCancelTests.cpp
        ParallelForEachTests.cpp
        ParallelTransformTests.cpp
        ParallelReduceTests.cpp
        ParallelScanTests.cpp
        ParallelSortTests.cpp
)
