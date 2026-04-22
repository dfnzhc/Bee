bee_add_component_benchmark(
    COMPONENT Tensor
    TARGET Tensor.Bench
    SOURCES
        BenchUtil.hpp
        EWiseBench.cpp
        ReduceBench.cpp
        MatmulBench.cpp
)
