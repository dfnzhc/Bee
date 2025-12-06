
if (BEE_BUILD_TESTS)
    add_subdirectory(Units)
endif ()

if (BEE_BUILD_CUDA_TESTS)
    add_subdirectory(CUDA)
endif ()