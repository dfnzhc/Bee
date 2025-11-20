if (NOT BEE_BUILD_TESTS)
    return()
endif ()

add_subdirectory(Units)
add_subdirectory(Benchmarks)