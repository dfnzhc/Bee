bee_add_component_benchmark(
    COMPONENT Task
    TARGET Task.Bench
    SOURCES
        TaskSubmitBench.cpp
        WorkPoolThroughputBench.cpp
        TaskGraphScaleBench.cpp
        WhenAllDepthBench.cpp
)
