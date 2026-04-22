include(CMakeParseArguments)

function(bee_group_sources)
    set(options)
    set(one_value_args TARGET BASE_DIR)
    set(multi_value_args SOURCES)

    cmake_parse_arguments(BEE_GROUP "${options}" "${one_value_args}" "${multi_value_args}" ${ARGN})

    if(NOT BEE_GROUP_TARGET)
        message(FATAL_ERROR "bee_group_sources requires TARGET")
    endif()

    if(NOT BEE_GROUP_BASE_DIR)
        set(BEE_GROUP_BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
    endif()

    if(BEE_GROUP_SOURCES)
        source_group(TREE "${BEE_GROUP_BASE_DIR}" FILES ${BEE_GROUP_SOURCES})
    endif()
endfunction()

function(bee_add_component)
    set(options)
    set(one_value_args NAME TYPE PUBLIC_INCLUDE_DIR)
    set(multi_value_args SOURCES PUBLIC_HEADERS PRIVATE_INCLUDE_DIRS DEPENDS)

    cmake_parse_arguments(BEE_COMP "${options}" "${one_value_args}" "${multi_value_args}" ${ARGN})

    if(NOT BEE_COMP_NAME)
        message(FATAL_ERROR "bee_add_component requires NAME")
    endif()

    if(NOT BEE_COMP_TYPE)
        message(FATAL_ERROR "bee_add_component requires TYPE")
    endif()

    if(BEE_PUBLIC_INCLUDE_DIR)
        set(bee_comp_public_includes "${BEE_PUBLIC_INCLUDE_DIR}")
    endif()
    if(BEE_COMP_PUBLIC_INCLUDE_DIR)
        list(APPEND bee_comp_public_includes "${BEE_COMP_PUBLIC_INCLUDE_DIR}")
    endif()
    if(NOT bee_comp_public_includes)
        message(FATAL_ERROR "bee_add_component requires PUBLIC_INCLUDE_DIR or BEE_PUBLIC_INCLUDE_DIR")
    endif()

    if(BEE_COMP_TYPE STREQUAL "INTERFACE")
        add_library(${BEE_COMP_NAME} INTERFACE)
        target_include_directories(${BEE_COMP_NAME}
            INTERFACE
                $<BUILD_INTERFACE:${bee_comp_public_includes}>
                $<INSTALL_INTERFACE:include>
        )
        if(BEE_COMP_DEPENDS)
            target_link_libraries(${BEE_COMP_NAME} INTERFACE ${BEE_COMP_DEPENDS})
        endif()
        target_compile_features(${BEE_COMP_NAME} INTERFACE cxx_std_23)

        # 让 IDE（CLion / VS）能看到 header-only 组件：额外创建一个带 SOURCES 的
        # 伴随自定义目标，名字是 <Name>.Headers，FOLDER 归到 Bee/HeaderOnly。
        if(BEE_COMP_PUBLIC_HEADERS)
            add_custom_target(${BEE_COMP_NAME}.Headers SOURCES ${BEE_COMP_PUBLIC_HEADERS})
            set_target_properties(${BEE_COMP_NAME}.Headers PROPERTIES FOLDER "Bee/HeaderOnly")
            bee_group_sources(
                TARGET ${BEE_COMP_NAME}.Headers
                BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}"
                SOURCES ${BEE_COMP_PUBLIC_HEADERS}
            )
        endif()
        set(_bee_folder "Bee/HeaderOnly")
    else()
        add_library(${BEE_COMP_NAME} ${BEE_COMP_TYPE}
            ${BEE_COMP_SOURCES}
            ${BEE_COMP_PUBLIC_HEADERS}
        )
        target_include_directories(${BEE_COMP_NAME}
            PUBLIC
                $<BUILD_INTERFACE:${bee_comp_public_includes}>
                $<INSTALL_INTERFACE:include>
            PRIVATE
                ${BEE_COMP_PRIVATE_INCLUDE_DIRS}
        )
        if(BEE_COMP_DEPENDS)
            target_link_libraries(${BEE_COMP_NAME} PUBLIC ${BEE_COMP_DEPENDS})
        endif()
        target_compile_features(${BEE_COMP_NAME} PUBLIC cxx_std_23)
        set(_bee_folder "Bee/Libraries")
    endif()
    set_target_properties(${BEE_COMP_NAME} PROPERTIES FOLDER "${_bee_folder}")

    set(bee_comp_all_sources ${BEE_COMP_SOURCES} ${BEE_COMP_PUBLIC_HEADERS})
    bee_group_sources(
        TARGET ${BEE_COMP_NAME}
        BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}"
        SOURCES ${bee_comp_all_sources}
    )

    add_library(Bee::${BEE_COMP_NAME} ALIAS ${BEE_COMP_NAME})
endfunction()

function(bee_add_component_test)
    set(options)
    set(one_value_args COMPONENT TARGET)
    set(multi_value_args SOURCES)

    cmake_parse_arguments(BEE_TEST "${options}" "${one_value_args}" "${multi_value_args}" ${ARGN})

    if(NOT BEE_BUILD_TESTS)
        return()
    endif()

    if(NOT BEE_TEST_COMPONENT OR NOT BEE_TEST_TARGET)
        message(FATAL_ERROR "bee_add_component_test requires COMPONENT and TARGET")
    endif()

    add_executable(${BEE_TEST_TARGET} ${BEE_TEST_SOURCES})
    set_target_properties(${BEE_TEST_TARGET} PROPERTIES FOLDER "Bee/Tests")
    bee_group_sources(
        TARGET ${BEE_TEST_TARGET}
        BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}"
        SOURCES ${BEE_TEST_SOURCES}
    )
    target_link_libraries(${BEE_TEST_TARGET}
        PRIVATE
            Bee::${BEE_TEST_COMPONENT}
            GTest::gtest_main
    )
    target_compile_features(${BEE_TEST_TARGET} PRIVATE cxx_std_23)

    include(GoogleTest)
    gtest_discover_tests(${BEE_TEST_TARGET})
endfunction()

# ---------------------------------------------------------------------------
# bee_add_component_benchmark
#
# 为某组件创建 google-benchmark 可执行目标。与 bee_add_component_test 对称：
#
#   bee_add_component_benchmark(
#       COMPONENT Task
#       TARGET Task.Bench
#       SOURCES bench_task.cpp bench_workpool.cpp ...
#   )
#
# 由顶层 `BEE_BUILD_BENCHMARKS` 选项控制是否启用。需要 google-benchmark
# 由 vcpkg 或包管理器提供；未找到时跳过并给出提示，不影响主构建。
# ---------------------------------------------------------------------------
function(bee_add_component_benchmark)
    set(options)
    set(one_value_args COMPONENT TARGET)
    set(multi_value_args SOURCES)

    cmake_parse_arguments(BEE_BENCH "${options}" "${one_value_args}" "${multi_value_args}" ${ARGN})

    if(NOT BEE_BUILD_BENCHMARKS)
        return()
    endif()

    if(NOT BEE_BENCH_COMPONENT OR NOT BEE_BENCH_TARGET)
        message(FATAL_ERROR "bee_add_component_benchmark requires COMPONENT and TARGET")
    endif()

    find_package(benchmark CONFIG QUIET)
    if(NOT benchmark_FOUND)
        message(STATUS "google-benchmark not found; skip ${BEE_BENCH_TARGET}.")
        return()
    endif()

    add_executable(${BEE_BENCH_TARGET} ${BEE_BENCH_SOURCES})
    set_target_properties(${BEE_BENCH_TARGET} PROPERTIES FOLDER "Bee/Benchmarks")
    bee_group_sources(
        TARGET ${BEE_BENCH_TARGET}
        BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}"
        SOURCES ${BEE_BENCH_SOURCES}
    )
    target_link_libraries(${BEE_BENCH_TARGET}
        PRIVATE
            Bee::${BEE_BENCH_COMPONENT}
            benchmark::benchmark
            benchmark::benchmark_main
    )
    target_compile_features(${BEE_BENCH_TARGET} PRIVATE cxx_std_23)
endfunction()
