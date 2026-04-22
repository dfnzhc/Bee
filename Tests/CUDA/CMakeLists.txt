if(TARGET Bee::CUDA)
    include(CheckLanguage)

    # 纯 C++ 测试走统一组件测试入口。
    bee_add_component_test(
        COMPONENT CUDA
        TARGET CUDA.Tests
        SOURCES
            ApiTests.cpp
            InfraTests.cpp
    )

    if(NOT CMAKE_CUDA_COMPILER_LOADED)
        check_language(CUDA)
        if(CMAKE_CUDA_COMPILER)
            enable_language(CUDA)
        endif()
    endif()

    if(CMAKE_CUDA_COMPILER_LOADED)
        target_sources(CUDA.Tests PRIVATE
            KernelTests.cu
        )

        set_target_properties(CUDA.Tests PROPERTIES
            CUDA_STANDARD 20
            CUDA_STANDARD_REQUIRED ON
            CUDA_SEPARABLE_COMPILATION OFF
            CUDA_ARCHITECTURES "${CMAKE_CUDA_ARCHITECTURES}"
        )
    else()
        message(STATUS "CUDA language is unavailable, skip CUDA sources in CUDA.Tests.")
    endif()
else()
    message(STATUS "CUDA tests are skipped because Bee::CUDA component is unavailable.")
endif()
