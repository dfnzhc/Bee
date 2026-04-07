if(TARGET Bee::cuKernels)
    include(CheckLanguage)

    # 纯 C++ 测试继续走统一组件测试入口。
    bee_add_component_test(
        COMPONENT cuKernels
        TARGET cuKernels.Tests
        SOURCES
            CUDAKernelTests.cpp
    )

    if(NOT CMAKE_CUDA_COMPILER_LOADED)
        check_language(CUDA)
        if(CMAKE_CUDA_COMPILER)
            enable_language(CUDA)
        endif()
    endif()

    if(CMAKE_CUDA_COMPILER_LOADED)
        target_sources(cuKernels.Tests PRIVATE
            KernelOpsTests.cu
        )

        set_target_properties(cuKernels.Tests PROPERTIES
            CUDA_STANDARD 20
            CUDA_STANDARD_REQUIRED ON
            CUDA_SEPARABLE_COMPILATION OFF
            CUDA_ARCHITECTURES "${CMAKE_CUDA_ARCHITECTURES}"
        )
    else()
        message(STATUS "CUDA language is unavailable, skip CUDA sources in cuKernels.Tests.")
    endif()
else()
    message(STATUS "cuKernels tests are skipped because cuKernels component is unavailable.")
endif()
