bee_add_component_test(
    COMPONENT Tensor
    TARGET Tensor.Tests
    SOURCES
        TensorSkeletonTests.cpp
        DTypeTests.cpp
        ShapeTests.cpp
        StorageTests.cpp
        TensorTests.cpp
)
