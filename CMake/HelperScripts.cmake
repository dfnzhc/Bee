# 设置代码文件的文件组
function(AssignSourceGroup)
    foreach (_source IN ITEMS ${ARGN})
        if (IS_ABSOLUTE "${_source}")
            file(RELATIVE_PATH _source_rel "${CMAKE_CURRENT_SOURCE_DIR}" "${_source}")
        else ()
            set(_source_rel "${_source}")
        endif ()
        get_filename_component(_source_path "${_source_rel}" PATH)
        string(REPLACE "/" "\\" _source_path_msvc "${_source_path}")
        source_group("${_source_path_msvc}" FILES "${_source}")
    endforeach ()
endfunction(AssignSourceGroup)

# 添加测试程序
macro(AddTestProgram TestFile Libraries Category)
    get_filename_component(FILE_NAME ${TestFile} NAME_WE)
    add_executable(${FILE_NAME} ${TestFile})
    gtest_discover_tests(${FILE_NAME})

    set_target_properties(${FILE_NAME} PROPERTIES FOLDER "Tests/${Category}")

    target_link_libraries(${FILE_NAME} PUBLIC ${Libraries})
    add_test(NAME "${FILE_NAME}Test" COMMAND ${FILE_NAME})
endmacro()

# 添加组件
function(BeeAddComponent)
    set(options SHARED)
    set(oneValueArgs NAME CUDA_DEPEND)
    set(multiValueArgs SOURCES HEADERS CUDA_SOURCES COMPILE_DEFINITIONS COMPILE_OPTIONS INCLUDE_DIRECTORIES LINK_LIBRARIES)

    cmake_parse_arguments(COMP "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    if (NOT COMP_NAME)
        BeeError("NAME is required")
        message(FATAL_ERROR "")
    endif ()

    set(target_name "Bee${COMP_NAME}")
    if (TARGET ${target_name})
        BeeWarn("Target ${target_name} already exists, skipping creation")
        return()
    endif ()

    set(ALL_SOURCES ${COMP_SOURCES} ${COMP_CUDA_SOURCES})

    # 是否是纯头文件组件
    set(is_header_only FALSE)
    if (NOT ALL_SOURCES)
        set(is_header_only TRUE)
    endif ()

    if (is_header_only)
        # 纯头文件库
        add_library(${target_name} INTERFACE)
        set(include_scope INTERFACE)
    else ()
        # 有源码的库：根据 SHARED 选项决定类型
        if (COMP_SHARED)
            set(lib_type SHARED)
        else ()
            set(lib_type STATIC)
        endif ()
        add_library(${target_name} ${lib_type} ${ALL_SOURCES})
        set(include_scope PUBLIC)
    endif ()

    # CUDA 编译特定配置
    set(is_cuda_target FALSE)
    if (COMP_CUDA_SOURCES)
        set(is_cuda_target TRUE)

        set_target_properties(${target_name} PROPERTIES CUDA_SEPARABLE_COMPILATION ON)
        set_target_properties(${target_name} PROPERTIES CUDA_STANDARD 20)
    endif ()

    # C++ 标准策略控制
    if (COMP_CUDA_DEPEND)
        # 被 CUDA 依赖的组件 -> 强制 C++20
        set_target_properties(${target_name} PROPERTIES
                CXX_STANDARD 20
                CXX_STANDARD_REQUIRED ON
        )
        target_compile_features(${target_name} ${include_scope} cxx_std_20)

        # 定义宏，供头文件判断
        target_compile_definitions(${target_name} ${include_scope}
                $<$<COMPILE_LANGUAGE:CUDA>:BEE_USE_CUDA>
        )
    elseif (is_cuda_target)
        # CUDA 组件自身 -> 强制 C++20
        set_target_properties(${target_name} PROPERTIES CXX_STANDARD 20)
        target_compile_features(${target_name} ${include_scope} cxx_std_20)
    else ()
        # 纯 CPU 组件 -> 默认 C++23
        set_target_properties(${target_name} PROPERTIES CXX_STANDARD 23)
        target_compile_features(${target_name} ${include_scope} cxx_std_23)
    endif ()

    # 通用属性配置

    # 公共包含
    target_include_directories(${target_name} ${include_scope} ${BEE_INCLUDE_DIR})
    
    # 私有包含自身目录
    if (NOT is_header_only)
        target_include_directories(${target_name}
                PRIVATE
                ${CMAKE_CURRENT_SOURCE_DIR}
                "${BEE_INCLUDE_DIR}/Bee/${COMP_NAME}"
        )
    endif()

    # 用户指定的包含目录
    if (COMP_INCLUDE_DIRECTORIES)
        target_include_directories(${target_name} ${include_scope} ${COMP_INCLUDE_DIRECTORIES})
    endif ()

    # 处理编译定义
    if (COMP_COMPILE_DEFINITIONS)
        target_compile_definitions(${target_name} ${include_scope} ${COMP_COMPILE_DEFINITIONS})
    endif ()

    # 处理编译选项
    if (COMP_COMPILE_OPTIONS)
        target_compile_options(${target_name} ${include_scope} ${COMP_COMPILE_OPTIONS})
    endif ()

    # 处理链接库
    if (COMP_LINK_LIBRARIES)
        target_link_libraries(${target_name} ${include_scope} ${COMP_LINK_LIBRARIES})
    endif ()

    # 创建别名
    add_library(Bee::${COMP_NAME} ALIAS ${target_name})
endfunction()
