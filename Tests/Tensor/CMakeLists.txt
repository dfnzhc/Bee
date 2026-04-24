bee_add_component_test(
    COMPONENT Tensor
    TARGET Tensor.Tests
    SOURCES
        TensorSkeletonTests.cpp
        DTypeTests.cpp
        ShapeTests.cpp
        StorageTests.cpp
        TensorTests.cpp
        ViewTests.cpp
        CreationTests.cpp
        BroadcastTests.cpp
        ElementWiseTests.cpp
        ReduceTests.cpp
        CastTests.cpp
        RandomTests.cpp
        MatmulTests.cpp
        GemmTests.cpp
        CudaStubTests.cpp
        IntegrationTests.cpp
        CudaRuntimeTests.cpp
        ContiguousCudaTests.cpp
)

