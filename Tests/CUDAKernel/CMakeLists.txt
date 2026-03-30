if(TARGET Bee::CUDAKernel)
    include(CheckLanguage)

    # 纯 C++ 测试继续走统一组件测试入口。
    bee_add_component_test(
        COMPONENT CUDAKernel
        TARGET CUDAKernel.Tests
        SOURCES
            CUDAKernelTests.cpp
            CheckTests.cpp
            LaunchTests.cpp
            VectorizeTests.cpp
    )

    # CUDA 相关测试通过“后置配置”追加到同一测试 target：
    # 1) 不改 bee_add_component_test
    # 2) 支持 .cu 源编译与设备链接
    if(NOT CMAKE_CUDA_COMPILER_LOADED)
        check_language(CUDA)
        if(CMAKE_CUDA_COMPILER)
            enable_language(CUDA)
        endif()
    endif()

    if(CMAKE_CUDA_COMPILER_LOADED)
        target_sources(CUDAKernel.Tests PRIVATE
            BlockDeviceTests.cpp
            BlockDeviceTestKernels.cu
            WgmmaDeviceTestKernels.cu
            KernelOpsTests.cu
        )

        set_target_properties(CUDAKernel.Tests PROPERTIES
            CUDA_STANDARD 20
            CUDA_STANDARD_REQUIRED ON
            CUDA_SEPARABLE_COMPILATION OFF
        )
    else()
        message(STATUS "CUDA language is unavailable, skip CUDA sources in CUDAKernel.Tests.")
    endif()
else()
    message(STATUS "CUDAKernel tests are skipped because CUDAKernel component is unavailable.")
endif()
