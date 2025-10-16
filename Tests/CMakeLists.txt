
# 单元测试
if (BEE_BUILD_TESTS)
    enable_testing()
    add_subdirectory(Units)
endif ()

# 基准测试
if (BEE_BUILD_BENCHMARKS)
    add_subdirectory(Benchmarks)
endif ()