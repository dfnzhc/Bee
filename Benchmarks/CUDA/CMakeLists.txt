if(NOT BEE_TENSOR_WITH_CUDA)
    message(STATUS "Benchmarks/CUDA: BEE_TENSOR_WITH_CUDA=OFF, skip CUDA bench")
    return()
endif()

bee_add_component_benchmark(
    COMPONENT Tensor
    TARGET CUDA.Bench
    SOURCES
        EWiseCudaBench.cpp
        MatmulCudaBench.cpp
        MemcpyBench.cpp
        ReduceCudaBench.cpp
)
